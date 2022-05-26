#include <cstring>
#include <CameraManager.hpp>

CameraManager::CameraManager() noexcept
	: m_cameraMatrices{}, m_fovRadian(DirectX::XMConvertToRadians(65.f)),
	m_sceneWidth(0), m_sceneHeight(0) {}

void CameraManager::CopyData(std::uint8_t* cpuHandle) noexcept {
	memcpy(cpuHandle, &m_cameraMatrices, sizeof(CameraMatrices));
}

void CameraManager::SetViewMatrix(const DirectX::XMMATRIX& view) noexcept {
	m_cameraMatrices.view = view;
}

void CameraManager::SetProjectionMatrix() noexcept {
	m_cameraMatrices.projection = DirectX::XMMatrixPerspectiveFovLH(
		m_fovRadian,
		m_sceneWidth / m_sceneHeight,
		0.1f, 100.f
	);
}

void CameraManager::SetCamera(const CameraMatrices& camera) noexcept {
	m_cameraMatrices = camera;
}

void CameraManager::SetFov(std::uint32_t fovAngleInDegree) noexcept {
	m_fovRadian = DirectX::XMConvertToRadians(static_cast<float>(fovAngleInDegree));

	SetProjectionMatrix();
}

void CameraManager::SetSceneResolution(std::uint32_t width, std::uint32_t height) noexcept {
	m_sceneWidth = static_cast<float>(width);
	m_sceneHeight = static_cast<float>(height);

	SetProjectionMatrix();
}
