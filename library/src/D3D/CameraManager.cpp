#include <cstring>
#include <CameraManager.hpp>

CameraManager::CameraManager(ID3D12Device* device, MemoryManager* memoryManager)
	: m_activeCameraIndex{ 0u }, m_cameraBufferInstanceSize{ 0u },
	m_cameraBuffer{ device, memoryManager, D3D12_HEAP_TYPE_UPLOAD },
	m_cameras{}
{}

std::uint32_t CameraManager::AddCamera(std::shared_ptr<Camera> camera) noexcept
{
	const auto index = static_cast<std::uint32_t>(std::size(m_cameras));

	m_cameras.emplace_back(std::move(camera));

	m_activeCameraIndex = index;

	return index;
}

void CameraManager::RemoveCamera(std::uint32_t index) noexcept
{
	m_cameras.erase(std::begin(m_cameras) + index);
}

void CameraManager::CreateBuffer(std::uint32_t frameCount)
{
	m_cameraBufferInstanceSize    = static_cast<UINT64>(sizeof(CameraBufferData));

	const UINT64 cameraBufferSize = m_cameraBufferInstanceSize * frameCount;

	m_cameraBuffer.Create(cameraBufferSize, D3D12_RESOURCE_STATE_GENERIC_READ);
}

void CameraManager::Update(UINT64 index) const noexcept
{
	std::uint8_t* bufferAddress = m_cameraBuffer.CPUHandle() + m_cameraBufferInstanceSize * index;

	constexpr size_t matrixSize = sizeof(DirectX::XMMATRIX);

	// Although I am not a big of useless checks. This would potentially allow us
	// to run the renderer without having any cameras.
	if (m_activeCameraIndex < std::size(m_cameras)) [[likely]]
	{
		const std::shared_ptr<Camera>& activeCamera = m_cameras[m_activeCameraIndex];

		const DirectX::XMMATRIX view = activeCamera->GetViewMatrix();

		memcpy(bufferAddress, &view, matrixSize);

		activeCamera->GetProjectionMatrix(bufferAddress + matrixSize);
	}
}

void CameraManager::SetDescriptorLayoutGraphics(
	std::vector<D3DDescriptorManager>& descriptorBuffers, size_t cameraRegister, size_t registerSpace,
	D3D12_SHADER_VISIBILITY shaderStage
) const noexcept {
	for (auto& descriptorBuffer : descriptorBuffers)
		descriptorBuffer.AddRootCBV(cameraRegister, registerSpace, shaderStage);
}

void CameraManager::SetDescriptorLayoutCompute(
	std::vector<D3DDescriptorManager>& descriptorBuffers, size_t cameraRegister, size_t registerSpace
) const noexcept {
	for (auto& descriptorBuffer : descriptorBuffers)
		descriptorBuffer.AddRootCBV(cameraRegister, registerSpace, D3D12_SHADER_VISIBILITY_ALL);
}

void CameraManager::SetDescriptorGraphics(
	std::vector<D3DDescriptorManager>& descriptorBuffers, size_t cameraRegister, size_t registerSpace
) const {
	for (size_t index = 0u; index < std::size(descriptorBuffers); ++index)
		// I will keep it like this for now. As you can't really pass the size of a root descriptor
		// I am not sure if you can bind a single resource to multiple descriptors. But hoping
		// I can just read the necessary data in the shader and it just works.
		descriptorBuffers[index].SetRootCBV(
			cameraRegister, registerSpace,
			m_cameraBuffer.GetGPUAddress() + (index * m_cameraBufferInstanceSize), true
		);
}

void CameraManager::SetDescriptorCompute(
	std::vector<D3DDescriptorManager>& descriptorBuffers, size_t cameraRegister, size_t registerSpace
) const {
	for (size_t index = 0u; index < std::size(descriptorBuffers); ++index)
		descriptorBuffers[index].SetRootCBV(
			cameraRegister, registerSpace,
			m_cameraBuffer.GetGPUAddress() + (index * m_cameraBufferInstanceSize), false
		);
}
