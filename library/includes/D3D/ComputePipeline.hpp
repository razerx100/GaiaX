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
	ComputePipeline() : m_computePipeline{}, m_computeShader{} {}

	void Create(
		ID3D12Device2* device, ID3D12RootSignature* computeRootSignature,
		const std::wstring& shaderPath,	const ShaderName& computeShader
	);
	void Create(
		ID3D12Device2* device, const D3DRootSignature& computeRootSignature,
		const std::wstring& shaderPath,	const ShaderName& computeShader
	) {
		Create(device, computeRootSignature.Get(), shaderPath, computeShader);
	}
	void Recreate(
		ID3D12Device2* device, ID3D12RootSignature* computeRootSignature,
		const std::wstring& shaderPath
	);

	void Bind(const D3DCommandList& computeCmdList) const noexcept;

	[[nodiscard]]
	ShaderName GetShaderName() const noexcept { return m_computeShader; }

private:
	[[nodiscard]]
	static std::unique_ptr<D3DPipelineObject> _createComputePipeline(
		ID3D12Device2* device, ID3D12RootSignature* computeRootSignature,
		const ShaderName& computeShader, const std::wstring& shaderPath
	);

private:
	std::unique_ptr<D3DPipelineObject> m_computePipeline;
	ShaderName                         m_computeShader;

	static constexpr ShaderType s_shaderBytecodeType = ShaderType::DXIL;

public:
	ComputePipeline(const ComputePipeline&) = delete;
	ComputePipeline& operator=(const ComputePipeline&) = delete;

	ComputePipeline(ComputePipeline&& other) noexcept
		: m_computePipeline{ std::move(other.m_computePipeline) },
		m_computeShader{ std::move(other.m_computeShader) }
	{}
	ComputePipeline& operator=(ComputePipeline&& other) noexcept
	{
		m_computePipeline = std::move(other.m_computePipeline);
		m_computeShader   = std::move(other.m_computeShader);

		return *this;
	}
};
#endif
