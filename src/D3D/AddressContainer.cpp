#include <AddressContainer.hpp>

void SingleAddressContainer::_setAddressStart(
	size_t addressStart, size_t subAllocationSize
) noexcept {
	m_addressStart = addressStart;
	m_subAllocationSize = subAllocationSize;
}

void SingleAddressContainer::SetAddressStart(
	void* addressStart, size_t subAllocationSize
) noexcept {
	_setAddressStart(reinterpret_cast<size_t>(addressStart), subAllocationSize);
}

void SingleAddressContainer::UpdateAddressStart(void* offset) noexcept {
	m_addressStart += reinterpret_cast<size_t>(offset);
}

size_t SingleAddressContainer::_getAddressStart(size_t index) const noexcept {
	return m_addressStart + (m_subAllocationSize * index);
}
