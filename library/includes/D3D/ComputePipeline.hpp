#ifndef COMPUTE_PIPELINE_HPP_
#define COMPUTE_PIPELINE_HPP_
#include <D3DHeaders.hpp>
#include <memory>
#include <string>
#include <D3DPipelineObject.hpp>
#include <D3DCommandQueue.hpp>
#include <D3DRootSignature.hpp>
#include <Shader.hpp>

class ComputePipeline
{
public:
	ComputePipeline() : m_computePipeline{} {}

	void Create(
		ID3D12Device2* device, ID3D12RootSignature* computeRootSignature,
		const ShaderName& computeShader, const std::wstring& shaderPath
	);
	void Create(
		ID3D12Device2* device, const D3DRootSignature& computeRootSignature,
		const ShaderName& computeShader, const std::wstring& shaderPath
	) {
		Create(device, computeRootSignature.Get(), computeShader, shaderPath);
	}

	void Bind(const D3DCommandList& computeCmdList) const noexcept;

private:
	[[nodiscard]]
	static std::unique_ptr<D3DPipelineObject> _createComputePipeline(
		ID3D12Device2* device, ID3D12RootSignature* computeRootSignature,
		const ShaderName& computeShader, const std::wstring& shaderPath
	);

private:
	std::unique_ptr<D3DPipelineObject> m_computePipeline;

	static constexpr ShaderType s_shaderBytecodeType = ShaderType::DXIL;

public:
	ComputePipeline(const ComputePipeline&) = delete;
	ComputePipeline& operator=(const ComputePipeline&) = delete;

	ComputePipeline(ComputePipeline&& other) noexcept
		: m_computePipeline{ std::move(other.m_computePipeline) }
	{}
	ComputePipeline& operator=(ComputePipeline&& other) noexcept
	{
		m_computePipeline = std::move(other.m_computePipeline);

		return *this;
	}
};
#endif
