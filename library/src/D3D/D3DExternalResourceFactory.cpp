#include <D3DExternalResourceFactory.hpp>

namespace Gaia
{
size_t D3DExternalResourceFactory::CreateExternalBuffer(ExternalBufferType type)
{
	D3D12_HEAP_TYPE memoryType            = D3D12_HEAP_TYPE_DEFAULT;
	D3D12_RESOURCE_STATES resourceState   = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	const D3D12_RESOURCE_FLAGS usageFlags = D3D12_RESOURCE_FLAG_NONE;

	if (type != ExternalBufferType::GPUOnly)
	{
		memoryType    = D3D12_HEAP_TYPE_UPLOAD;
		resourceState = D3D12_RESOURCE_STATE_GENERIC_READ;
	}

	return m_externalBuffers.Add(
		std::make_shared<D3DExternalBuffer>(
			m_device, m_memoryManager, memoryType, resourceState, usageFlags
		)
	);
}

size_t D3DExternalResourceFactory::CreateExternalTexture()
{
	return m_externalTextures.Add(
		std::make_shared<D3DExternalTexture>(m_device, m_memoryManager)
	);
}
}
