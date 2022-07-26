module;

#include <D3DHeaders.hpp>
#include <memory>

export module D3DResource;

export class D3DResource {
public:
	virtual ~D3DResource() = default;

	void CreateResource(
		ID3D12Device* device, ID3D12Heap* heap, size_t offset, const D3D12_RESOURCE_DESC& desc,
		D3D12_RESOURCE_STATES initialState
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

export class D3DCPUWResource : public D3DResource {
public:
	void MapBuffer();

	[[nodiscard]]
	std::uint8_t* GetCPUWPointer() const noexcept;

private:
	std::uint8_t* m_cpuHandle;
};

export using D3DResourceShared = std::shared_ptr<D3DResource>;
export using D3DCPUWResourceShared = std::shared_ptr<D3DCPUWResource>;
