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
		: m_device{ device }, m_rootSignature{ nullptr }, m_shaderPath{}, m_pipelines{},
		m_overwritablePSOs{}
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

	void SetOverwritable(const ShaderName& shaderName) noexcept
	{
		std::optional<std::uint32_t> oPsoIndex = TryToGetPSOIndex(shaderName);

		if (oPsoIndex)
			m_overwritablePSOs[oPsoIndex.value()] = true;
	}

	std::uint32_t AddOrGetGraphicsPipeline(
		const ShaderName& pixelShader, DXGI_FORMAT rtvFormat, DXGI_FORMAT dsvFormat
	) {
		auto psoIndex                          = std::numeric_limits<std::uint32_t>::max();
		std::optional<std::uint32_t> oPSOIndex = TryToGetPSOIndex(pixelShader);

		if (oPSOIndex)
			psoIndex = oPSOIndex.value();
		else
		{
			Pipeline pipeline{};

			pipeline.Create(
				m_device, m_rootSignature, rtvFormat, dsvFormat, m_shaderPath, pixelShader
			);

			psoIndex = AddPipeline(std::move(pipeline));
		}

		m_overwritablePSOs[psoIndex] = false;

		return psoIndex;
	}

	std::uint32_t AddOrGetComputePipeline(const ShaderName& computeShader)
	{
		auto psoIndex                          = std::numeric_limits<std::uint32_t>::max();
		std::optional<std::uint32_t> oPSOIndex = TryToGetPSOIndex(computeShader);

		if (oPSOIndex)
			psoIndex = oPSOIndex.value();
		else
		{
			Pipeline pipeline{};

			pipeline.Create(m_device, m_rootSignature, m_shaderPath, computeShader);

			psoIndex = AddPipeline(std::move(pipeline));
		}

		m_overwritablePSOs[psoIndex] = false;

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

	[[nodiscard]]
	const std::wstring& GetShaderPath() const noexcept { return m_shaderPath; }

private:
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

	[[nodiscard]]
	std::optional<std::uint32_t> FindFirstOverwritableIndex() const noexcept
	{
		auto result = std::ranges::find(m_overwritablePSOs, true);

		std::optional<std::uint32_t> foundIndex{};;

		if (result != std::end(m_overwritablePSOs))
			foundIndex = static_cast<std::uint32_t>(
				std::distance(std::begin(m_overwritablePSOs), result)
			);

		return foundIndex;
	}

	[[nodiscard]]
	std::uint32_t AddPipeline(Pipeline&& pipeline) noexcept
	{
		auto psoIndex                                   = std::numeric_limits<std::uint32_t>::max();
		std::optional<std::uint32_t> oOverwritableIndex = FindFirstOverwritableIndex();

		if (oOverwritableIndex)
		{
			psoIndex              = oOverwritableIndex.value();
			m_pipelines[psoIndex] = std::move(pipeline);
		}
		else
		{
			psoIndex = static_cast<std::uint32_t>(std::size(m_pipelines));

			m_pipelines.emplace_back(std::move(pipeline));
		}

		return psoIndex;
	}

private:
	ID3D12Device5*        m_device;
	ID3D12RootSignature*  m_rootSignature;
	std::wstring          m_shaderPath;
	std::vector<Pipeline> m_pipelines;
	std::vector<bool>     m_overwritablePSOs;

public:
	PipelineManager(const PipelineManager&) = delete;
	PipelineManager& operator=(const PipelineManager&) = delete;

	PipelineManager(PipelineManager&& other) noexcept
		: m_device{ other.m_device },
		m_rootSignature{ other.m_rootSignature },
		m_shaderPath{ std::move(other.m_shaderPath) },
		m_pipelines{ std::move(other.m_pipelines) },
		m_overwritablePSOs{ std::move(other.m_overwritablePSOs) }
	{}
	PipelineManager& operator=(PipelineManager&& other) noexcept
	{
		m_device           = other.m_device;
		m_rootSignature    = other.m_rootSignature;
		m_shaderPath       = std::move(other.m_shaderPath);
		m_pipelines        = std::move(other.m_pipelines);
		m_overwritablePSOs = std::move(other.m_overwritablePSOs);

		return *this;
	}
};
#endif
