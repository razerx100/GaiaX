#ifndef __DEVICE_MANAGER_HPP__
#define __DEVICE_MANAGER_HPP__
#include <IDeviceManager.hpp>

class DeviceManager : public IDeviceManager {
public:
	DeviceManager();

	ID3D12Device5* GetDeviceRef() const noexcept override;
	IDXGIFactory4* GetFactoryRef() const noexcept override;

private:
	void GetHardwareAdapter(
		IDXGIFactory1* pFactory,
		IDXGIAdapter1** ppAdapter
	);

private:
	ComPtr<ID3D12Device5> m_pDevice;
    ComPtr<IDXGIFactory4> m_pFactory;
};
#endif
