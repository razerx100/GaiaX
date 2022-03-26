#include <HeapManager.hpp>
#include <d3dx12.h>
#include <CRSMath.hpp>
#include <D3DHeap.hpp>
#include <D3DThrowMacros.hpp>
#include <GraphicsEngineDx12.hpp>
#include <UploadBuffer.hpp>

HeapManager::HeapManager()
	: m_currentMemoryOffset(0u) {

	m_uploadHeap = std::make_unique<D3DHeap>();
	m_gpuHeap = std::make_unique<D3DHeap>();
}

void HeapManager::CreateBuffers(ID3D12Device* device, bool msaa) {
	size_t alignedSize = Ceres::Math::Align(
		m_currentMemoryOffset, msaa ? 4_MB : 64_KB
	);

	m_uploadHeap->CreateHeap(device, alignedSize, msaa, true);
	m_gpuHeap->CreateHeap(device, alignedSize, msaa);

	for (size_t index = 0u; index < m_bufferData.size(); ++index) {
		std::shared_ptr<IUploadBuffer>& uploadBuffer = m_uploadBuffers[index];
		std::shared_ptr<D3DBuffer>& gpuBuffer = m_gpuBuffers[index];
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
				bufferData.type == BufferType::Texture
			),
			false
		);
	}
}

void HeapManager::RecordUpload(ID3D12GraphicsCommandList* copyList) {
	for (size_t index = 0u; index < m_bufferData.size(); ++index) {
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

		if (bufferData.type == BufferType::Texture) {
			D3D12_TEXTURE_COPY_LOCATION dest = {};
			dest.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
			dest.pResource = gpuBuffer;
			dest.SubresourceIndex = 0u;

			D3D12_SUBRESOURCE_FOOTPRINT srcFootprint = {};
			srcFootprint.Depth = 1u;
			srcFootprint.Format = bufferData.textureFormat;
			srcFootprint.Width = static_cast<UINT>(bufferData.width);
			srcFootprint.Height = static_cast<UINT>(bufferData.height);
			srcFootprint.RowPitch = static_cast<UINT>(
				Ceres::Math::Align(bufferData.rowPitch, 256u)
				);

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

		D3D12_RESOURCE_STATES afterState = D3D12_RESOURCE_STATE_COMMON;
		if (bufferData.type == BufferType::Index)
			afterState = D3D12_RESOURCE_STATE_INDEX_BUFFER;
		else if (bufferData.type == BufferType::Vertex)
			afterState = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
		else if (bufferData.type == BufferType::Texture)
			afterState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

		D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			gpuBuffer,
			D3D12_RESOURCE_STATE_COPY_DEST,
			afterState
		);
		copyList->ResourceBarrier(1u, &barrier);
	}
}

void HeapManager::ReleaseUploadBuffer() {
	m_bufferData = std::vector<BufferData>();
	m_gpuBuffers = std::vector<std::shared_ptr<D3DBuffer>>();
	m_uploadBuffers = std::vector<std::shared_ptr<IUploadBuffer>>();
	m_uploadHeap.reset();
}

BufferPair HeapManager::AddBuffer(
	size_t bufferSize, BufferType type
) {
	constexpr size_t alignment = 64_KB;

	m_currentMemoryOffset = Ceres::Math::Align(m_currentMemoryOffset, alignment);

	m_bufferData.emplace_back(
		type, bufferSize / 4u, 1u, alignment, m_currentMemoryOffset, bufferSize, bufferSize,
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
		textureFormat = DXGI_FORMAT_R8G8B8A8_UINT;

	size_t alignment = 0u;

	if (msaa)
		alignment = 64_KB;
	else
		alignment = 4_KB;

	D3D12_RESOURCE_DESC texDesc = GetTextureDesc(height, width, alignment, textureFormat);

	D3D12_RESOURCE_ALLOCATION_INFO allocInfo =
		device->GetResourceAllocationInfo(0u, 1u, &texDesc);

	m_currentMemoryOffset = Ceres::Math::Align(m_currentMemoryOffset, allocInfo.Alignment);

	m_bufferData.emplace_back(
		BufferType::Texture,
		width, height, allocInfo.Alignment, m_currentMemoryOffset,
		width * pixelSizeInBytes, allocInfo.SizeInBytes, textureFormat
	);

	m_currentMemoryOffset += allocInfo.SizeInBytes;

	return { m_gpuBuffers.back(), m_uploadBuffers.back() };
}

void HeapManager::CreatePlacedResource(
	ID3D12Device* device, ID3D12Heap* memory, std::shared_ptr<D3DBuffer> resource,
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

