#include <DeviceManager.hpp>
#include <Exception.hpp>

DeviceManager::DeviceManager() {
    std::uint32_t dxgiFactoryFlags = 0;

#ifdef _DEBUG
    {
        ComPtr<ID3D12Debug> debugController;
        D3D12GetDebugInterface(__uuidof(ID3D12Debug), &debugController);
        debugController->EnableDebugLayer();

        dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
    }
#endif

    ComPtr<IDXGIFactory4> factory;
    CreateDXGIFactory2(dxgiFactoryFlags, __uuidof(IDXGIFactory4), &factory);

    {
        ComPtr<IDXGIAdapter1> adapter;

        GetHardwareAdapter(factory.Get(), &adapter);

        D3D12CreateDevice(
            adapter.Get(),
            D3D_FEATURE_LEVEL_11_0,
            __uuidof(ID3D12Device5),
            &m_pDevice
        );
    }
}

ID3D12Device5* DeviceManager::GetDeviceRef() const noexcept {
	return m_pDevice.Get();
}

void DeviceManager::GetHardwareAdapter(
	IDXGIFactory1* pFactory,
	IDXGIAdapter1** ppAdapter
) {
    ComPtr<IDXGIFactory6> pFactory6;

    if (SUCCEEDED(pFactory->QueryInterface(__uuidof(IDXGIFactory6), &pFactory6)))
        pFactory6->EnumAdapterByGpuPreference(
            0u,
            DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
            __uuidof(IDXGIAdapter1),
            reinterpret_cast<void**>(ppAdapter)
        );
    else
        pFactory->EnumAdapters1(
            0u, ppAdapter
        );

    if (FAILED(
        D3D12CreateDevice(
            *ppAdapter, D3D_FEATURE_LEVEL_11_0,
            __uuidof(ID3D12Device), nullptr)
    ))
        throw GenericException(
            __LINE__, __FILE__,
            "GPU doesn't have hardware support for DirectX12."
        );
}
