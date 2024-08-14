#ifndef DEVICE_MANAGER_HPP_
#define DEVICE_MANAGER_HPP_
#include <D3DHeaders.hpp>
#include <utility>

class DeviceManager
{
public:
	DeviceManager() : m_device{}, m_adapter{}, m_factory{} {}

	void Create(D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_12_0);

	[[nodiscard]]
	ID3D12Device5* GetDevice() const noexcept { return m_device.Get(); }
	[[nodiscard]]
	IDXGIFactory4* GetFactory() const noexcept { return m_factory.Get(); }
	[[nodiscard]]
	IDXGIAdapter3* GetAdapter() const noexcept { return m_adapter.Get(); }

	[[nodiscard]]
	DXGI_ADAPTER_DESC1 GetAdapterDesc() const noexcept;

private:
	static void GetHardwareAdapter(
		IDXGIFactory1* pFactory, IDXGIAdapter1** ppAdapter, D3D_FEATURE_LEVEL featureLevel
	);

private:
	ComPtr<ID3D12Device5> m_device;
	ComPtr<IDXGIAdapter3> m_adapter;
    ComPtr<IDXGIFactory4> m_factory;

public:
	DeviceManager(const DeviceManager&) = delete;
	DeviceManager& operator=(const DeviceManager&) = delete;

	DeviceManager(DeviceManager&& other) noexcept
		: m_device{ std::move(other.m_device) },
		m_adapter{ std::move(other.m_adapter) },
		m_factory{ std::move(other.m_factory) }
	{}
	DeviceManager& operator=(DeviceManager&& other) noexcept
	{
		m_device  = std::move(other.m_device);
		m_adapter = std::move(other.m_adapter);
		m_factory = std::move(other.m_factory);
	}
};
#endif
