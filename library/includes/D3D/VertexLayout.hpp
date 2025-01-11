#ifndef VERTEX_LAYOUT_HPP_
#define VERTEX_LAYOUT_HPP_
#include <D3DHeaders.hpp>
#include <string>
#include <array>
#include <cassert>

template<UINT ElementCount>
class VertexLayout
{
public:
	VertexLayout()
		: m_inputDescs{}, m_semanticNames{}, m_vertexOffset{ 0u }, m_currentIndex{ 0u }
	{}

	VertexLayout& AddInputElement(
		std::string semanticName, DXGI_FORMAT format, UINT sizeInBytes
	) {
		assert(m_currentIndex < ElementCount && "Element count exceeded.");

		m_semanticNames[m_currentIndex] = std::move(semanticName);

		m_inputDescs[m_currentIndex]    = D3D12_INPUT_ELEMENT_DESC
		{
			.SemanticName         = nullptr,
			// Necessary if there are multiple data with the same Semantic name.
			.SemanticIndex        = 0u,
			.Format               = format,
			.InputSlot            = 0u, // Input Assembler Index.
			.AlignedByteOffset    = m_vertexOffset,
			.InputSlotClass       = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			.InstanceDataStepRate = 0u
		};

		++m_currentIndex;
		m_vertexOffset += sizeInBytes;

		return *this;
	}

	[[nodiscard]]
	D3D12_INPUT_LAYOUT_DESC GetLayoutDesc() noexcept
	{
		// There should be a single Semantic name for each element,
		// so this should be fine.
		for (size_t index = 0u; index < ElementCount; ++index)
			m_inputDescs[index].SemanticName = m_semanticNames[index].c_str();

		return D3D12_INPUT_LAYOUT_DESC
		{
			.pInputElementDescs = std::data(m_inputDescs),
			.NumElements        = ElementCount
		};
	}

private:
	std::array<D3D12_INPUT_ELEMENT_DESC, ElementCount> m_inputDescs;
	std::array<std::string, ElementCount>              m_semanticNames;
	UINT                                               m_vertexOffset;
	size_t                                             m_currentIndex;

public:
	VertexLayout(const VertexLayout& other) noexcept
		: m_inputDescs{ other.m_inputDescs }, m_semanticNames{ other.m_semanticNames },
		m_vertexOffset{ other.m_vertexOffset }, m_currentIndex{ other.m_currentIndex }
	{}
	VertexLayout& operator=(const VertexLayout& other) noexcept
	{
		m_inputDescs    = other.m_inputDescs;
		m_semanticNames = other.m_semanticNames;
		m_vertexOffset  = other.m_vertexOffset;
		m_currentIndex  = other.m_currentIndex;

		return *this;
	}

	VertexLayout(VertexLayout&& other) noexcept
		: m_inputDescs{ other.m_inputDescs },
		m_semanticNames{ std::move(other.m_semanticNames) },
		m_vertexOffset{ other.m_vertexOffset }, m_currentIndex{ other.m_currentIndex }
	{}
	VertexLayout& operator=(VertexLayout&& other) noexcept
	{
		m_inputDescs    = other.m_inputDescs;
		m_semanticNames = std::move(other.m_semanticNames);
		m_vertexOffset  = other.m_vertexOffset;
		m_currentIndex  = other.m_currentIndex;

		return *this;
	}
};
#endif
