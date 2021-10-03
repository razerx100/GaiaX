#ifndef __DEVICE_MANAGER_HPP__
#define __DEVICE_MANAGER_HPP__
#include <D3DHeaders.hpp>

class DeviceManager {
public:
	DeviceManager();

	ID3D12Device5* GetDeviceRef() const noexcept;

private:
	void GetHardwareAdapter(
		IDXGIFactory1* pFactory,
		IDXGIAdapter1** ppAdapter
	);

private:
	ComPtr<ID3D12Device5> m_pDevice;
};

DeviceManager* GetD3DDeviceInstance() noexcept;
void InitD3DDeviceInstance();
void CleanUpD3DDeviceInstance() noexcept;
#endif
