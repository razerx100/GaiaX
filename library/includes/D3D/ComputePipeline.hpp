#ifndef COMPUTE_PIPELINE_HPP_
#define COMPUTE_PIPELINE_HPP_
#include <D3DHeaders.hpp>
#include <memory>
#include <string>
#include <D3DPipelineObject.hpp>
#include <D3DCommandQueue.hpp>
#include <D3DRootSignature.hpp>
#include <ExternalPipeline.hpp>

class ComputePipeline
{
public:
	ComputePipeline() : m_computePipeline{}, m_computeExternalPipeline{} {}

	void Create(
		ID3D12Device2* device, ID3D12RootSignature* computeRootSignature,
		const std::wstring& shaderPath,	const ExternalComputePipeline& computeExtPipeline
	);
	void Create(
		ID3D12Device2* device, const D3DRootSignature& computeRootSignature,
		const std::wstring& shaderPath,	const ExternalComputePipeline& computeExtPipeline
	) {
		Create(device, computeRootSignature.Get(), shaderPath, computeExtPipeline);
	}
	void Recreate(
		ID3D12Device2* device, ID3D12RootSignature* computeRootSignature,
		const std::wstring& shaderPath
	);

	void Bind(const D3DCommandList& computeCmdList) const noexcept;

	[[nodiscard]]
	const ExternalComputePipeline& GetExternalPipeline() const noexcept
	{
		return m_computeExternalPipeline;
	}

private:
	[[nodiscard]]
	static std::unique_ptr<D3DPipelineObject> _createComputePipeline(
		ID3D12Device2* device, ID3D12RootSignature* computeRootSignature,
		const ExternalComputePipeline& computeExtPipeline, const std::wstring& shaderPath
	);

private:
	std::unique_ptr<D3DPipelineObject> m_computePipeline;
	ExternalComputePipeline            m_computeExternalPipeline;

	static constexpr ShaderBinaryType s_shaderBytecodeType = ShaderBinaryType::DXIL;

public:
	ComputePipeline(const ComputePipeline&) = delete;
	ComputePipeline& operator=(const ComputePipeline&) = delete;

	ComputePipeline(ComputePipeline&& other) noexcept
		: m_computePipeline{ std::move(other.m_computePipeline) },
		m_computeExternalPipeline{ std::move(other.m_computeExternalPipeline) }
	{}
	ComputePipeline& operator=(ComputePipeline&& other) noexcept
	{
		m_computePipeline         = std::move(other.m_computePipeline);
		m_computeExternalPipeline = std::move(other.m_computeExternalPipeline);

		return *this;
	}
};
#endif
