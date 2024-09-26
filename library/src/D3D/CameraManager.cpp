#include <cstring>
#include <CameraManager.hpp>

CameraManager::CameraManager(ID3D12Device* device, MemoryManager* memoryManager)
	: m_activeCameraIndex{ 0u }, m_cameraBufferInstanceSize{ 0u },
	m_cameraBuffer{ GetCPUResource<Buffer>(device, memoryManager) },
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
	// Since I will set the camera data as a Constant buffer, I need to align the size.
	// The first one would be fine and be aligned to 256B but the subsequent ones wouldn't
	// be aligned unless the size is aligned.
	m_cameraBufferInstanceSize    = Buffer::GetCBVAlignedAddress(sizeof(CameraBufferData));

	const UINT64 cameraBufferSize = m_cameraBufferInstanceSize * frameCount;

	// Upload buffers must be in the generic read state.
	m_cameraBuffer.Create(cameraBufferSize, D3D12_RESOURCE_STATE_GENERIC_READ);
}

void CameraManager::Update(UINT64 index) const noexcept
{
	std::uint8_t* bufferAddress = m_cameraBuffer.CPUHandle() + m_cameraBufferInstanceSize * index;

	constexpr size_t matrixSize = sizeof(DirectX::XMMATRIX);

	// Although I am not a big fan of useless checks. This would potentially allow us
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
	std::vector<D3DDescriptorManager>& descriptorManagers, size_t cameraRegister, size_t registerSpace,
	D3D12_SHADER_VISIBILITY shaderStage
) const noexcept {
	for (auto& descriptorManager : descriptorManagers)
		descriptorManager.AddRootCBV(cameraRegister, registerSpace, shaderStage);
}

void CameraManager::SetDescriptorLayoutCompute(
	std::vector<D3DDescriptorManager>& descriptorManagers, size_t cameraRegister, size_t registerSpace
) const noexcept {
	for (auto& descriptorManager : descriptorManagers)
		descriptorManager.AddRootCBV(cameraRegister, registerSpace, D3D12_SHADER_VISIBILITY_ALL);
}

void CameraManager::SetDescriptorGraphics(
	std::vector<D3DDescriptorManager>& descriptorManagers, size_t cameraRegister, size_t registerSpace
) const {
	for (size_t index = 0u; index < std::size(descriptorManagers); ++index)
		// Root descriptors don't have bound checks and the user must do that themselves but it is
		// fine to bind the same resource as multiple root descriptors.
		descriptorManagers[index].SetRootCBV(
			cameraRegister, registerSpace,
			m_cameraBuffer.GetGPUAddress() + (index * m_cameraBufferInstanceSize), true
		);
}

void CameraManager::SetDescriptorCompute(
	std::vector<D3DDescriptorManager>& descriptorManagers, size_t cameraRegister, size_t registerSpace
) const {
	for (size_t index = 0u; index < std::size(descriptorManagers); ++index)
		descriptorManagers[index].SetRootCBV(
			cameraRegister, registerSpace,
			m_cameraBuffer.GetGPUAddress() + (index * m_cameraBufferInstanceSize), false
		);
}
