#include <DeviceManager.hpp>
#include <Exception.hpp>
#include <cassert>

void DeviceManager::Create(D3D_FEATURE_LEVEL featureLevel /* = D3D_FEATURE_LEVEL_12_0 */)
{
    UINT dxgiFactoryFlags = 0u;

#ifdef _DEBUG
    {
        ComPtr<ID3D12Debug5> debugController{};
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

DeviceManager::Resolution DeviceManager::GetDisplayResolution(UINT displayIndex) const
{
    ComPtr<IDXGIAdapter1> adapter{};
	DXGI_ADAPTER_DESC gpuDesc{};

	LUID gpuLUid        = m_device->GetAdapterLuid();
	bool adapterMatched = false;

	for (UINT index = 0u; m_factory->EnumAdapters1(index, &adapter) != DXGI_ERROR_NOT_FOUND;)
    {
		adapter->GetDesc(&gpuDesc);

		const LUID& lUid1 = gpuDesc.AdapterLuid;
		const LUID& lUid2 = gpuLUid;

		if (lUid1.HighPart == lUid2.HighPart && lUid1.LowPart == lUid2.LowPart) {
			adapterMatched = true;
			break;
		}
	}

	assert(adapterMatched && "GPU IDs don't match.");

	adapter->GetDesc(&gpuDesc);

    ComPtr<IDXGIOutput> displayOutput{};
	[[maybe_unused]] HRESULT displayCheck = adapter->EnumOutputs(displayIndex, &displayOutput);
	assert(SUCCEEDED(displayCheck) && "Invalid display index.");

	DXGI_OUTPUT_DESC displayData{};
	displayOutput->GetDesc(&displayData);

	return Resolution
    {
		.width  = static_cast<UINT>(displayData.DesktopCoordinates.right),
		.height = static_cast<UINT>(displayData.DesktopCoordinates.bottom)
	};
}
