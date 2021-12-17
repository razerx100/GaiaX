#include <VertexLayout.hpp>

static const std::vector<const char*> vertexElementTypeNameMap{
	"Position",
	"Color"
};

static const std::vector<DXGI_FORMAT> vertexElementTypeFormatMap{
	DXGI_FORMAT_R32G32B32_FLOAT,
	DXGI_FORMAT_R32G32B32A32_FLOAT
};

static const std::vector<std::uint32_t> vertexElementTypeSizeMap{
	12u,
	16u
};

VertexLayout::VertexLayout(const std::vector<VertexElementType>& inputLayout) {

	for (std::uint32_t inputOffset = 0u; VertexElementType elementType : inputLayout) {
		std::uint32_t elementTypeId = static_cast<std::uint32_t>(elementType);

		m_inputDescs.emplace_back(
			vertexElementTypeNameMap[elementTypeId], 0u,
			vertexElementTypeFormatMap[elementTypeId], 0u,
			inputOffset, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0u
		);

		inputOffset += vertexElementTypeSizeMap[elementTypeId];
	}
}

D3D12_INPUT_LAYOUT_DESC VertexLayout::GetLayout() const noexcept {
	return {
		m_inputDescs.data(),
		static_cast<std::uint32_t>(m_inputDescs.size())
	};
}
