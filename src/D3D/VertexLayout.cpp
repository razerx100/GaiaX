#include <VertexLayout.hpp>

VertexLayout::VertexLayout() noexcept : m_vertexOffset(0u) {}

D3D12_INPUT_LAYOUT_DESC VertexLayout::GetLayoutDesc() const noexcept {
	return { std::data(m_inputDescs), static_cast<UINT>(std::size(m_inputDescs)) };
}

VertexLayout& VertexLayout::AddInputElement(
	const char* inputName, DXGI_FORMAT format, UINT sizeInBytes
) noexcept {
	m_inputDescs.emplace_back(
		inputName, 0u, format, 0u, m_vertexOffset,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0u
	);

	m_vertexOffset += sizeInBytes;

	return *this;
}
