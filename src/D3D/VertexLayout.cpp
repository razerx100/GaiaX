#include <VertexLayout.hpp>

VertexLayout::VertexLayout() {
	InitLayout();
}

D3D12_INPUT_LAYOUT_DESC VertexLayout::GetLayout() const noexcept {
	return {
		m_inputDescs.data(),
		static_cast<UINT>(m_inputDescs.size())
	};
}

void VertexLayout::InitLayout() noexcept {
	// Position
	UINT inputOffset = 12u;
	m_inputDescs.emplace_back(
		"Position", 0u,
		DXGI_FORMAT_R32G32B32_FLOAT, 0u,
		inputOffset, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0u
	);

	// UV
	inputOffset += 8u;
	m_inputDescs.emplace_back(
		"UV", 0u,
		DXGI_FORMAT_R32G32_FLOAT, 0u,
		inputOffset, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0u
	);
}
