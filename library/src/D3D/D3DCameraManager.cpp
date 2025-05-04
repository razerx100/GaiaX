#include <cstring>
#include <D3DCameraManager.hpp>

namespace Gaia
{
CameraManager::CameraManager(ID3D12Device* device, MemoryManager* memoryManager)
	: m_activeCameraIndex{ 0u }, m_cameraBufferInstanceSize{ 0u },
	m_cameraBuffer{ GetCPUResource<Buffer>(device, memoryManager) }
{}

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

void CameraManager::Update(UINT64 index, const Camera& cameraData) const noexcept
{
	std::uint8_t* bufferAddress = m_cameraBuffer.CPUHandle() + m_cameraBufferInstanceSize * index;

	constexpr size_t matrixSize = sizeof(DirectX::XMMATRIX);

	const DirectX::XMMATRIX view = cameraData.GetViewMatrix();

	memcpy(bufferAddress, &view, matrixSize);

	// Projection matrix's address
	bufferAddress += matrixSize;

	cameraData.GetProjectionMatrix(bufferAddress);

	// Frustum's address
	bufferAddress += matrixSize;

	// In the clip space.
	const Frustum viewFrustum        = cameraData.GetViewFrustum(view);

	constexpr size_t frustumDataSize = sizeof(Frustum);

	memcpy(bufferAddress, &viewFrustum, frustumDataSize);

	// View position's address
	bufferAddress += frustumDataSize;

	const DirectX::XMFLOAT3 viewPosition = cameraData.GetCameraPosition();

	memcpy(bufferAddress, &viewPosition, sizeof(DirectX::XMFLOAT3));
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
}
