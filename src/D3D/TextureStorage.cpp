#include <TextureStorage.hpp>
#include <InstanceManager.hpp>

size_t TextureStorage::AddColor(const void* data, size_t bufferSize) noexcept {
	D3DGPUSharedAddress sharedAddress =
		VertexBufferInst::GetRef()->AddDataAndGetSharedAddress(data, bufferSize, 256u);

	auto [sharedIndex, sharedHeapHandle] = DescTableManInst::GetRef()->GetColorIndex();

	m_colorData.emplace_back(sharedHeapHandle, sharedAddress, bufferSize);
	m_colorIndices.emplace_back(sharedIndex);

	return m_colorIndices.size() - 1u;
}

void TextureStorage::CreateBufferViews(ID3D12Device* device) {
	D3D12_CONSTANT_BUFFER_VIEW_DESC constBufferViewDesc = {};
	D3D12_CPU_DESCRIPTOR_HANDLE handle = {};

	for (auto& data : m_colorData) {
		constBufferViewDesc.BufferLocation = *data.address;
		constBufferViewDesc.SizeInBytes = static_cast<UINT>(data.bufferSize);

		handle.ptr = *data.handle;

		device->CreateConstantBufferView(&constBufferViewDesc, handle);
	}
}

size_t TextureStorage::GetPhysicalIndexFromVirtual(size_t virtualIndex) const noexcept {
	return *m_colorIndices[virtualIndex];
}
