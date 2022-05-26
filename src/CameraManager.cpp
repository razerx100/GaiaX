#include <cstring>
#include <CameraManager.hpp>

CameraManager::CameraManager() noexcept : m_cameraMatrices{}, m_fov(65.f) {}

void CameraManager::CopyData(std::uint8_t* cpuHandle) noexcept {
	memcpy(cpuHandle, &m_cameraMatrices, sizeof(CameraMatrices));
}

void CameraManager::SetViewMatrix(const DirectX::XMMATRIX& view) noexcept {
	m_cameraMatrices.view = view;
}

void CameraManager::SetProjectionMatrix(std::uint32_t width, std::uint32_t height) noexcept {

	m_cameraMatrices.projection = DirectX::XMMatrixPerspectiveFovLH(
		DirectX::XMConvertToRadians(m_fov),
		static_cast<float>(width) / static_cast<float>(height),
		0.1f, 100.f
	);
}

void CameraManager::SetCamera(const CameraMatrices& camera) noexcept {
	m_cameraMatrices = camera;
}

void CameraManager::SetFov(std::uint32_t fovAngleInDegree) noexcept {
	m_fov = static_cast<float>(fovAngleInDegree);
}
