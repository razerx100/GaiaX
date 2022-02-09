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
#include <IViewportAndScissorManager.hpp>
#include <IHeapManager.hpp>

class DeviceInst : public _ObjectManager<IDeviceManager, DeviceInst> {
public:
	static void Init();
};

class SwapchainInst : public _ObjectManager<ISwapChainManager, SwapchainInst> {
public:
	static void Init(
		IDXGIFactory4* factory, ID3D12CommandQueue* cmdQueue, void* windowHandle,
		size_t bufferCount,
		std::uint32_t width, std::uint32_t height,
		bool variableRefreshRateAvailable = true
	);
};

class GfxQueInst : public _ObjectManager<IGraphicsQueueManager, GfxQueInst> {
public:
	static void Init(
		ID3D12Device* device,
		size_t bufferCount
	);
};

class GfxCmdListInst : public _ObjectManager<ICommandListManager, GfxCmdListInst> {
public:
	static void Init(
		ID3D12Device5* device,
		size_t bufferCount
	);
};

class DebugInfoInst : public _ObjectManager<DebugInfoManager, DebugInfoInst> {
public:
	static void Init();
};

class DepthBuffInst : public _ObjectManager<IDepthBuffer, DepthBuffInst> {
public:
	static void Init(ID3D12Device* device);
};

class ModelContainerInst : public _ObjectManager<IModelContainer, ModelContainerInst> {
public:
	static void Init(const char* shaderPath);
};

class CpyQueInst : public _ObjectManager<ICopyQueueManager, CpyQueInst> {
public:
	static void Init(
		ID3D12Device* device
	);
};

class CpyCmdListInst : public _ObjectManager<ICommandListManager, CpyCmdListInst> {
public:
	static void Init(
		ID3D12Device5* device
	);
};

class VertexBufferInst : public _ObjectManager<IResourceBuffer, VertexBufferInst> {
public:
	static void Init();
};

class IndexBufferInst : public _ObjectManager<IResourceBuffer, IndexBufferInst> {
public:
	static void Init();
};

class ViewPAndScsrInst : public _ObjectManager<IViewportAndScissorManager, ViewPAndScsrInst> {
public:
	static void Init(std::uint32_t width, std::uint32_t height);
};

class HeapManagerInst : public _ObjectManager<IHeapManager, HeapManagerInst> {
public:
	static void Init();
};
#endif
