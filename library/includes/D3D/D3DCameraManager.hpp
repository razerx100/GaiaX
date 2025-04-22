#ifndef D3D_CAMERA_MANAGER_HPP_
#define D3D_CAMERA_MANAGER_HPP_
#include <cstdint>
#include <memory>
#include <vector>
#include <D3DResources.hpp>
#include <D3DDescriptorHeapManager.hpp>

#include <Camera.hpp>
#include <DirectXMath.h>

class CameraManager
{
public:
	CameraManager(ID3D12Device* device, MemoryManager* memoryManager);

	[[nodiscard]]
	std::uint32_t AddCamera(std::shared_ptr<Camera> camera) noexcept;

	void SetCamera(std::uint32_t index) noexcept { m_activeCameraIndex = index; }

	void RemoveCamera(std::uint32_t index) noexcept;

	void CreateBuffer(std::uint32_t frameCount);

	void Update(UINT64 index) const noexcept;

	void SetDescriptorLayoutGraphics(
		std::vector<D3DDescriptorManager>& descriptorManagers, size_t cameraRegister,
		size_t registerSpace, D3D12_SHADER_VISIBILITY shaderStage
	) const noexcept;
	void SetDescriptorLayoutCompute(
		std::vector<D3DDescriptorManager>& descriptorManagers, size_t cameraRegister,
		size_t registerSpace
	) const noexcept;

	void SetDescriptorGraphics(
		std::vector<D3DDescriptorManager>& descriptorManagers, size_t cameraRegister,
		size_t registerSpace
	) const;
	void SetDescriptorCompute(
		std::vector<D3DDescriptorManager>& descriptorManagers, size_t cameraRegister,
		size_t registerSpace
	) const;

private:
	// Not actually using this. Just using it to get the struct size.
	struct CameraBufferData
	{
		DirectX::XMMATRIX view;
		DirectX::XMMATRIX projection;
		Frustum           viewFrustum;
		DirectX::XMFLOAT4 viewPosition;
	};

private:
	size_t                               m_activeCameraIndex;
	UINT64                               m_cameraBufferInstanceSize;
	Buffer                               m_cameraBuffer;
	std::vector<std::shared_ptr<Camera>> m_cameras;

public:
	CameraManager(const CameraManager&) = delete;
	CameraManager& operator=(const CameraManager&) = delete;

	CameraManager(CameraManager&& other) noexcept
		: m_activeCameraIndex{ other.m_activeCameraIndex },
		m_cameraBufferInstanceSize{ other.m_cameraBufferInstanceSize },
		m_cameraBuffer{ std::move(other.m_cameraBuffer) },
		m_cameras{ std::move(other.m_cameras) }
	{}
	CameraManager& operator=(CameraManager&& other) noexcept
	{
		m_activeCameraIndex        = other.m_activeCameraIndex;
		m_cameraBufferInstanceSize = other.m_cameraBufferInstanceSize;
		m_cameraBuffer             = std::move(other.m_cameraBuffer);
		m_cameras                  = std::move(other.m_cameras);

		return *this;
	}
};
#endif
