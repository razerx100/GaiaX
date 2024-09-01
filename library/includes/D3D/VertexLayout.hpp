#ifndef VERTEX_LAYOUT_HPP_
#define VERTEX_LAYOUT_HPP_
#include <D3DHeaders.hpp>
#include <string>
#include <vector>

class VertexLayout
{
public:
	VertexLayout() : m_inputDescs{}, m_semanticNames{}, m_vertexOffset{ 0u } {}

	VertexLayout& AddInputElement(
		std::string semanticName, DXGI_FORMAT format, UINT sizeInBytes
	) noexcept;

	[[nodiscard]]
	D3D12_INPUT_LAYOUT_DESC GetLayoutDesc() noexcept;

private:
	std::vector<D3D12_INPUT_ELEMENT_DESC> m_inputDescs;
	std::vector<std::string>              m_semanticNames;
	UINT                                  m_vertexOffset;

public:
	VertexLayout(const VertexLayout& other) noexcept
		: m_inputDescs{ other.m_inputDescs }, m_semanticNames{ other.m_semanticNames },
		m_vertexOffset{ other.m_vertexOffset }
	{}
	VertexLayout& operator=(const VertexLayout& other) noexcept
	{
		m_inputDescs    = other.m_inputDescs;
		m_semanticNames = other.m_semanticNames;
		m_vertexOffset  = other.m_vertexOffset;

		return *this;
	}

	VertexLayout(VertexLayout&& other) noexcept
		: m_inputDescs{ std::move(other.m_inputDescs) },
		m_semanticNames{ std::move(other.m_semanticNames) },
		m_vertexOffset{ other.m_vertexOffset }
	{}
	VertexLayout& operator=(VertexLayout&& other) noexcept
	{
		m_inputDescs    = std::move(other.m_inputDescs);
		m_semanticNames = std::move(other.m_semanticNames);
		m_vertexOffset  = other.m_vertexOffset;

		return *this;
	}
};
#endif
