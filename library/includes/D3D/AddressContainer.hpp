#ifndef ADDRESS_CONTAINER_HPP_
#define ADDRESS_CONTAINER_HPP_
#include <D3DHeaders.hpp>
#include <concepts>

class SingleAddressContainer {
public:
	template<std::integral AddressType>
	void SetAddress(
		AddressType addressStart, size_t subAllocationSize, size_t alignment
	) noexcept {
		_setAddress(static_cast<size_t>(addressStart), subAllocationSize, alignment);
	}
	void SetAddress(
		void* addressStart, size_t subAllocationSize, size_t alignment
	) noexcept;

	template<std::integral AddressType>
	void UpdateAddressStart(AddressType offset) noexcept {
		m_addressStart += static_cast<size_t>(offset);
	}
	void UpdateAddressStart(void* offset) noexcept;

	template<std::integral AddressType>
	[[nodiscard]]
	AddressType GetAddressStart(size_t index) const noexcept {
		return static_cast<AddressType>(_getAddressStart(index));
	}

	template<typename PointerType>
	[[nodiscard]]
	PointerType* GetAddressPTRStart(size_t index) const noexcept {
		return reinterpret_cast<PointerType*>(_getAddressStart(index));
	}

private:
	void _setAddress(
		size_t addressStart, size_t subAllocationSize, size_t alignment
	) noexcept;

	[[nodiscard]]
	size_t _getAddressStart(size_t index) const noexcept;

private:
	size_t m_addressStart;
	size_t m_subAllocationSize;
};
#endif
