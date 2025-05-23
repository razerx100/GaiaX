#include <D3DComputePipeline.hpp>
#include <D3DShader.hpp>

namespace Gaia
{
void ComputePipeline::Create(
	ID3D12Device2* device, ID3D12RootSignature* computeRootSignature,
	const std::wstring& shaderPath, const ExternalComputePipeline& computeExtPipeline
) {
	m_computeExternalPipeline = computeExtPipeline;

	m_computePipeline = _createComputePipeline(
		device, computeRootSignature, computeExtPipeline, shaderPath
	);
}

void ComputePipeline::Recreate(
	ID3D12Device2* device, ID3D12RootSignature* computeRootSignature,
	const std::wstring& shaderPath
) {
	m_computePipeline = _createComputePipeline(
		device, computeRootSignature, m_computeExternalPipeline, shaderPath
	);
}

std::unique_ptr<D3DPipelineObject> ComputePipeline::_createComputePipeline(
	ID3D12Device2* device, ID3D12RootSignature* computeRootSignature,
	const ExternalComputePipeline& computeExtPipeline, const std::wstring& shaderPath
) {
	auto cs            = std::make_unique<D3DShader>();
	const bool success = cs->LoadBinary(
		shaderPath + computeExtPipeline.GetComputeShader().GetNameWithExtension(
			s_shaderBytecodeType
		)
	);

	auto pso = std::make_unique<D3DPipelineObject>();

	if (success)
		pso->CreateComputePipeline(
			device,
			ComputePipelineBuilder{ computeRootSignature }.SetComputeStage(cs->GetByteCode())
		);

	return pso;
}

void ComputePipeline::Bind(const D3DCommandList& computeCmdList) const noexcept
{
	ID3D12GraphicsCommandList* cmdList = computeCmdList.Get();

	cmdList->SetPipelineState(m_computePipeline->Get());
}
}
