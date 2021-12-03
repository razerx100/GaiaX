#ifndef __I_DEVICE_MANAGER_HPP__
#define __I_DEVICE_MANAGER_HPP__
#include <D3DHeaders.hpp>

class IDeviceManager {
public:
	virtual ~IDeviceManager() = default;

	virtual ID3D12Device5* GetDeviceRef() const noexcept = 0;
	virtual IDXGIFactory4* GetFactoryRef() const noexcept = 0;
};

IDeviceManager* CreateD3DDeviceInstance();

#endif