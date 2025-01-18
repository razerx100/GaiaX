#ifndef D3D_EXTERNAL_RESOURCE_MANAGER_HPP_
#define D3D_EXTERNAL_RESOURCE_MANAGER_HPP_
#include <D3DExternalResourceFactory.hpp>

class D3DExternalResourceManager
{
public:
	D3DExternalResourceManager(ID3D12Device* device, MemoryManager* memoryManager);

	[[nodiscard]]
	ExternalResourceFactory* GetResourceFactory() noexcept { return &m_resourceFactory; }

private:
	D3DExternalResourceFactory m_resourceFactory;

public:
	D3DExternalResourceManager(const D3DExternalResourceManager&) = delete;
	D3DExternalResourceManager& operator=(const D3DExternalResourceManager&) = delete;

	D3DExternalResourceManager(D3DExternalResourceManager&& other) noexcept
		: m_resourceFactory{ std::move(other.m_resourceFactory) }
	{}
	D3DExternalResourceManager& operator=(D3DExternalResourceManager&& other) noexcept
	{
		m_resourceFactory = std::move(other.m_resourceFactory);

		return *this;
	}
};
#endif
