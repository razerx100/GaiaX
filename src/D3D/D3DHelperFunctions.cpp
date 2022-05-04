#include <D3DHelperFunctions.hpp>
#include <D3DThrowMacros.hpp>

Resolution GetDisplayResolution(
	ID3D12Device* gpu, IDXGIFactory1* factory,
	std::uint32_t displayIndex
) {
	LUID gpuLUid = gpu->GetAdapterLuid();

	ComPtr<IDXGIAdapter1> adapter;
	DXGI_ADAPTER_DESC gpuDesc = {};

	bool adapterMatched = false;

	for (UINT index = 0u;
		factory->EnumAdapters1(index, &adapter) != DXGI_ERROR_NOT_FOUND;) {

		adapter->GetDesc(&gpuDesc);

		const LUID& lUid1 = gpuDesc.AdapterLuid;
		const LUID& lUid2 = gpuLUid;

		if (lUid1.HighPart == lUid2.HighPart && lUid1.LowPart == lUid2.LowPart) {
			adapterMatched = true;
			break;
		}
	}

	if (!adapterMatched)
		D3D_GENERIC_THROW("GPU ID doesn't match.");

	adapter->GetDesc(&gpuDesc);

	ComPtr<IDXGIOutput> pDisplayOutput;
	if (FAILED(adapter->EnumOutputs(displayIndex, &pDisplayOutput)))
		D3D_GENERIC_THROW("Searched GPU couldn't be found.");

	DXGI_OUTPUT_DESC displayData = {};
	pDisplayOutput->GetDesc(&displayData);

	return {
		static_cast<std::uint64_t>(displayData.DesktopCoordinates.right),
		static_cast<std::uint64_t>(displayData.DesktopCoordinates.bottom)
	};
}
