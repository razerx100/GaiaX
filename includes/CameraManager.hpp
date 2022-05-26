#ifndef CAMERA_MANAGER_HPP_
#define CAMERA_MANAGER_HPP_
#include <cstdint>
#include <DirectXMath.h>

struct CameraMatrices {
	DirectX::XMMATRIX view;
	DirectX::XMMATRIX projection;
};

class CameraManager {
public:
	CameraManager() noexcept;

	void CopyData(std::uint8_t* cpuHandle) noexcept;

	void SetViewMatrix(const DirectX::XMMATRIX& view) noexcept;
	void SetCamera(const CameraMatrices& camera) noexcept;
	void SetFov(std::uint32_t fovAngleInDegree) noexcept;
	void SetSceneResolution(std::uint32_t width, std::uint32_t height) noexcept;

private:
	void SetProjectionMatrix() noexcept;

private:
	CameraMatrices m_cameraMatrices;
	float m_fovRadian;
	float m_sceneWidth;
	float m_sceneHeight;
};
#endif
