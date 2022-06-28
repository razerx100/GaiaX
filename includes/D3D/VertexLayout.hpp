#ifndef VERTEX_LAYOUT_HPP_
#define VERTEX_LAYOUT_HPP_
#include <D3DHeaders.hpp>
#include <vector>

class VertexLayout {
public:
	VertexLayout();

	[[nodiscard]]
	D3D12_INPUT_LAYOUT_DESC GetLayout() const noexcept;

private:
	void InitLayout() noexcept;

private:
	std::vector<D3D12_INPUT_ELEMENT_DESC> m_inputDescs;
};
#endif
