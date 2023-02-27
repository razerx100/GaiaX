#ifndef GRAPHICS_PIPELINE_BASE_HPP_
#define GRAPHICS_PIPELINE_BASE_HPP_
#include <string>
#include <memory>
#include <D3DHeaders.hpp>
#include <D3DPipelineObject.hpp>

class GraphicsPipelineBase {
public:
	void CreateGraphicsPipeline(
		ID3D12Device2* device, ID3D12RootSignature* graphicsRootSignature,
		const std::wstring& shaderPath
	) noexcept;

	void BindGraphicsPipeline(
		ID3D12GraphicsCommandList* graphicsCommandList, ID3D12RootSignature* graphicsRS
	) const noexcept;

protected:
	[[nodiscard]]
	virtual std::unique_ptr<D3DPipelineObject> _createGraphicsPipelineObject(
		ID3D12Device2* device, const std::wstring& shaderPath, const std::wstring& pixelShader,
		ID3D12RootSignature* graphicsRootSignature
	) const noexcept = 0 ;

protected:
	std::unique_ptr<D3DPipelineObject> m_graphicPSO;
	std::wstring m_pixelShader;
};
#endif
