#ifndef CAMERA_MANAGER_HPP_
#define CAMERA_MANAGER_HPP_
#include <cstdint>
#include <DirectXMath.h>
#include <ISharedDataContainer.hpp>

struct CameraMatrices {
	DirectX::XMMATRIX view;
	DirectX::XMMATRIX projection;
};

class CameraManager {
public:
	CameraManager(ISharedDataContainer& sharedContainer) noexcept;

	void CopyData(std::uint8_t* cpuHandle) noexcept;

	void SetCamera(const CameraMatrices& camera) noexcept;
	void SetSceneResolution(std::uint32_t width, std::uint32_t height) noexcept;

private:
	void SetProjectionMatrix() noexcept;
	void FetchCameraData() noexcept;

private:
	CameraMatrices m_cameraMatrices;
	float m_fovRadian;
	float m_sceneWidth;
	float m_sceneHeight;
	ISharedDataContainer& m_sharedData;
};
#endif
