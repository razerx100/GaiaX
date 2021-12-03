#ifndef __INSTANCE_MANAGER_HPP__
#define __INSTANCE_MANAGER_HPP__
#include <ObjectManager.hpp>
#include <IDeviceManager.hpp>
#include <ISwapChainManager.hpp>
#include <IGraphicsQueueManager.hpp>
#include <ICommandListManager.hpp>
#include <DebugInfoManager.hpp>

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
		ID3D12Device5* device,
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
#endif
