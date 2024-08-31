#include <D3DDescriptorLayout.hpp>

void D3DDescriptorMap::Bind(
	const D3DDescriptorHeap& descriptorHeap, ID3D12GraphicsCommandList* commandList
) const {
	for (const auto& viewMap : m_singleDescriptors)
		viewMap.bindViewFunction(commandList, viewMap.rootIndex, viewMap.bufferAddress);

	for (const auto& tableMap : m_descriptorTables)
		tableMap.bindTableFunction(
			commandList, tableMap.rootIndex, descriptorHeap.GetGPUHandle(tableMap.descriptorIndex)
		);
}

D3DDescriptorMap& D3DDescriptorMap::AddCBVGfx(
	UINT rootIndex, D3D12_GPU_VIRTUAL_ADDRESS bufferAddress
) noexcept {
	m_singleDescriptors.emplace_back(
		SingleDescriptorMap{
			.rootIndex        = rootIndex,
			.bufferAddress    = bufferAddress,
			.bindViewFunction = &ProxyView<&ID3D12GraphicsCommandList::SetGraphicsRootConstantBufferView>
		}
	);

	return *this;
}

D3DDescriptorMap& D3DDescriptorMap::AddCBVCom(
	UINT rootIndex, D3D12_GPU_VIRTUAL_ADDRESS bufferAddress
) noexcept {
	m_singleDescriptors.emplace_back(
		SingleDescriptorMap{
			.rootIndex        = rootIndex,
			.bufferAddress    = bufferAddress,
			.bindViewFunction = &ProxyView<&ID3D12GraphicsCommandList::SetComputeRootConstantBufferView>
		}
	);

	return *this;
}

D3DDescriptorMap& D3DDescriptorMap::AddUAVGfx(
	UINT rootIndex, D3D12_GPU_VIRTUAL_ADDRESS bufferAddress
) noexcept {
	m_singleDescriptors.emplace_back(
		SingleDescriptorMap{
			.rootIndex        = rootIndex,
			.bufferAddress    = bufferAddress,
			.bindViewFunction = &ProxyView<&ID3D12GraphicsCommandList::SetGraphicsRootUnorderedAccessView>
		}
	);

	return *this;
}

D3DDescriptorMap& D3DDescriptorMap::AddUAVCom(
	UINT rootIndex, D3D12_GPU_VIRTUAL_ADDRESS bufferAddress
) noexcept {
	m_singleDescriptors.emplace_back(
		SingleDescriptorMap{
			.rootIndex        = rootIndex,
			.bufferAddress    = bufferAddress,
			.bindViewFunction = &ProxyView<&ID3D12GraphicsCommandList::SetComputeRootUnorderedAccessView>
		}
	);

	return *this;
}

D3DDescriptorMap& D3DDescriptorMap::AddSRVGfx(
	UINT rootIndex, D3D12_GPU_VIRTUAL_ADDRESS bufferAddress
) noexcept {
	m_singleDescriptors.emplace_back(
		SingleDescriptorMap{
			.rootIndex        = rootIndex,
			.bufferAddress    = bufferAddress,
			.bindViewFunction = &ProxyView<&ID3D12GraphicsCommandList::SetGraphicsRootShaderResourceView>
		}
	);

	return *this;
}

D3DDescriptorMap& D3DDescriptorMap::AddSRVCom(
	UINT rootIndex, D3D12_GPU_VIRTUAL_ADDRESS bufferAddress
) noexcept {
	m_singleDescriptors.emplace_back(
		SingleDescriptorMap{
			.rootIndex        = rootIndex,
			.bufferAddress    = bufferAddress,
			.bindViewFunction = &ProxyView<&ID3D12GraphicsCommandList::SetComputeRootShaderResourceView>
		}
	);

	return *this;
}

D3DDescriptorMap& D3DDescriptorMap::AddDescTableGfx(UINT rootIndex, UINT descriptorIndex) noexcept
{
	m_descriptorTables.emplace_back(
		DescriptorTableMap{
			.rootIndex         = rootIndex,
			.descriptorIndex   = descriptorIndex,
			.bindTableFunction = &ProxyTable<&ID3D12GraphicsCommandList::SetGraphicsRootDescriptorTable>
		}
	);

	return *this;
}

D3DDescriptorMap& D3DDescriptorMap::AddDescTableCom(UINT rootIndex, UINT descriptorIndex) noexcept
{
	m_descriptorTables.emplace_back(
		DescriptorTableMap{
			.rootIndex         = rootIndex,
			.descriptorIndex   = descriptorIndex,
			.bindTableFunction = &ProxyTable<&ID3D12GraphicsCommandList::SetComputeRootDescriptorTable>
		}
	);

	return *this;
}
