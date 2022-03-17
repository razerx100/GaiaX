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
		CreatePlacedResource(
			device, m_uploadHeap->GetHeap(),
			m_uploadBuffers[index]->GetBuffer(), m_bufferData[index].offset,
			GetBufferDesc(m_bufferData[index].bufferSize),
			true
		);
		m_uploadBuffers[index]->MapBuffer();

		CreatePlacedResource(
			device, m_gpuHeap->GetHeap(),
			m_gpuBuffers[index], m_bufferData[index].offset,
			GetResourceDesc(
				m_bufferData[index],
				m_bufferData[index].type == BufferType::Texture
			),
			false
		);
	}
}

void HeapManager::RecordUpload(ID3D12GraphicsCommandList* copyList) {
	for (size_t index = 0u; index < m_bufferData.size(); ++index) {
		D3D12_RESOURCE_BARRIER activationBarriers[2] = {};
		activationBarriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_ALIASING;
		activationBarriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		activationBarriers[0].Aliasing.pResourceBefore = nullptr;
		activationBarriers[0].Aliasing.pResourceBefore =
			m_uploadBuffers[index]->GetBuffer()->Get();

		activationBarriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_ALIASING;
		activationBarriers[1].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		activationBarriers[1].Aliasing.pResourceBefore = nullptr;
		activationBarriers[1].Aliasing.pResourceBefore = m_gpuBuffers[index]->Get();

		copyList->ResourceBarrier(2u, activationBarriers);

		if (m_bufferData[index].type == BufferType::Texture) {
			D3D12_TEXTURE_COPY_LOCATION src = {};
			src.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
			src.pResource = m_uploadBuffers[index]->GetBuffer()->Get();
			src.SubresourceIndex = 0u;

			D3D12_SUBRESOURCE_FOOTPRINT destFootprint = {};
			destFootprint.Depth = 0u;
			destFootprint.Format = GraphicsEngineDx12::RENDER_FORMAT;
			destFootprint.Width = static_cast<UINT>(m_bufferData[index].width / 4u);
			destFootprint.Height = static_cast<UINT>(m_bufferData[index].height);
			destFootprint.RowPitch = static_cast<UINT>(m_bufferData[index].width);

			D3D12_PLACED_SUBRESOURCE_FOOTPRINT placedFootprint = {};
			placedFootprint.Offset = 0u;
			placedFootprint.Footprint = destFootprint;

			D3D12_TEXTURE_COPY_LOCATION dest = {};
			dest.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
			dest.pResource = m_gpuBuffers[index]->Get();
			dest.PlacedFootprint = placedFootprint;

			copyList->CopyTextureRegion(
				&src,
				0u, 0u, 0u,
				&dest,
				nullptr
			);
		}
		else
			copyList->CopyResource(
				m_gpuBuffers[index]->Get(),
				m_uploadBuffers[index]->GetBuffer()->Get()
			);

		D3D12_RESOURCE_STATES afterState = D3D12_RESOURCE_STATE_COMMON;
		if (m_bufferData[index].type == BufferType::Index)
			afterState = D3D12_RESOURCE_STATE_INDEX_BUFFER;
		else if (m_bufferData[index].type == BufferType::Vertex)
			afterState = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
		else if (m_bufferData[index].type == BufferType::Texture)
			afterState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

		D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			m_gpuBuffers[index]->Get(),
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
		1u, bufferSize, alignment, m_currentMemoryOffset, bufferSize, type
	);
	m_gpuBuffers.emplace_back(std::make_shared<D3DBuffer>());
	m_uploadBuffers.emplace_back(std::make_shared<UploadBuffer>());

	m_currentMemoryOffset += bufferSize;

	return { m_gpuBuffers.back(), m_uploadBuffers.back() };
}

BufferPair HeapManager::AddTexture(
	ID3D12Device* device,
	size_t rowPitch, size_t rows, bool msaa
) {
	m_gpuBuffers.emplace_back(std::make_shared<D3DBuffer>());
	m_uploadBuffers.emplace_back(std::make_shared<UploadBuffer>());

	size_t alignment = 0u;

	if (msaa)
		alignment = 64_KB;
	else
		alignment = 4_KB;

	D3D12_RESOURCE_DESC texDesc = GetTextureDesc(rows, rowPitch, alignment);

	D3D12_RESOURCE_ALLOCATION_INFO allocInfo =
		device->GetResourceAllocationInfo(0u, 1u, &texDesc);

	m_currentMemoryOffset = Ceres::Math::Align(m_currentMemoryOffset, allocInfo.Alignment);

	m_bufferData.emplace_back(
		rows, rowPitch, allocInfo.Alignment, m_currentMemoryOffset,
		allocInfo.SizeInBytes, BufferType::Texture
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
	size_t height, size_t width, size_t alignment
) const noexcept {
	D3D12_RESOURCE_DESC texDesc = {};

	texDesc.Format = GraphicsEngineDx12::RENDER_FORMAT;
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
		bufferData.alignment
	) : GetBufferDesc(bufferData.width);
}

