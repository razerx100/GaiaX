#include <ResourceBuffer.hpp>
#include <d3dx12.h>
#include <CRSMath.hpp>
#include <D3DHeap.hpp>
#include <D3DThrowMacros.hpp>
#include <GraphicsEngineDx12.hpp>
#include <UploadBuffer.hpp>

ResourceBuffer::ResourceBuffer()
	: m_currentMemoryOffset(0u) {

	m_uploadHeap = std::make_unique<D3DHeap>();
	m_gpuHeap = std::make_unique<D3DHeap>();
}

void ResourceBuffer::CreateBuffers(ID3D12Device* device, bool msaa) {
	size_t alignedSize = Ceres::Math::Align(
		m_currentMemoryOffset, msaa ? 4_MB : 64_KB
	);

	m_uploadHeap->CreateHeap(device, alignedSize, msaa, true);
	m_gpuHeap->CreateHeap(device, alignedSize, msaa);

	for (size_t index = 0u; index < m_uploadBufferData.size(); ++index) {
		CreatePlacedResource(
			device, m_uploadHeap->GetHeap(),
			m_uploadBufferData[index].buffer->GetBuffer(), m_uploadBufferData[index].offset,
			m_uploadBufferData[index].type == BufferType::Texture ?
			GetTextureDesc(0u, 0u) : GetBufferDesc(m_uploadBufferData[index].size),
			true
		);
		m_uploadBufferData[index].buffer->MapBuffer();

		CreatePlacedResource(
			device, m_gpuHeap->GetHeap(),
			m_gpuBuffers[index], m_uploadBufferData[index].offset,
			m_uploadBufferData[index].type == BufferType::Texture ?
			GetTextureDesc(0u, 0u) : GetBufferDesc(m_uploadBufferData[index].size),
			false
		);
	}
}

void ResourceBuffer::CopyData() noexcept {
	for(auto& bufferData : m_uploadBufferData)
		std::memcpy(
			bufferData.buffer->GetCPUHandle(),
			bufferData.data, bufferData.size
		);
}

void ResourceBuffer::RecordUpload(ID3D12GraphicsCommandList* copyList) {
	for (size_t index = 0u; index < m_uploadBufferData.size(); ++index) {
		D3D12_RESOURCE_BARRIER activationBarriers[2] = {};
		activationBarriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_ALIASING;
		activationBarriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		activationBarriers[0].Aliasing.pResourceBefore = nullptr;
		activationBarriers[0].Aliasing.pResourceBefore =
			m_uploadBufferData[index].buffer->GetBuffer()->Get();

		activationBarriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_ALIASING;
		activationBarriers[1].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		activationBarriers[1].Aliasing.pResourceBefore = nullptr;
		activationBarriers[1].Aliasing.pResourceBefore = m_gpuBuffers[index]->Get();

		copyList->ResourceBarrier(2u, activationBarriers);

		copyList->CopyResource(
			m_gpuBuffers[index]->Get(),
			m_uploadBufferData[index].buffer->GetBuffer()->Get()
		);

		D3D12_RESOURCE_STATES afterState = D3D12_RESOURCE_STATE_COMMON;
		if (m_uploadBufferData[index].type == BufferType::Index)
			afterState = D3D12_RESOURCE_STATE_INDEX_BUFFER;
		else if (m_uploadBufferData[index].type == BufferType::Vertex)
			afterState = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;

		D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			m_gpuBuffers[index]->Get(),
			D3D12_RESOURCE_STATE_COPY_DEST,
			afterState
		);
		copyList->ResourceBarrier(1u, &barrier);
	}
}

void ResourceBuffer::ReleaseUploadBuffer() {
	m_uploadBufferData = std::vector<BufferData>();
	m_gpuBuffers = std::vector<std::shared_ptr<D3DBuffer>>();
	m_uploadHeap.reset();
}

std::shared_ptr<D3DBuffer> ResourceBuffer::AddBuffer(
	const void* source, size_t bufferSize, BufferType type
) {
	m_uploadBufferData.emplace_back(
		std::make_unique<UploadBuffer>(),
		source, bufferSize, m_currentMemoryOffset, type
	);
	m_gpuBuffers.emplace_back(std::make_shared<D3DBuffer>());
	m_currentMemoryOffset += bufferSize;

	return m_gpuBuffers.back();
}

std::shared_ptr<D3DBuffer> ResourceBuffer::AddTexture(
	const void* source, size_t bufferSize
) {
	// 4KB, 64KB or 4MB alignment
	return nullptr;
}

void ResourceBuffer::CreatePlacedResource(
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

D3D12_RESOURCE_DESC ResourceBuffer::GetBufferDesc(size_t bufferSize) const noexcept {
	return CD3DX12_RESOURCE_DESC::Buffer(static_cast<UINT64>(bufferSize));
}

D3D12_RESOURCE_DESC ResourceBuffer::GetTextureDesc(
	size_t height, size_t width
) const noexcept {
	return CD3DX12_RESOURCE_DESC::Tex2D(
		GraphicsEngineDx12::RENDER_FORMAT,
		static_cast<UINT64>(width), static_cast<UINT>(height)
	);
}
