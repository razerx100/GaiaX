#include <DeviceManager.hpp>
#include <Exception.hpp>


void DeviceManager::Create(D3D_FEATURE_LEVEL featureLevel /* = D3D_FEATURE_LEVEL_12_0 */)
{
    UINT dxgiFactoryFlags = 0u;

#ifdef _DEBUG
    {
        ComPtr<ID3D12Debug> debugController{};
        D3D12GetDebugInterface(IID_PPV_ARGS(&debugController));

        debugController->EnableDebugLayer();

        dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
    }
#endif

    CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&m_factory));

    {
        ComPtr<IDXGIAdapter1> adapter{};
        GetHardwareAdapter(m_factory.Get(), &adapter, featureLevel);

        adapter->QueryInterface(IID_PPV_ARGS(&m_adapter));

        D3D12CreateDevice(m_adapter.Get(), featureLevel,IID_PPV_ARGS(&m_device));

#ifdef _DEBUG
        m_debugLogger.CreateInfoQueue(m_device.Get());
#endif
    }
}

DXGI_ADAPTER_DESC1 DeviceManager::GetAdapterDesc() const noexcept
{
    DXGI_ADAPTER_DESC1 adapterDescription{};

    if (m_adapter)
        m_adapter->GetDesc1(&adapterDescription);

    return adapterDescription;
}

void DeviceManager::GetHardwareAdapter(
    IDXGIFactory1* pFactory, IDXGIAdapter1** ppAdapter, D3D_FEATURE_LEVEL featureLevel
) {
    ComPtr<IDXGIFactory6> factory6{};
    bool found = false;

    if (SUCCEEDED(pFactory->QueryInterface(IID_PPV_ARGS(&factory6))))
    {
        for (UINT index = 0u;
            SUCCEEDED(factory6->EnumAdapterByGpuPreference(
                index, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(ppAdapter)
            ));
            ++index)
        {
            if (SUCCEEDED(
                D3D12CreateDevice(
                    *ppAdapter, featureLevel, __uuidof(ID3D12Device), nullptr
                )))
            {
                found = true;
                break;
            }

        }
    } else {
        for (UINT index = 0u;
            SUCCEEDED(pFactory->EnumAdapters1(index, ppAdapter));
            ++index)
        {
            if (SUCCEEDED(
                D3D12CreateDevice(
                    *ppAdapter, featureLevel,__uuidof(ID3D12Device), nullptr
                )))
            {
                found = true;
                break;
            }
        }
    }

    if (!found)
        throw Exception(
            "D3D12 Feature Error", "None of the GPUs has required D3D12 feature support."
        );
}
