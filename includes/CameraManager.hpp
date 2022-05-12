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
	void CopyData(std::uint8_t* cpuHandle) noexcept;

	void SetViewMatrix(const DirectX::XMMATRIX& view) noexcept;
	void SetProjectionMatrix(const DirectX::XMMATRIX& projection) noexcept;
	void SetCamera(const CameraMatrices& camera) noexcept;

private:
	CameraMatrices m_cameraMatrices;
};
#endif
