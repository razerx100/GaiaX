#ifndef D3D_UPLOADABLE_RESOURCE_MANAGER_HPP_
#define D3D_UPLOADABLE_RESOURCE_MANAGER_HPP_
#include <D3DResourceManager.hpp>

class D3DUploadableResourceManager : public _D3DResourceManager<D3DUploadableResourceView> {
public:
	void CreateResource(ID3D12Device* device);
	void RecordResourceUpload(ID3D12GraphicsCommandList* copyList) noexcept;
	void ReleaseUploadResource() noexcept;
};
#endif
