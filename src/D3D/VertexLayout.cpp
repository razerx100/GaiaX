#include <VertexLayout.hpp>

void VertexLayout::AddElement(const char* elementName, bool alpha) noexcept {
	DXGI_FORMAT elementFormat;
	std::uint32_t elementSize;

	if (alpha) {
		elementFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;
		elementSize = 16u;
	}
	else {
		elementFormat = DXGI_FORMAT_R32G32B32_FLOAT;
		elementSize = 12u;
	}

	m_inputDescs.emplace_back(
		elementName, 0, elementFormat,
		0, m_inputOffset, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
	);

	m_inputOffset += elementSize;
}

D3D12_INPUT_LAYOUT_DESC VertexLayout::GetLayout() const noexcept {
	return {
		m_inputDescs.data(),
		static_cast<std::uint32_t>(m_inputDescs.size())
	};
}
