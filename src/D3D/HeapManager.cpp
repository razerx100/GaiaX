#include <HeapManager.hpp>
#include <d3dx12.h>
#include <D3DHeap.hpp>
#include <D3DThrowMacros.hpp>
#include <UploadBuffer.hpp>
#include <D3DHelperFunctions.hpp>

HeapManager::HeapManager()
	: m_currentMemoryOffset(0u), m_maxAlignment(64_KB) {

	m_uploadHeap = std::make_unique<D3DHeap>();
	m_gpuHeap = std::make_unique<D3DHeap>();
}

void HeapManager::CreateBuffers(ID3D12Device* device, bool msaa) {
	size_t alignedSize = Align(m_currentMemoryOffset, m_maxAlignment);

	m_uploadHeap->CreateHeap(device, alignedSize, msaa, true);
	m_gpuHeap->CreateHeap(device, alignedSize, msaa);

	for (size_t index = 0u; index < std::size(m_bufferData); ++index) {
		UploadBufferShared& uploadBuffer = m_uploadBuffers[index];
		D3DBufferShared& gpuBuffer = m_gpuBuffers[index];
		BufferData& bufferData = m_bufferData[index];

		CreatePlacedResource(
			device, m_uploadHeap->GetHeap(),
			uploadBuffer->GetBuffer(), bufferData.offset,
			GetBufferDesc(bufferData.bufferSize),
			true
		);
		uploadBuffer->MapBuffer();

		CreatePlacedResource(
			device, m_gpuHeap->GetHeap(),
			gpuBuffer, bufferData.offset,
			GetResourceDesc(
				bufferData,
				bufferData.isTexture
			),
			false
		);
	}
}

void HeapManager::RecordUpload(ID3D12GraphicsCommandList* copyList) {
	for (size_t index = 0u; index < std::size(m_bufferData); ++index) {
		D3D12_RESOURCE_BARRIER activationBarriers[2] = {};

		D3D12_RESOURCE_BARRIER& barrierUploadBuffer = activationBarriers[0];
		D3D12_RESOURCE_BARRIER& barrierGPUBuffer = activationBarriers[1];

		ID3D12Resource* uploadBuffer = m_uploadBuffers[index]->GetBuffer()->Get();
		ID3D12Resource* gpuBuffer = m_gpuBuffers[index]->Get();

		barrierUploadBuffer.Type = D3D12_RESOURCE_BARRIER_TYPE_ALIASING;
		barrierUploadBuffer.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrierUploadBuffer.Aliasing.pResourceBefore = nullptr;
		barrierUploadBuffer.Aliasing.pResourceBefore = uploadBuffer;

		barrierGPUBuffer.Type = D3D12_RESOURCE_BARRIER_TYPE_ALIASING;
		barrierGPUBuffer.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrierGPUBuffer.Aliasing.pResourceBefore = nullptr;
		barrierGPUBuffer.Aliasing.pResourceBefore = gpuBuffer;

		copyList->ResourceBarrier(2u, activationBarriers);

		BufferData& bufferData = m_bufferData[index];

		if (bufferData.isTexture) {
			D3D12_TEXTURE_COPY_LOCATION dest = {};
			dest.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
			dest.pResource = gpuBuffer;
			dest.SubresourceIndex = 0u;

			D3D12_SUBRESOURCE_FOOTPRINT srcFootprint = {};
			srcFootprint.Depth = 1u;
			srcFootprint.Format = bufferData.textureFormat;
			srcFootprint.Width = static_cast<UINT>(bufferData.width);
			srcFootprint.Height = static_cast<UINT>(bufferData.height);
			srcFootprint.RowPitch = static_cast<UINT>(Align(bufferData.rowPitch, 256u));

			D3D12_PLACED_SUBRESOURCE_FOOTPRINT placedFootprint = {};
			placedFootprint.Offset = 0u;
			placedFootprint.Footprint = srcFootprint;

			D3D12_TEXTURE_COPY_LOCATION src = {};
			src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
			src.pResource = uploadBuffer;
			src.PlacedFootprint = placedFootprint;

			copyList->CopyTextureRegion(
				&dest,
				0u, 0u, 0u,
				&src,
				nullptr
			);
		}
		else
			copyList->CopyResource(
				gpuBuffer,
				uploadBuffer
			);
	}
}

void HeapManager::ReleaseUploadBuffer() {
	m_bufferData = std::vector<BufferData>();
	m_gpuBuffers = std::vector<D3DBufferShared>();
	m_uploadBuffers = std::vector<UploadBufferShared>();
	m_uploadHeap.reset();
}

BufferPair HeapManager::AddBuffer(
	size_t bufferSize
) {
	constexpr size_t alignment = 64_KB;

	m_currentMemoryOffset = Align(m_currentMemoryOffset, alignment);

	m_bufferData.emplace_back(
		false, bufferSize / 4u, 1u, alignment, m_currentMemoryOffset, bufferSize, bufferSize,
		DXGI_FORMAT_UNKNOWN
	);
	m_gpuBuffers.emplace_back(std::make_shared<D3DBuffer>());
	m_uploadBuffers.emplace_back(std::make_shared<UploadBuffer>());

	m_currentMemoryOffset += bufferSize;

	return { m_gpuBuffers.back(), m_uploadBuffers.back() };
}

BufferPair HeapManager::AddTexture(
	ID3D12Device* device,
	size_t width, size_t height, size_t pixelSizeInBytes,
	bool msaa
) {
	m_gpuBuffers.emplace_back(std::make_shared<D3DBuffer>());
	m_uploadBuffers.emplace_back(std::make_shared<UploadBuffer>());

	DXGI_FORMAT textureFormat = DXGI_FORMAT_UNKNOWN;

	if (pixelSizeInBytes == 16u)
		textureFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;
	else if (pixelSizeInBytes == 4u)
		textureFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

	D3D12_RESOURCE_ALLOCATION_INFO allocationInfo = {};

	if (msaa) {
		allocationInfo = GetAllocationInfo(device, height, width, 64_KB, textureFormat);
		if (!CheckIfAlignmentPossible(allocationInfo.SizeInBytes))
			allocationInfo = GetAllocationInfo(device, height, width, 4_MB, textureFormat);
	}
	else {
		allocationInfo = GetAllocationInfo(device, height, width, 4_KB, textureFormat);
		if (!CheckIfAlignmentPossible(allocationInfo.SizeInBytes))
			allocationInfo = GetAllocationInfo(device, height, width, 64_KB, textureFormat);
	}

	const auto& [bufferSize, alignment] = allocationInfo;

	m_maxAlignment = std::max(m_maxAlignment, static_cast<size_t>(alignment));

	m_currentMemoryOffset = Align(m_currentMemoryOffset, alignment);

	m_bufferData.emplace_back(
		true,
		width, height, alignment, m_currentMemoryOffset,
		width * pixelSizeInBytes, bufferSize, textureFormat
	);

	m_currentMemoryOffset += bufferSize;

	return { m_gpuBuffers.back(), m_uploadBuffers.back() };
}

void HeapManager::CreatePlacedResource(
	ID3D12Device* device, ID3D12Heap* memory, D3DBufferShared resource,
	size_t offset, const D3D12_RESOURCE_DESC& desc, bool upload
) const {
	HRESULT hr;
	D3D_THROW_FAILED(hr,
		device->CreatePlacedResource(
			memory, offset, &desc,
			upload ? D3D12_RESOURCE_STATE_GENERIC_READ : D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			__uuidof(ID3D12Resource), reinterpret_cast<void**>(
				resource->ReleaseAndGetAddress()
				)
		)
	);
}

D3D12_RESOURCE_DESC HeapManager::GetBufferDesc(size_t bufferSize) const noexcept {
	return CD3DX12_RESOURCE_DESC::Buffer(static_cast<UINT64>(bufferSize));
}

D3D12_RESOURCE_DESC HeapManager::GetTextureDesc(
	size_t height, size_t width, size_t alignment,
	DXGI_FORMAT textureFormat
) const noexcept {
	D3D12_RESOURCE_DESC texDesc = {};

	texDesc.Format = textureFormat;
	texDesc.Width = static_cast<UINT64>(width);
	texDesc.Height = static_cast<UINT>(height);
	texDesc.DepthOrArraySize = 1u;
	texDesc.MipLevels = 0u;
	texDesc.SampleDesc.Count = 1u;
	texDesc.SampleDesc.Quality = 0u;
	texDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Alignment = alignment;
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

	return texDesc;
}

D3D12_RESOURCE_DESC	HeapManager::GetResourceDesc(
	const BufferData& bufferData, bool texture
) const noexcept {
	return 	texture ? GetTextureDesc(
		bufferData.height, bufferData.width,
		bufferData.alignment, bufferData.textureFormat
	) : GetBufferDesc(bufferData.bufferSize);
}

D3D12_RESOURCE_ALLOCATION_INFO HeapManager::GetAllocationInfo(
	ID3D12Device* device,
	size_t height, size_t width, size_t alignment, DXGI_FORMAT textureFormat
) const noexcept {
	D3D12_RESOURCE_DESC texDesc = GetTextureDesc(height, width, alignment, textureFormat);

	D3D12_RESOURCE_ALLOCATION_INFO allocInfo =
		device->GetResourceAllocationInfo(0u, 1u, &texDesc);

	return allocInfo;
}

bool HeapManager::CheckIfAlignmentPossible(UINT64 bufferSize) const noexcept {
	return bufferSize == UINT64_MAX ? false : true;
}
