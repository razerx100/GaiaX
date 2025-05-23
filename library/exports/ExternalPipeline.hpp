#ifndef EXTERNAL_PIPELINE_HPP_
#define EXTERNAL_PIPELINE_HPP_
#include <ExternalFormat.hpp>
#include <Shader.hpp>
#include <cassert>
#include <array>
#include <bitset>

enum class ExternalBlendOP : std::uint8_t
{
	Add,
	Subtract,
	ReverseSubtract,
	Min,
	Max
};

enum class ExternalBlendFactor : std::uint8_t
{
	One,
	Zero,
	SrcColour,
	DstColour,
	SrcAlpha,
	DstAlpha,
	OneMinusSrcColour,
	OneMinusDstColour,
	OneMinusSrcAlpha,
	OneMinusDstAlpha
};

struct ExternalBlendState
{
	bool                enabled        = false;
	ExternalBlendOP     alphaBlendOP   = ExternalBlendOP::Add;
	ExternalBlendOP     colourBlendOP  = ExternalBlendOP::Add;
	ExternalBlendFactor alphaBlendSrc  = ExternalBlendFactor::One;
	ExternalBlendFactor alphaBlendDst  = ExternalBlendFactor::Zero;
	ExternalBlendFactor colourBlendSrc = ExternalBlendFactor::One;
	ExternalBlendFactor colourBlendDst = ExternalBlendFactor::Zero;

	bool operator==(const ExternalBlendState& other) const noexcept
	{
		return enabled     == other.enabled &&
			alphaBlendOP   == other.alphaBlendOP &&
			colourBlendOP  == other.colourBlendOP &&
			alphaBlendSrc  == other.alphaBlendSrc &&
			alphaBlendDst  == other.alphaBlendDst &&
			colourBlendSrc == other.colourBlendSrc &&
			colourBlendDst == other.colourBlendDst;
	}
};

class ExternalGraphicsPipeline
{
	enum class State : std::uint32_t
	{
		Unknown         = 0u,
		BackfaceCulling = 1u,
		DepthWrite      = 2u,
		GPUCulling      = 4u
	};

public:
	static constexpr size_t s_maxRenderTargetCount = 8u;

	using RenderFormats_t = std::array<std::uint8_t, s_maxRenderTargetCount>;
	using BlendStates_t   = std::array<ExternalBlendState, s_maxRenderTargetCount>;

public:
	ExternalGraphicsPipeline()
		: m_vertexShader{}, m_fragmentShader{}, m_renderTargetFormats{ 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u },
		m_blendStates{}, m_renderTargetCount{ 0u }, m_depthFormat{ 0u }, m_stencilFormat{ 0u },
		m_pipelineStates{ static_cast<std::uint32_t>(State::GPUCulling) }
	{}

	ExternalGraphicsPipeline(const ShaderName& fragmentShader, const ShaderName& vertexShader)
		: ExternalGraphicsPipeline{}
	{
		SetVertexShader(vertexShader);
		SetFragmentShader(fragmentShader);
	}

	void SetVertexShader(const ShaderName& vertexShader) noexcept
	{
		m_vertexShader = vertexShader;
	}

	void SetFragmentShader(const ShaderName& fragmentShader) noexcept
	{
		m_fragmentShader = fragmentShader;
	}

	void EnableDepthTesting(ExternalFormat format, bool enableDepthWrite) noexcept
	{
		m_depthFormat = static_cast<std::uint8_t>(format);

		m_pipelineStates[static_cast<size_t>(State::DepthWrite)] = enableDepthWrite;
	}
	void EnableStencilTesting(ExternalFormat format) noexcept
	{
		m_stencilFormat = static_cast<std::uint8_t>(format);
	}
	void EnableBackfaceCulling() noexcept
	{
		m_pipelineStates[static_cast<size_t>(State::BackfaceCulling)] = true;
	}
	void DisableGPUCulling() noexcept
	{
		m_pipelineStates[static_cast<size_t>(State::GPUCulling)] = false;
	}

	void AddRenderTarget(ExternalFormat format, const ExternalBlendState& blendState)
	{
		// That's the hard limit in DirectX12 and Vulkan can technically have more but will keep it
		// at 8.
		assert(
			m_renderTargetCount <= 8u
			&& "A pipeline can have 8 concurrent Render Targets at max."
		);

		m_blendStates[m_renderTargetCount]         = blendState;

		m_renderTargetFormats[m_renderTargetCount] = static_cast<std::uint8_t>(format);

		++m_renderTargetCount;
	}

	[[nodiscard]]
	ExternalFormat GetDepthFormat() const noexcept
	{
		return static_cast<ExternalFormat>(m_depthFormat);
	}
	[[nodiscard]]
	ExternalFormat GetStencilFormat() const noexcept
	{
		return static_cast<ExternalFormat>(m_stencilFormat);
	}
	[[nodiscard]]
	bool GetBackfaceCullingState() const noexcept
	{
		return m_pipelineStates.test(static_cast<size_t>(State::BackfaceCulling));
	}
	[[nodiscard]]
	bool IsDepthWriteEnabled() const noexcept
	{
		return m_pipelineStates.test(static_cast<size_t>(State::DepthWrite));
	}
	[[nodiscard]]
	bool IsGPUCullingEnabled() const noexcept
	{
		return m_pipelineStates.test(static_cast<size_t>(State::GPUCulling));
	}

	[[nodiscard]]
	const ShaderName& GetFragmentShader() const noexcept { return m_fragmentShader; }

	[[nodiscard]]
	const ShaderName& GetVertexShader() const noexcept { return m_vertexShader; }

	[[nodiscard]]
	std::uint32_t GetRenderTargetCount() const noexcept { return m_renderTargetCount; }

	[[nodiscard]]
	ExternalFormat GetRenderTargetFormat(size_t index) const noexcept
	{
		return static_cast<ExternalFormat>(m_renderTargetFormats[index]);
	}
	[[nodiscard]]
	ExternalBlendState GetBlendState(size_t renderTargetIndex) const noexcept
	{
		return m_blendStates[renderTargetIndex];
	}
	// If multiple pipelines have the same attachment signature, they can be rendered in the
	// same render pass.
	[[nodiscard]]
	bool IsAttachmentSignatureSame(const ExternalGraphicsPipeline& other) const noexcept
	{
		// If the render target count doesn't match, no need to do the next check.
		return m_renderTargetCount == other.m_renderTargetCount &&
			AreRenderTargetFormatsSame(other.m_renderTargetFormats) &&
			m_depthFormat   == other.m_depthFormat &&
			m_stencilFormat == other.m_stencilFormat;
	}

private:
	[[nodiscard]]
	bool AreRenderTargetFormatsSame(const RenderFormats_t& otherRenderTargetFormats) const noexcept
	{
		// In case the size of the valid render target formats aren't the same, it should still not
		// be an issue, as we are comparing an array, which will have the same actual size.
		bool isSame = true;

		constexpr size_t formatCount = s_maxRenderTargetCount;

		for (size_t index = 0u; index < formatCount; ++index)
			if (m_renderTargetFormats[index] != otherRenderTargetFormats[index])
			{
				isSame = false;

				break;
			}

		return isSame;
	}

	[[nodiscard]]
	bool AreBlendStatesSame(const BlendStates_t& otherBlendStates) const noexcept
	{
		bool isSame = true;

		constexpr size_t blendStateCount = s_maxRenderTargetCount;

		for (size_t index = 0u; index < blendStateCount; ++index)
			if (m_blendStates[index] != otherBlendStates[index])
			{
				isSame = false;

				break;
			}

		return isSame;
	}

private:
	ShaderName       m_vertexShader;
	ShaderName       m_fragmentShader;
	RenderFormats_t  m_renderTargetFormats;
	BlendStates_t    m_blendStates;
	std::uint8_t     m_renderTargetCount;
	std::uint8_t     m_depthFormat;
	std::uint8_t     m_stencilFormat;
	std::bitset<32U> m_pipelineStates;

public:
	ExternalGraphicsPipeline(const ExternalGraphicsPipeline& other) noexcept
		: m_vertexShader{ other.m_vertexShader },
		m_fragmentShader{ other.m_fragmentShader },
		m_renderTargetFormats{ other.m_renderTargetFormats },
		m_blendStates{ other.m_blendStates },
		m_renderTargetCount{ other.m_renderTargetCount },
		m_depthFormat{ other.m_depthFormat },
		m_stencilFormat{ other.m_stencilFormat },
		m_pipelineStates{ other.m_pipelineStates }
	{}
	ExternalGraphicsPipeline& operator=(const ExternalGraphicsPipeline& other) noexcept
	{
		m_vertexShader        = other.m_vertexShader;
		m_fragmentShader      = other.m_fragmentShader;
		m_renderTargetFormats = other.m_renderTargetFormats;
		m_blendStates         = other.m_blendStates;
		m_renderTargetCount   = other.m_renderTargetCount;
		m_depthFormat         = other.m_depthFormat;
		m_stencilFormat       = other.m_stencilFormat;
		m_pipelineStates      = other.m_pipelineStates;

		return *this;
	}

	ExternalGraphicsPipeline(ExternalGraphicsPipeline&& other) noexcept
		: m_vertexShader{ std::move(other.m_vertexShader) },
		m_fragmentShader{ std::move(other.m_fragmentShader) },
		m_renderTargetFormats{ std::move(other.m_renderTargetFormats) },
		m_blendStates{ other.m_blendStates },
		m_renderTargetCount{ other.m_renderTargetCount },
		m_depthFormat{ other.m_depthFormat },
		m_stencilFormat{ other.m_stencilFormat },
		m_pipelineStates{ other.m_pipelineStates }
	{}
	ExternalGraphicsPipeline& operator=(ExternalGraphicsPipeline&& other) noexcept
	{
		m_vertexShader        = std::move(other.m_vertexShader);
		m_fragmentShader      = std::move(other.m_fragmentShader);
		m_renderTargetFormats = std::move(other.m_renderTargetFormats);
		m_blendStates         = other.m_blendStates;
		m_renderTargetCount   = other.m_renderTargetCount;
		m_depthFormat         = other.m_depthFormat;
		m_stencilFormat       = other.m_stencilFormat;
		m_pipelineStates      = other.m_pipelineStates;

		return *this;
	}

	bool operator==(const ExternalGraphicsPipeline& other) const noexcept
	{
		return m_vertexShader == other.m_vertexShader &&
			m_fragmentShader == other.m_fragmentShader &&
			IsAttachmentSignatureSame(other) &&
			AreBlendStatesSame(other.m_blendStates) &&
			m_pipelineStates    == other.m_pipelineStates;
	}
};

class ExternalComputePipeline
{
public:
	ExternalComputePipeline() : m_computeShader{} {}
	ExternalComputePipeline(const ShaderName& computeShader) : m_computeShader{ computeShader } {}

	void SetComputeShader(const ShaderName& computeShader) noexcept
	{
		m_computeShader = computeShader;
	}

	[[nodiscard]]
	const ShaderName& GetComputeShader() const noexcept { return m_computeShader; }

private:
	ShaderName m_computeShader;

public:
	ExternalComputePipeline(const ExternalComputePipeline& other) noexcept
		: m_computeShader{ other.m_computeShader }
	{}
	ExternalComputePipeline& operator=(const ExternalComputePipeline& other) noexcept
	{
		m_computeShader = other.m_computeShader;

		return *this;
	}

	ExternalComputePipeline(ExternalComputePipeline&& other) noexcept
		: m_computeShader{ std::move(other.m_computeShader) }
	{}
	ExternalComputePipeline& operator=(ExternalComputePipeline&& other) noexcept
	{
		m_computeShader = std::move(other.m_computeShader);

		return *this;
	}

	bool operator==(const ExternalComputePipeline& other) const noexcept
	{
		return m_computeShader == other.m_computeShader;
	}
};
#endif
