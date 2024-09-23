#include <D3DCommandQueue.hpp>

// D3D CommandList
D3DCommandList::D3DCommandList(ID3D12Device4* device, D3D12_COMMAND_LIST_TYPE type) : D3DCommandList{}
{
	Create(device, type);
}

void D3DCommandList::Create(ID3D12Device4* device, D3D12_COMMAND_LIST_TYPE type)
{
	device->CreateCommandAllocator(type, IID_PPV_ARGS(&m_commandAllocator));

	ComPtr<ID3D12GraphicsCommandList> commandList{};

	device->CreateCommandList1(
		0u, type, D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(&m_commandList)
	);
}

void D3DCommandList::Reset() const
{
	m_commandAllocator->Reset();
	m_commandList->Reset(m_commandAllocator.Get(), nullptr);
}

void D3DCommandList::Close() const
{
	m_commandList->Close();
}

void D3DCommandList::Copy(
	const Buffer& src, UINT64 srcOffset, const Buffer& dst, UINT64 dstOffset, UINT64 size
) const noexcept {
	m_commandList->CopyBufferRegion(dst.Get(), dstOffset, src.Get(), srcOffset, size);
}

void D3DCommandList::Copy(
	const Buffer& src, UINT64 srcOffset, const Texture& dst, UINT subresourceIndex
) const noexcept {
	D3D12_TEXTURE_COPY_LOCATION dstLocation
	{
		.pResource        = dst.Get(),
		.Type             = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
		.SubresourceIndex = subresourceIndex
	};

	D3D12_SUBRESOURCE_FOOTPRINT srcFootprint
	{
		.Format   = dst.Format(),
		.Width    = static_cast<UINT>(dst.GetWidth()),
		.Height   = dst.GetHeight(),
		.Depth    = dst.GetDepth(),
		.RowPitch = static_cast<UINT>(dst.GetRowPitchD3DAligned())
	};

	D3D12_PLACED_SUBRESOURCE_FOOTPRINT srcPlacedFootprint
	{
		.Offset    = srcOffset,
		.Footprint = srcFootprint
	};

	D3D12_TEXTURE_COPY_LOCATION srcLocation
	{
		.pResource       = src.Get(),
		.Type            = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT,
		.PlacedFootprint = srcPlacedFootprint
	};

	m_commandList->CopyTextureRegion(
		&dstLocation,
		0u, 0u, 0u,
		&srcLocation,
		nullptr
	);
}

// D3D Command Queue
void D3DCommandQueue::Create(ID3D12Device4* device, D3D12_COMMAND_LIST_TYPE type, size_t bufferCount)
{
	D3D12_COMMAND_QUEUE_DESC queueDesc
	{
		.Type  = type,
		.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE
	};

	device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue));

	for (size_t index = 0u; index < bufferCount; ++index)
		m_commandLists.emplace_back(device, type);
}

void D3DCommandQueue::Signal(ID3D12Fence* fence, UINT64 signalValue) const noexcept
{
	m_commandQueue->Signal(fence, signalValue);
}

void D3DCommandQueue::Wait(ID3D12Fence* fence, UINT64 waitValue) const noexcept
{
	m_commandQueue->Wait(fence, waitValue);
}

void D3DCommandQueue::ExecuteCommandLists(
	ID3D12CommandList* const * commandLists, UINT commandListCount
) const noexcept {
	m_commandQueue->ExecuteCommandLists(commandListCount, commandLists);
}

void D3DCommandQueue::ExecuteCommandLists(
	ID3D12GraphicsCommandList* commandList
) const noexcept {
	ID3D12CommandList* const commandLists{ commandList };

	ExecuteCommandLists(&commandLists, 1u);
}
