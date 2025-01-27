#include <D3DExternalResourceManager.hpp>
#include <limits>

D3DExternalResourceManager::D3DExternalResourceManager(ID3D12Device* device, MemoryManager* memoryManager)
	: m_resourceFactory{ device, memoryManager }, m_gfxExtensions{}
{}

void D3DExternalResourceManager::OnGfxExtensionDeletion(const GraphicsTechniqueExtension& gfxExtension)
{
	const std::vector<std::uint32_t>& externalIndices = gfxExtension.GetExternalBufferIndices();

	for (std::uint32_t externalIndex : externalIndices)
		m_resourceFactory.RemoveExternalBuffer(externalIndex);
}

std::uint32_t D3DExternalResourceManager::AddGraphicsTechniqueExtension(
	std::shared_ptr<GraphicsTechniqueExtension> extension
) {
	return static_cast<std::uint32_t>(m_gfxExtensions.Add(std::move(extension)));
}

void D3DExternalResourceManager::RemoveGraphicsTechniqueExtension(std::uint32_t index) noexcept
{
	OnGfxExtensionDeletion(*m_gfxExtensions[index]);

	m_gfxExtensions.RemoveElement(index);
}

void D3DExternalResourceManager::UpdateExtensionData(size_t frameIndex) const noexcept
{
	for (const GfxExtension_t& extension : m_gfxExtensions)
		extension->UpdateCPUData(frameIndex);
}

void D3DExternalResourceManager::SetGraphicsDescriptorLayout(
	std::vector<D3DDescriptorManager>& descriptorManagers
) {
	const size_t descriptorManagerCount = std::size(descriptorManagers);

	for (const GfxExtension_t& extension : m_gfxExtensions)
	{
		const std::vector<ExternalBufferBindingDetails>& bindingDetails = extension->GetBindingDetails();

		for (const ExternalBufferBindingDetails& details : bindingDetails)
		{
			for (size_t index = 0u; index < descriptorManagerCount; ++index)
			{
				D3DDescriptorManager& descriptorManager = descriptorManagers[index];

				if (details.layoutInfo.type == ExternalBufferType::CPUVisibleUniform)
					descriptorManager.AddRootCBV(
						static_cast<size_t>(details.layoutInfo.bindingIndex),
						s_externalBufferRegisterSpace, D3D12_SHADER_VISIBILITY_PIXEL
					);
				else
					descriptorManager.AddRootSRV(
						static_cast<size_t>(details.layoutInfo.bindingIndex),
						s_externalBufferRegisterSpace, D3D12_SHADER_VISIBILITY_PIXEL
					);
			}
		}
	}
}

void D3DExternalResourceManager::UpdateDescriptor(
	std::vector<D3DDescriptorManager>& descriptorManagers,
	const ExternalBufferBindingDetails& bindingDetails
) const {
	const size_t descriptorManagerCount = std::size(descriptorManagers);

	for (size_t index = 0u; index < descriptorManagerCount; ++index)
	{
		D3DDescriptorManager& descriptorManager = descriptorManagers[index];
		// If there is no frameIndex. Then we add the buffer to all the frames.
		const bool isSeparateFrameDescriptor
			= bindingDetails.descriptorInfo.frameIndex != std::numeric_limits<std::uint32_t>::max()
			&& bindingDetails.descriptorInfo.frameIndex != index;

		if (isSeparateFrameDescriptor)
			continue;

		UpdateDescriptor(descriptorManager, bindingDetails);
	}
}

void D3DExternalResourceManager::UpdateDescriptor(
	D3DDescriptorManager& descriptorManager, const ExternalBufferBindingDetails& bindingDetails
) const {
	const D3DExternalBuffer& d3dBuffer = m_resourceFactory.GetD3DExternalBuffer(
		bindingDetails.descriptorInfo.externalBufferIndex
	);

	if (bindingDetails.layoutInfo.type == ExternalBufferType::CPUVisibleUniform)
		descriptorManager.SetRootCBV(
			static_cast<size_t>(bindingDetails.layoutInfo.bindingIndex), s_externalBufferRegisterSpace,
			d3dBuffer.GetBuffer().GetGPUAddress() + bindingDetails.descriptorInfo.bufferOffset,
			true
		);
	else
		descriptorManager.SetRootSRV(
			static_cast<size_t>(bindingDetails.layoutInfo.bindingIndex), s_externalBufferRegisterSpace,
			d3dBuffer.GetBuffer().GetGPUAddress() + bindingDetails.descriptorInfo.bufferOffset,
			true
		);
}

void D3DExternalResourceManager::UploadExternalBufferGPUOnlyData(
	StagingBufferManager& stagingBufferManager, TemporaryDataBufferGPU& tempGPUBuffer,
	std::uint32_t externalBufferIndex, std::shared_ptr<void> cpuData, size_t srcDataSizeInBytes,
	size_t dstBufferOffset
) const {
	stagingBufferManager.AddBuffer(
		std::move(cpuData),
		static_cast<UINT64>(srcDataSizeInBytes),
		&m_resourceFactory.GetD3DExternalBuffer(
			static_cast<size_t>(externalBufferIndex)
		).GetBuffer(),
		static_cast<UINT64>(dstBufferOffset),
		tempGPUBuffer
	);
}
