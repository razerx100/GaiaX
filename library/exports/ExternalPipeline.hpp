#ifndef EXTERNAL_PIPELINE_HPP_
#define EXTERNAL_PIPELINE_HPP_
#include <ExternalFormat.hpp>
#include <Shader.hpp>
#include <cassert>
#include <array>
#include <bitset>

class ExternalGraphicsPipeline
{
	enum class State : std::uint8_t
	{
		Unknown         = 0u,
		AlphaBlending   = 1u,
		BackfaceCulling = 2u
	};

public:
	using RenderFormats_t = std::array<std::uint8_t, 8u>;

public:
	ExternalGraphicsPipeline(ShaderName fragmentShader)
		: m_fragmentShader{ fragmentShader },
		m_renderTargetFormats{ 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u },
		m_renderTargetCount{ 0u }, m_depthFormat{ 0u }, m_stencilFormat{ 0u }, m_pipelineStates{ 0u }
	{}

	void EnableDepthTesting(ExternalFormat format) noexcept
	{
		m_depthFormat = static_cast<std::uint8_t>(format);
	}
	void EnableStencilTesting(ExternalFormat format) noexcept
	{
		m_stencilFormat = static_cast<std::uint8_t>(format);
	}
	void EnableAlphaBlending() noexcept
	{
		m_pipelineStates |= static_cast<std::uint8_t>(State::AlphaBlending);
	}
	void EnableBackfaceCulling() noexcept
	{
		m_pipelineStates |= static_cast<std::uint8_t>(State::BackfaceCulling);
	}

	void AddRenderTarget(ExternalFormat format)
	{
		// That's the hard limit in DirectX12 and Vulkan can technically have more but will keep it
		// at 8.
		assert(
			m_renderTargetCount <= 8u
			&& "A pipeline can have 8 concurrent Render Targets at max."
		);

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
	bool GetAlphaBlendingState() const noexcept
	{
		constexpr auto blendStateU8 = static_cast<std::uint8_t>(State::AlphaBlending);

		return (m_pipelineStates & blendStateU8) == blendStateU8;
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
	RenderFormats_t GetRenderTargetFormats() const noexcept { return m_renderTargetFormats; }

private:
	[[nodiscard]]
	bool AreRenderTargetFormatsSame(const ExternalGraphicsPipeline& other) const noexcept
	{
		bool isSame = true;

		const size_t formatCount = std::size(m_renderTargetFormats);

		for (size_t index = 0u; index < formatCount; ++index)
			if (m_renderTargetFormats[index] != other.m_renderTargetFormats[index])
			{
				isSame = false;

				break;
			}

		return isSame;
	}

private:
	ShaderName      m_fragmentShader;
	RenderFormats_t m_renderTargetFormats;
	std::uint32_t   m_renderTargetCount;
	std::uint8_t    m_depthFormat;
	std::uint8_t    m_stencilFormat;
	std::uint16_t   m_pipelineStates;

public:
	ExternalGraphicsPipeline(const ExternalGraphicsPipeline& other) noexcept
		: m_fragmentShader{ other.m_fragmentShader },
		m_renderTargetFormats{ other.m_renderTargetFormats },
		m_renderTargetCount{ other.m_renderTargetCount },
		m_depthFormat{ other.m_depthFormat },
		m_stencilFormat{ other.m_stencilFormat },
		m_pipelineStates{ other.m_pipelineStates }
	{}
	ExternalGraphicsPipeline& operator=(const ExternalGraphicsPipeline& other) noexcept
	{
		m_fragmentShader      = other.m_fragmentShader;
		m_renderTargetFormats = other.m_renderTargetFormats;
		m_renderTargetCount   = other.m_renderTargetCount;
		m_depthFormat         = other.m_depthFormat;
		m_stencilFormat       = other.m_stencilFormat;
		m_pipelineStates      = other.m_pipelineStates;

		return *this;
	}

	ExternalGraphicsPipeline(ExternalGraphicsPipeline&& other) noexcept
		: m_fragmentShader{ std::move(other.m_fragmentShader) },
		m_renderTargetFormats{ std::move(other.m_renderTargetFormats) },
		m_renderTargetCount{ other.m_renderTargetCount },
		m_depthFormat{ other.m_depthFormat },
		m_stencilFormat{ other.m_stencilFormat },
		m_pipelineStates{ other.m_pipelineStates }
	{}
	ExternalGraphicsPipeline& operator=(ExternalGraphicsPipeline&& other) noexcept
	{
		m_fragmentShader      = std::move(other.m_fragmentShader);
		m_renderTargetFormats = std::move(other.m_renderTargetFormats);
		m_renderTargetCount   = other.m_renderTargetCount;
		m_depthFormat         = other.m_depthFormat;
		m_stencilFormat       = other.m_stencilFormat;
		m_pipelineStates      = other.m_pipelineStates;

		return *this;
	}

	bool operator==(const ExternalGraphicsPipeline& other) const noexcept
	{
		return m_fragmentShader == other.m_fragmentShader &&
			AreRenderTargetFormatsSame(other) &&
			m_renderTargetCount == other.m_renderTargetCount &&
			m_depthFormat       == other.m_depthFormat &&
			m_stencilFormat     == other.m_stencilFormat &&
			m_pipelineStates    == other.m_pipelineStates;
	}
};

class ExternalComputePipeline
{
public:
	ExternalComputePipeline(ShaderName computeShader) : m_computeShader{ computeShader } {}

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
