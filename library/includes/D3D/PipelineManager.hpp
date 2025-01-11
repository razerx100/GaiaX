#ifndef PIPELINE_MANAGER_HPP_
#define PIPELINE_MANAGER_HPP_
#include <optional>
#include <ranges>
#include <algorithm>
#include <D3DRootSignature.hpp>
#include <GraphicsPipelineVS.hpp>
#include <GraphicsPipelineMS.hpp>
#include <ComputePipeline.hpp>

template<typename Pipeline>
class PipelineManager
{
public:
	PipelineManager(ID3D12Device5* device)
		: m_device{ device }, m_rootSignature{ nullptr }, m_shaderPath{}, m_pipelines{}
	{}

	void SetRootSignature(ID3D12RootSignature* rootSignature) noexcept
	{
		m_rootSignature = rootSignature;
	}

	void SetShaderPath(std::wstring shaderPath) noexcept
	{
		m_shaderPath = std::move(shaderPath);
	}

	void BindPipeline(size_t index, const D3DCommandList& commandList) const noexcept
	{
		m_pipelines[index].Bind(commandList);
	}

	[[nodiscard]]
	std::optional<std::uint32_t> TryToGetPSOIndex(const ShaderName& shaderName) const noexcept
	{
		std::optional<std::uint32_t> oPSOIndex{};

		auto result = std::ranges::find_if(m_pipelines,
			[&shaderName](const Pipeline& pipeline)
			{
				return shaderName == pipeline.GetShaderName();
			});

		if (result != std::end(m_pipelines))
			oPSOIndex = static_cast<std::uint32_t>(std::distance(std::begin(m_pipelines), result));

		return oPSOIndex;
	}

	std::uint32_t AddGraphicsPipeline(
		const ShaderName& shaderName, DXGI_FORMAT rtvFormat, DXGI_FORMAT dsvFormat
	) {
		const auto psoIndex = static_cast<std::uint32_t>(std::size(m_pipelines));

		Pipeline pipeline{};

		pipeline.Create(
			m_device, m_rootSignature, rtvFormat, dsvFormat, m_shaderPath, shaderName
		);

		m_pipelines.emplace_back(std::move(pipeline));

		return psoIndex;
	}

	std::uint32_t AddComputePipeline(const ShaderName& shaderName)
	{
		const auto psoIndex = static_cast<std::uint32_t>(std::size(m_pipelines));

		Pipeline pipeline{};

		pipeline.Create(m_device, m_rootSignature, m_shaderPath, shaderName);

		m_pipelines.emplace_back(std::move(pipeline));

		return psoIndex;
	}

	void RecreateAllGraphicsPipelines(DXGI_FORMAT rtvFormat, DXGI_FORMAT dsvFormat)
	{
		for (Pipeline& pipeline : m_pipelines)
			pipeline.Recreate(m_device, m_rootSignature, rtvFormat, dsvFormat, m_shaderPath);
	}
	void RecreateAllComputePipelines()
	{
		for (Pipeline& pipeline : m_pipelines)
			pipeline.Recreate(m_device, m_rootSignature, m_shaderPath);
	}

	[[nodiscard]]
	ID3D12RootSignature* GetRootSignature() const noexcept { return m_rootSignature; }

private:
	ID3D12Device5*        m_device;
	ID3D12RootSignature*  m_rootSignature;
	std::wstring          m_shaderPath;
	std::vector<Pipeline> m_pipelines;

public:
	PipelineManager(const PipelineManager&) = delete;
	PipelineManager& operator=(const PipelineManager&) = delete;

	PipelineManager(PipelineManager&& other) noexcept
		: m_device{ other.m_device },
		m_rootSignature{ other.m_rootSignature },
		m_shaderPath{ std::move(other.m_shaderPath) },
		m_pipelines{ std::move(other.m_pipelines) }
	{}
	PipelineManager& operator=(PipelineManager&& other) noexcept
	{
		m_device        = other.m_device;
		m_rootSignature = other.m_rootSignature;
		m_shaderPath    = std::move(other.m_shaderPath);
		m_pipelines     = std::move(other.m_pipelines);

		return *this;
	}
};
#endif
