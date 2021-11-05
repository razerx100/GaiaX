#ifndef __DEVICE_MANAGER_HPP__
#define __DEVICE_MANAGER_HPP__
#include <D3DHeaders.hpp>

class DeviceManager {
public:
	DeviceManager();

	ID3D12Device5* GetDeviceRef() const noexcept;
	IDXGIFactory4* GetFactoryRef() const noexcept;

private:
	void GetHardwareAdapter(
		IDXGIFactory1* pFactory,
		IDXGIAdapter1** ppAdapter
	);

private:
	ComPtr<ID3D12Device5> m_pDevice;
    ComPtr<IDXGIFactory4> m_pFactory;
};

DeviceManager* GetD3DDeviceInstance() noexcept;
void InitD3DDeviceInstance();
void CleanUpD3DDeviceInstance() noexcept;
#endif
