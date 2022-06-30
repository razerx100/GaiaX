#ifndef VERTEX_LAYOUT_HPP_
#define VERTEX_LAYOUT_HPP_
#include <D3DHeaders.hpp>
#include <string>
#include <vector>

class VertexLayout {
public:
	VertexLayout() noexcept;

	void AddInputElement(
		const std::string& inputName, DXGI_FORMAT format, UINT sizeInBytes
	) noexcept;

	[[nodiscard]]
	D3D12_INPUT_LAYOUT_DESC GetLayoutDesc() const noexcept;

private:
	std::vector<D3D12_INPUT_ELEMENT_DESC> m_inputDescs;

	UINT m_vertexOffset;
};
#endif
