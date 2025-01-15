#include <D3DExternalResourceFactory.hpp>
#include <D3DExternalBuffer.hpp>

std::unique_ptr<ExternalBuffer> D3DExternalResourceFactory::CreateBufferExp(ExternalBufferType type) const
{
	D3D12_HEAP_TYPE memoryType            = D3D12_HEAP_TYPE_DEFAULT;
	D3D12_RESOURCE_STATES resourceState   = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	const D3D12_RESOURCE_FLAGS usageFlags = D3D12_RESOURCE_FLAG_NONE;

	if (type == ExternalBufferType::CPUVisible)
	{
		memoryType    = D3D12_HEAP_TYPE_UPLOAD;
		resourceState = D3D12_RESOURCE_STATE_GENERIC_READ;
	}

	return std::make_unique<D3DExternalBuffer>(
		m_device, m_memoryManager, memoryType, resourceState, usageFlags
	);
}
