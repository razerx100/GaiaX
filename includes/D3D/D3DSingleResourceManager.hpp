#ifndef D3D_SINGLE_RESOURCE_MANAGER_HPP_
#define D3D_SINGLE_RESOURCE_MANAGER_HPP_
#include <D3DResourceManager.hpp>

class D3DSingleResourceManager : public _D3DResourceManager<D3DResourceView> {
public:
	D3DSingleResourceManager(
		ResourceType type, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE
	);

	void CreateResource(ID3D12Device* device);
};
#endif
