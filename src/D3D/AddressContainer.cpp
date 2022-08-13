#include <AddressContainer.hpp>
#include <D3DHelperFunctions.hpp>

void SingleAddressContainer::_setAddress(
	size_t addressStart, size_t subAllocationSize, size_t alignment
) noexcept {
	m_addressStart = addressStart;
	// Subsequent allocations needs to be aligned as well
	m_subAllocationSize = Align(subAllocationSize, alignment);
}

void SingleAddressContainer::SetAddress(
	void* addressStart, size_t subAllocationSize, size_t alignment
) noexcept {
	_setAddress(reinterpret_cast<size_t>(addressStart), subAllocationSize, alignment);
}

void SingleAddressContainer::UpdateAddressStart(void* offset) noexcept {
	m_addressStart += reinterpret_cast<size_t>(offset);
}

size_t SingleAddressContainer::_getAddressStart(size_t index) const noexcept {
	return m_addressStart + (m_subAllocationSize * index);
}
