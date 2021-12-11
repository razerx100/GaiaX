#ifndef __PIPELINE_OBJECT_GFX_HPP__
#define __PIPELINE_OBJECT_GFX_HPP__
#include <IPipelineObject.hpp>
#include <memory>
#include <vector>

class PipelineObjectGFX : public IPipelineObject {
public:
	void CreatePipelineState(
		ID3D12Device* device,
		const class VertexLayout& vertexLayout,
		ID3D12RootSignature* rootSignature,
		std::shared_ptr<class IShader> vertexShader,
		std::shared_ptr<class IShader> pixelShader
	);

	ID3D12PipelineState* Get() const noexcept override;

private:
	ComPtr<ID3D12PipelineState> m_pPipelineState;
};
#endif
