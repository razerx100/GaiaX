#ifndef DEVICE_MANAGER_HPP_
#define DEVICE_MANAGER_HPP_
#include <D3DHeaders.hpp>

class DeviceManager {
public:
	DeviceManager();

	[[nodiscard]]
	ID3D12Device5* GetDeviceRef() const noexcept;
	[[nodiscard]]
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
#endif
