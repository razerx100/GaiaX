#ifndef VERTEX_LAYOUT_HPP_
#define VERTEX_LAYOUT_HPP_
#include <D3DHeaders.hpp>
#include <IModel.hpp>
#include <vector>

class VertexLayout {
public:
	VertexLayout() = default;
	VertexLayout(const std::vector<VertexElementType>& inputLayout) noexcept;

	void InitLayout(const std::vector<VertexElementType>& inputLayout) noexcept;

	[[nodiscard]]
	D3D12_INPUT_LAYOUT_DESC GetLayout() const noexcept;

private:
	std::vector<D3D12_INPUT_ELEMENT_DESC> m_inputDescs;
};
#endif
