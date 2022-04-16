#include <VertexLayout.hpp>

static const std::vector<const char*> vertexElementTypeNameMap{
	"Position",
	"UV"
};

static const std::vector<DXGI_FORMAT> vertexElementTypeFormatMap{
	DXGI_FORMAT_R32G32B32_FLOAT,
	DXGI_FORMAT_R32G32_FLOAT
};

static const std::vector<size_t> vertexElementTypeSizeMap{
	12u,
	8u
};

VertexLayout::VertexLayout(const std::vector<VertexElementType>& inputLayout) noexcept {
	InitLayout(inputLayout);
}

D3D12_INPUT_LAYOUT_DESC VertexLayout::GetLayout() const noexcept {
	return {
		m_inputDescs.data(),
		static_cast<UINT>(m_inputDescs.size())
	};
}

void VertexLayout::InitLayout(const std::vector<VertexElementType>& inputLayout) noexcept {
	for (size_t inputOffset = 0u; VertexElementType elementType : inputLayout) {
		auto elementTypeId = static_cast<size_t>(elementType);

		m_inputDescs.emplace_back(
			vertexElementTypeNameMap[elementTypeId], 0u,
			vertexElementTypeFormatMap[elementTypeId], 0u,
			static_cast<UINT>(inputOffset), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0u
		);

		inputOffset += vertexElementTypeSizeMap[elementTypeId];
	}
}
