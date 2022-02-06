#ifndef __I_RESOURCE_BUFFER_HPP__
#define __I_RESOURCE_BUFFER_HPP__
#include <D3DHeaders.hpp>
#include <D3DBuffer.hpp>
#include <memory>
#include <cstdint>

enum class BufferType {
	Index,
	Vertex,
	Texture
};

class IResourceBuffer {
public:
	virtual ~IResourceBuffer() = default;

	virtual std::shared_ptr<D3DBuffer> AddBuffer(
		const void* source, size_t bufferSize, BufferType type
	) = 0;
	virtual std::shared_ptr<D3DBuffer> AddTexture(const void* source, size_t bufferSize) = 0;
	virtual void CreateBuffers(ID3D12Device* device, bool msaa = false) = 0;
	virtual void CopyData() noexcept = 0;
	virtual void RecordUpload(ID3D12GraphicsCommandList* copyList) = 0;
	virtual void ReleaseUploadBuffer() = 0;
};

IResourceBuffer* CreateResourceBufferInstance();
#endif
