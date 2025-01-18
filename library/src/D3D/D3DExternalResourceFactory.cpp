#include <D3DExternalResourceFactory.hpp>

size_t D3DExternalResourceFactory::CreateExternalBuffer(ExternalBufferType type)
{
	D3D12_HEAP_TYPE memoryType            = D3D12_HEAP_TYPE_DEFAULT;
	D3D12_RESOURCE_STATES resourceState   = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	const D3D12_RESOURCE_FLAGS usageFlags = D3D12_RESOURCE_FLAG_NONE;

	if (type == ExternalBufferType::CPUVisible)
	{
		memoryType    = D3D12_HEAP_TYPE_UPLOAD;
		resourceState = D3D12_RESOURCE_STATE_GENERIC_READ;
	}

	const size_t bufferIndex = std::size(m_externalBuffers);

	m_externalBuffers.emplace_back(
		std::make_unique<D3DExternalBuffer>(
			m_device, m_memoryManager, memoryType, resourceState, usageFlags
		)
	);

	return bufferIndex;
}

void D3DExternalResourceFactory::RemoveExternalBuffer(size_t index) noexcept
{
	m_externalBuffers.erase(std::next(std::begin(m_externalBuffers), index));
}
