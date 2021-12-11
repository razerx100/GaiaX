#ifndef __VERTEX_LAYOUT_HPP__
#define __VERTEX_LAYOUT_HPP__
#include <D3DHeaders.hpp>
#include <vector>

class VertexLayout {
public:
	VertexLayout() = default;

	void AddElement(const char* elementName, bool alpha = false) noexcept;
	D3D12_INPUT_LAYOUT_DESC GetLayout() const noexcept;

private:
	std::vector<D3D12_INPUT_ELEMENT_DESC> m_inputDescs;
	std::uint32_t m_inputOffset;
};
#endif
