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
		BackfaceCulling = 1u
	};

public:
	using RenderFormats_t = std::array<std::uint8_t, 8u>;

public:
	ExternalGraphicsPipeline()
		: m_fragmentShader{}, m_renderTargetFormats{ 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u },
		m_renderTargetAlphaBlendingState{ 0u }, m_renderTargetCount{ 0u },
		m_depthFormat{ 0u }, m_stencilFormat{ 0u }, m_pipelineStates{ 0u }
	{}

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

	void AddRenderTarget(ExternalFormat format)
	{
		// That's the hard limit in DirectX12 and Vulkan can technically have more but will keep it
		// at 8.
		assert(
			m_renderTargetCount <= 8u
			&& "A pipeline can have 8 concurrent Render Targets at max."
		);

		const auto flagPosition = static_cast<std::uint8_t>(1u << m_renderTargetCount);

		m_renderTargetAlphaBlendingState          |= flagPosition;

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
	bool GetRenderTargetBlendingState(size_t index) const noexcept
	{
		const auto flagPosition = static_cast<std::uint8_t>(1u << index);

		return (m_renderTargetAlphaBlendingState & flagPosition) == flagPosition;
	}
	// If multiple pipelines have the same attachment signature, they can be rendered in the
	// same render pass.
	[[nodiscard]]
	bool IsAttachmentSignatureSame(const ExternalGraphicsPipeline& other) const noexcept
	{
		// If the render target count doesn't match, no need to do the next check.
		return m_renderTargetCount == other.m_renderTargetCount &&
			AreRenderTargetFormatsSame(other) &&
			m_depthFormat == other.m_depthFormat &&
			m_stencilFormat == other.m_stencilFormat;
	}

private:
	[[nodiscard]]
	bool AreRenderTargetFormatsSame(const ExternalGraphicsPipeline& other) const noexcept
	{
		// In case the size of the valid render target formats aren't the same, it should still not
		// be an issue, as we are comparing an array, which will have the same actual size.
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
	std::uint8_t    m_renderTargetAlphaBlendingState;
	std::uint8_t    m_renderTargetCount;
	std::uint8_t    m_depthFormat;
	std::uint8_t    m_stencilFormat;
	std::uint16_t   m_pipelineStates;

public:
	ExternalGraphicsPipeline(const ExternalGraphicsPipeline& other) noexcept
		: m_fragmentShader{ other.m_fragmentShader },
		m_renderTargetFormats{ other.m_renderTargetFormats },
		m_renderTargetAlphaBlendingState{ other.m_renderTargetAlphaBlendingState },
		m_renderTargetCount{ other.m_renderTargetCount },
		m_depthFormat{ other.m_depthFormat },
		m_stencilFormat{ other.m_stencilFormat },
		m_pipelineStates{ other.m_pipelineStates }
	{}
	ExternalGraphicsPipeline& operator=(const ExternalGraphicsPipeline& other) noexcept
	{
		m_fragmentShader                 = other.m_fragmentShader;
		m_renderTargetFormats            = other.m_renderTargetFormats;
		m_renderTargetAlphaBlendingState = other.m_renderTargetAlphaBlendingState;
		m_renderTargetCount              = other.m_renderTargetCount;
		m_depthFormat                    = other.m_depthFormat;
		m_stencilFormat                  = other.m_stencilFormat;
		m_pipelineStates                 = other.m_pipelineStates;

		return *this;
	}

	ExternalGraphicsPipeline(ExternalGraphicsPipeline&& other) noexcept
		: m_fragmentShader{ std::move(other.m_fragmentShader) },
		m_renderTargetFormats{ std::move(other.m_renderTargetFormats) },
		m_renderTargetAlphaBlendingState{ other.m_renderTargetAlphaBlendingState },
		m_renderTargetCount{ other.m_renderTargetCount },
		m_depthFormat{ other.m_depthFormat },
		m_stencilFormat{ other.m_stencilFormat },
		m_pipelineStates{ other.m_pipelineStates }
	{}
	ExternalGraphicsPipeline& operator=(ExternalGraphicsPipeline&& other) noexcept
	{
		m_fragmentShader                 = std::move(other.m_fragmentShader);
		m_renderTargetFormats            = std::move(other.m_renderTargetFormats);
		m_renderTargetAlphaBlendingState = other.m_renderTargetAlphaBlendingState;
		m_renderTargetCount              = other.m_renderTargetCount;
		m_depthFormat                    = other.m_depthFormat;
		m_stencilFormat                  = other.m_stencilFormat;
		m_pipelineStates                 = other.m_pipelineStates;

		return *this;
	}

	bool operator==(const ExternalGraphicsPipeline& other) const noexcept
	{
		return m_fragmentShader == other.m_fragmentShader &&
			IsAttachmentSignatureSame(other) &&
			m_renderTargetAlphaBlendingState == other.m_renderTargetAlphaBlendingState &&
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
