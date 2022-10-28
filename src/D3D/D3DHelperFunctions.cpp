#include <D3DHelperFunctions.hpp>
#include <cassert>

Resolution GetDisplayResolution(
	ID3D12Device* gpu, IDXGIFactory1* factory, std::uint32_t displayIndex
) {
	LUID gpuLUid = gpu->GetAdapterLuid();

	ComPtr<IDXGIAdapter1> adapter;
	DXGI_ADAPTER_DESC gpuDesc = {};

	bool adapterMatched = false;

	for (UINT index = 0u; factory->EnumAdapters1(index, &adapter) != DXGI_ERROR_NOT_FOUND;) {

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

	ComPtr<IDXGIOutput> pDisplayOutput;
	assert(
		FAILED(adapter->EnumOutputs(displayIndex, &pDisplayOutput)) && "Invalid display index."
	);

	DXGI_OUTPUT_DESC displayData = {};
	pDisplayOutput->GetDesc(&displayData);

	return {
		static_cast<std::uint64_t>(displayData.DesktopCoordinates.right),
		static_cast<std::uint64_t>(displayData.DesktopCoordinates.bottom)
	};
}

D3D12_RESOURCE_BARRIER GetTransitionBarrier(
	ID3D12Resource* resource, D3D12_RESOURCE_STATES beforeState,
	D3D12_RESOURCE_STATES afterState
) noexcept {
	D3D12_RESOURCE_BARRIER transitionBarrier{};
	transitionBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	transitionBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	transitionBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	transitionBarrier.Transition.pResource = resource;
	transitionBarrier.Transition.StateBefore = beforeState;
	transitionBarrier.Transition.StateAfter = afterState;

	return transitionBarrier;
}
