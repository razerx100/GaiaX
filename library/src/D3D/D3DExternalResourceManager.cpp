#include <D3DExternalResourceManager.hpp>

D3DExternalResourceManager::D3DExternalResourceManager(
	ID3D12Device* device, MemoryManager* memoryManager
) : m_resourceFactory{ device, memoryManager }
{}
