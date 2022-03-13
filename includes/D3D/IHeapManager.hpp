#ifndef __I_HEAP_MANAGER_HPP__
#define __I_HEAP_MANAGER_HPP__
#include <D3DHeaders.hpp>
#include <D3DBuffer.hpp>
#include <IUploadBuffer.hpp>
#include <memory>
#include <cstdint>

enum class BufferType {
	Index,
	Vertex,
	Texture
};

using BufferPair = std::pair<std::shared_ptr<D3DBuffer>, std::shared_ptr<IUploadBuffer>>;

class IHeapManager {
public:
	virtual ~IHeapManager() = default;

	virtual BufferPair AddBuffer(
		size_t bufferSize, BufferType type
	) = 0;
	virtual BufferPair AddTexture(
		ID3D12Device* device,
		size_t rowPitch, size_t rows, bool msaa = false
	) = 0;
	virtual void CreateBuffers(ID3D12Device* device, bool msaa = false) = 0;
	virtual void RecordUpload(ID3D12GraphicsCommandList* copyList) = 0;
	virtual void ReleaseUploadBuffer() = 0;
};

IHeapManager* CreateHeapManagerInstance();
#endif
