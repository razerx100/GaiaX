#ifndef __INSTANCE_MANAGER_HPP__
#define __INSTANCE_MANAGER_HPP__
#include <ObjectManager.hpp>
#include <IDeviceManager.hpp>
#include <ISwapChainManager.hpp>
#include <IGraphicsQueueManager.hpp>
#include <ICommandListManager.hpp>
#include <DebugInfoManager.hpp>
#include <IDepthBuffer.hpp>
#include <IModelContainer.hpp>
#include <ICopyQueueManager.hpp>
#include <IResourceBuffer.hpp>

class DeviceInst : public _ObjectManager<IDeviceManager> {
public:
	static void Init();
};

class SwapchainInst : public _ObjectManager<ISwapChainManager> {
public:
	static void Init(
		IDXGIFactory4* factory, ID3D12CommandQueue* cmdQueue, void* windowHandle,
		std::uint8_t bufferCount,
		std::uint32_t width, std::uint32_t height,
		bool variableRefreshRateAvailable = true
	);
};

class GfxQueInst : public _ObjectManager<IGraphicsQueueManager> {
public:
	static void Init(
		ID3D12Device* device,
		std::uint8_t bufferCount
	);
};

class GfxCmdListInst : public _ObjectManager<ICommandListManager> {
public:
	static void Init(
		ID3D12Device5* device,
		std::uint8_t bufferCount
	);
};

class DebugInfoInst : public _ObjectManager<DebugInfoManager> {
public:
	static void Init();
};

class DepthBuffInst : public _ObjectManager<IDepthBuffer> {
public:
	static void Init(ID3D12Device* device);
};

class ModelContainerInst : public _ObjectManager<IModelContainer> {
public:
	static void Init(const char* shaderPath);
};

class CpyQueInst : public _ObjectManager<ICopyQueueManager> {
public:
	static void Init(
		ID3D12Device* device
	);
};

class CpyCmdListInst : public _ObjectManager<ICommandListManager> {
public:
	static void Init(
		ID3D12Device5* device
	);
};

class VertexBufferInst : public _ObjectManager<IResourceBuffer> {
public:
	static void Init();
};

class IndexBufferInst : public _ObjectManager<IResourceBuffer> {
public:
	static void Init();
};
#endif
