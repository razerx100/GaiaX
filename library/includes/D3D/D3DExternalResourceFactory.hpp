#ifndef D3D_EXTERNAL_RESOURCE_FACTORY_HPP_
#define D3D_EXTERNAL_RESOURCE_FACTORY_HPP_
#include <utility>
#include <ExternalResourceFactory.hpp>
#include <D3DAllocator.hpp>

class D3DExternalResourceFactory : public ExternalResourceFactory
{
public:
	D3DExternalResourceFactory(ID3D12Device* device, MemoryManager* memoryManager)
		: m_device{ device }, m_memoryManager{ memoryManager }
	{}

	[[nodiscard]]
	std::unique_ptr<ExternalBuffer> CreateExternalBuffer(ExternalBufferType type) const override;

private:
	ID3D12Device*  m_device;
	MemoryManager* m_memoryManager;

public:
	D3DExternalResourceFactory(const D3DExternalResourceFactory& other) noexcept
		: m_device{ other.m_device }, m_memoryManager{ other.m_memoryManager }
	{}
	D3DExternalResourceFactory& operator=(const D3DExternalResourceFactory& other) noexcept
	{
		m_device        = other.m_device;
		m_memoryManager = other.m_memoryManager;

		return *this;
	}

	D3DExternalResourceFactory(D3DExternalResourceFactory&& other) noexcept
		: m_device{ other.m_device }, m_memoryManager{ std::exchange(other.m_memoryManager, nullptr) }
	{}
	D3DExternalResourceFactory& operator=(D3DExternalResourceFactory&& other) noexcept
	{
		m_device        = other.m_device;
		m_memoryManager = std::exchange(other.m_memoryManager, nullptr);

		return *this;
	}
};
#endif
