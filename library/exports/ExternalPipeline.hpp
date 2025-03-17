#ifndef EXTERNAL_PIPELINE_HPP_
#define EXTERNAL_PIPELINE_HPP_
#include <ExternalFormat.hpp>
#include <Shader.hpp>
#include <cassert>
#include <array>

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
	bool                enabled;
	ExternalBlendOP     alphaBlendOP;
	ExternalBlendOP     colourBlendOP;
	ExternalBlendFactor alphaBlendSrc;
	ExternalBlendFactor alphaBlendDst;
	ExternalBlendFactor colourBlendSrc;
	ExternalBlendFactor colourBlendDst;

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
	enum class State : std::uint8_t
	{
		Unknown         = 0u,
		BackfaceCulling = 1u
	};

public:
	static constexpr size_t s_maxRenderTargetCount = 8u;

	using RenderFormats_t = std::array<std::uint8_t, s_maxRenderTargetCount>;
	using BlendStates_t   = std::array<ExternalBlendState, s_maxRenderTargetCount>;

public:
	ExternalGraphicsPipeline()
		: m_fragmentShader{}, m_renderTargetFormats{ 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u },
		m_blendStates{}, m_renderTargetCount{ 0u }, m_depthFormat{ 0u }, m_stencilFormat{ 0u },
		m_pipelineStates{ 0u }
	{
		const ExternalBlendState defaultState = GetDefaultBlendState();

		for (ExternalBlendState& blendState : m_blendStates)
			blendState = defaultState;
	}

	ExternalGraphicsPipeline(const ShaderName& fragmentShader)
		: ExternalGraphicsPipeline{}
	{
		SetFragmentShader(fragmentShader);
	}

	void SetFragmentShader(const ShaderName& fragmentShader) noexcept
	{
		m_fragmentShader = fragmentShader;
	}

	void EnableDepthTesting(ExternalFormat format) noexcept
	{
		m_depthFormat = static_cast<std::uint8_t>(format);
	}
	void EnableStencilTesting(ExternalFormat format) noexcept
	{
		m_stencilFormat = static_cast<std::uint8_t>(format);
	}
	void EnableBackfaceCulling() noexcept
	{
		m_pipelineStates |= static_cast<std::uint8_t>(State::BackfaceCulling);
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
		constexpr auto backfaceStateU8 = static_cast<std::uint8_t>(State::BackfaceCulling);

		return (m_pipelineStates & backfaceStateU8) == backfaceStateU8;
	}

	[[nodiscard]]
	const ShaderName& GetFragmentShader() const noexcept { return m_fragmentShader; }

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

	[[nodiscard]]
	static ExternalBlendState GetDefaultBlendState() noexcept
	{
		return ExternalBlendState
		{
			.enabled        = false,
			.alphaBlendOP   = ExternalBlendOP::Add,
			.colourBlendOP  = ExternalBlendOP::Add,
			.alphaBlendSrc  = ExternalBlendFactor::One,
			.alphaBlendDst  = ExternalBlendFactor::Zero,
			.colourBlendSrc = ExternalBlendFactor::One,
			.colourBlendDst = ExternalBlendFactor::Zero
		};
	}

private:
	ShaderName      m_fragmentShader;
	RenderFormats_t m_renderTargetFormats;
	BlendStates_t   m_blendStates;
	std::uint8_t    m_renderTargetCount;
	std::uint8_t    m_depthFormat;
	std::uint8_t    m_stencilFormat;
	std::uint16_t   m_pipelineStates;

public:
	ExternalGraphicsPipeline(const ExternalGraphicsPipeline& other) noexcept
		: m_fragmentShader{ other.m_fragmentShader },
		m_renderTargetFormats{ other.m_renderTargetFormats },
		m_blendStates{ other.m_blendStates },
		m_renderTargetCount{ other.m_renderTargetCount },
		m_depthFormat{ other.m_depthFormat },
		m_stencilFormat{ other.m_stencilFormat },
		m_pipelineStates{ other.m_pipelineStates }
	{}
	ExternalGraphicsPipeline& operator=(const ExternalGraphicsPipeline& other) noexcept
	{
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
		: m_fragmentShader{ std::move(other.m_fragmentShader) },
		m_renderTargetFormats{ std::move(other.m_renderTargetFormats) },
		m_blendStates{ other.m_blendStates },
		m_renderTargetCount{ other.m_renderTargetCount },
		m_depthFormat{ other.m_depthFormat },
		m_stencilFormat{ other.m_stencilFormat },
		m_pipelineStates{ other.m_pipelineStates }
	{}
	ExternalGraphicsPipeline& operator=(ExternalGraphicsPipeline&& other) noexcept
	{
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
		return m_fragmentShader == other.m_fragmentShader &&
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
