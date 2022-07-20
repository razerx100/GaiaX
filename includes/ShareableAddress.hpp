#ifndef SHAREABLE_ADDRESS_HPP_
#define SHAREABLE_ADDRESS_HPP_
#include <concepts>

template<std::integral T>
class _ShareableAddress {
public:
	_ShareableAddress() = default;

	template<std::integral Q>
	_ShareableAddress(Q address) : m_address(static_cast<T>(address)) {}

	template<std::integral Q>
	_ShareableAddress<T>& operator=(Q address) noexcept {
		Set<Q>(address);

		return *this;
	}

	template<std::integral Q> requires (sizeof(T) >= sizeof(Q))
	_ShareableAddress<T>& operator+=(Q address) noexcept {
		Offset<Q>(address);

		return *this;
	}

	template<std::integral Q>
	operator Q() const noexcept {
		return static_cast<Q>(Get());
	}

	T Get() const noexcept {
		return m_address;
	}

	template<std::integral Q>
	void Set(Q address) noexcept {
		m_address = static_cast<T>(address);
	}

	template<std::integral Q> requires (sizeof(T) >= sizeof(Q))
	void Offset(Q address) noexcept {
		m_address += address;
	}

private:
	T m_address;
};

using ShareableAddress = _ShareableAddress<size_t>;
#endif
