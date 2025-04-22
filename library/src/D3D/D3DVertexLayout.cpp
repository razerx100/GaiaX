#include <D3DVertexLayout.hpp>

namespace Gaia
{
VertexLayout& VertexLayout::AddInputElement(
	std::string semanticName, DXGI_FORMAT format, UINT sizeInBytes
) noexcept {
	m_semanticNames.emplace_back(std::move(semanticName));

	m_inputDescs.emplace_back(
		D3D12_INPUT_ELEMENT_DESC
		{
			.SemanticName         = nullptr,
			// Necessary if there are multiple data with the same Semantic name.
			.SemanticIndex        = 0u,
			.Format               = format,
			.InputSlot            = 0u, // Input Assembler Index.
			.AlignedByteOffset    = m_vertexOffset,
			.InputSlotClass       = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			.InstanceDataStepRate = 0u
		}
	);

	m_vertexOffset += sizeInBytes;

	return *this;
}

D3D12_INPUT_LAYOUT_DESC VertexLayout::GetLayoutDesc() noexcept
{
	// There should be a single Semantic name for each element,
	// so this should be fine.
	for (size_t index = 0u; index < std::size(m_inputDescs); ++index)
		m_inputDescs[index].SemanticName = m_semanticNames[index].c_str();

	return D3D12_INPUT_LAYOUT_DESC
	{
		.pInputElementDescs = std::data(m_inputDescs),
		.NumElements        = static_cast<UINT>(std::size(m_inputDescs))
	};
}
}
