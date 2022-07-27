#ifndef D3D_RESOURCE_HPP_
#define D3D_RESOURCE_HPP_

#include <D3DHeaders.hpp>
#include <memory>

class D3DResource {
public:
	virtual ~D3DResource() = default;

	void CreateResource(
		ID3D12Device* device, ID3D12Heap* heap, size_t offset, const D3D12_RESOURCE_DESC& desc,
		D3D12_RESOURCE_STATES initialState, const D3D12_CLEAR_VALUE* clearValue = nullptr
	);

	[[nodiscard]]
	ID3D12Resource* Get() const noexcept;
	[[nodiscard]]
	ID3D12Resource** GetAddress() noexcept;
	[[nodiscard]]
	ID3D12Resource** ReleaseAndGetAddress() noexcept;

protected:
	ComPtr<ID3D12Resource> m_pBuffer;
};

class D3DCPUWResource : public D3DResource {
public:
	void MapBuffer();

	[[nodiscard]]
	std::uint8_t* GetCPUWPointer() const noexcept;

private:
	std::uint8_t* m_cpuHandle;
};

using D3DResourceShared = std::shared_ptr<D3DResource>;
using D3DCPUWResourceShared = std::shared_ptr<D3DCPUWResource>;
#endif
