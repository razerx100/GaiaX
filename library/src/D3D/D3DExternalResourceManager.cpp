#include <D3DExternalResourceManager.hpp>
#include <limits>

D3DExternalResourceManager::D3DExternalResourceManager(ID3D12Device* device, MemoryManager* memoryManager)
	: m_resourceFactory{ device, memoryManager }, m_gfxExtensions{}
{}

void D3DExternalResourceManager::OnGfxExtensionAddition(GraphicsTechniqueExtension& gfxExtension)
{
	const std::vector<ExternalBufferDetails>& bufferDetails = gfxExtension.GetBufferDetails();

	for (const ExternalBufferDetails& details : bufferDetails)
	{
		const size_t bufferIndex = m_resourceFactory.CreateExternalBuffer(details.type);

		gfxExtension.SetBuffer(
			m_resourceFactory.GetExternalBufferSP(bufferIndex), details.bufferId,
			static_cast<std::uint32_t>(bufferIndex)
		);
	}
}

void D3DExternalResourceManager::AddGraphicsTechniqueExtension(
	std::shared_ptr<GraphicsTechniqueExtension> extension
) {
	OnGfxExtensionAddition(*extension);

	m_gfxExtensions.emplace_back(std::move(extension));
}

void D3DExternalResourceManager::UpdateExtensionData(size_t frameIndex) const noexcept
{
	for (const GfxExtension& extension : m_gfxExtensions)
		extension->UpdateCPUData(frameIndex);
}

void D3DExternalResourceManager::SetGraphicsDescriptorLayout(
	std::vector<D3DDescriptorManager>& descriptorManagers
) {
	const size_t descriptorManagerCount = std::size(descriptorManagers);

	for (const GfxExtension& extension : m_gfxExtensions)
	{
		const std::vector<ExternalBufferBindingDetails>& bindingDetails = extension->GetBindingDetails();

		for (const ExternalBufferBindingDetails& details : bindingDetails)
		{
			for (size_t index = 0u; index < descriptorManagerCount; ++index)
			{
				D3DDescriptorManager& descriptorManager = descriptorManagers[index];

				descriptorManager.AddRootSRV(
					static_cast<size_t>(details.bindingIndex), s_externalBufferRegisterSpace,
					D3D12_SHADER_VISIBILITY_PIXEL
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
			= bindingDetails.frameIndex != std::numeric_limits<std::uint32_t>::max()
			&& bindingDetails.frameIndex != index;

		if (isSeparateFrameDescriptor)
			continue;

		UpdateDescriptor(descriptorManager, bindingDetails);
	}
}

void D3DExternalResourceManager::UpdateDescriptor(
	D3DDescriptorManager& descriptorManager, const ExternalBufferBindingDetails& bindingDetails
) const {
	const D3DExternalBuffer& d3dBuffer = m_resourceFactory.GetD3DExternalBuffer(
		bindingDetails.externalBufferIndex
	);

	descriptorManager.SetRootSRV(
		static_cast<size_t>(bindingDetails.bindingIndex), s_externalBufferRegisterSpace,
		d3dBuffer.GetBuffer().GetGPUAddress() + bindingDetails.bufferOffset,
		true
	);
}
