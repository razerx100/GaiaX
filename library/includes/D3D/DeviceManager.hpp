#ifndef DEVICE_MANAGER_HPP_
#define DEVICE_MANAGER_HPP_
#include <D3DHeaders.hpp>
#include <utility>
#include <D3DDebugLogger.hpp>

class DeviceManager
{
public:
	DeviceManager() : m_device{}, m_adapter{}, m_factory{}, m_debugLogger{} {}

	void Create(D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_12_0);

	[[nodiscard]]
	ID3D12Device5* GetDevice() const noexcept { return m_device.Get(); }
	[[nodiscard]]
	IDXGIFactory5* GetFactory() const noexcept { return m_factory.Get(); }
	[[nodiscard]]
	IDXGIAdapter3* GetAdapter() const noexcept { return m_adapter.Get(); }
	[[nodiscard]]
	D3DDebugLogger& GetDebugLogger() noexcept { return m_debugLogger; }

	[[nodiscard]]
	DXGI_ADAPTER_DESC1 GetAdapterDesc() const noexcept;

private:
	static void GetHardwareAdapter(
		IDXGIFactory1* pFactory, IDXGIAdapter1** ppAdapter, D3D_FEATURE_LEVEL featureLevel
	);

private:
	ComPtr<ID3D12Device5> m_device;
	ComPtr<IDXGIAdapter3> m_adapter;
    ComPtr<IDXGIFactory5> m_factory;
	D3DDebugLogger        m_debugLogger;

public:
	DeviceManager(const DeviceManager&) = delete;
	DeviceManager& operator=(const DeviceManager&) = delete;

	DeviceManager(DeviceManager&& other) noexcept
		: m_device{ std::move(other.m_device) },
		m_adapter{ std::move(other.m_adapter) },
		m_factory{ std::move(other.m_factory) },
		m_debugLogger{ std::move(other.m_debugLogger) }
	{}
	DeviceManager& operator=(DeviceManager&& other) noexcept
	{
		m_device      = std::move(other.m_device);
		m_adapter     = std::move(other.m_adapter);
		m_factory     = std::move(other.m_factory);
		m_debugLogger = std::move(other.m_debugLogger);
	}
};
#endif
