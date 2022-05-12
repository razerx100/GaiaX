#include <cstring>
#include <CameraManager.hpp>

void CameraManager::CopyData(std::uint8_t* cpuHandle) noexcept {
	memcpy(cpuHandle, &m_cameraMatrices, sizeof(CameraMatrices));
}

void CameraManager::SetViewMatrix(const DirectX::XMMATRIX& view) noexcept {
	m_cameraMatrices.view = view;
}

void CameraManager::SetProjectionMatrix(const DirectX::XMMATRIX& projection) noexcept {
	m_cameraMatrices.projection = projection;
}

void CameraManager::SetCamera(const CameraMatrices& camera) noexcept {
	m_cameraMatrices = camera;
}
