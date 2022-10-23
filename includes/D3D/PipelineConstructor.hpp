#ifndef PIPELINE_CONSTRUCTOR_HPP_
#define PIPELINE_CONSTRUCTOR_HPP_
#include <memory>
#include <string>
#include <D3DPipelineObject.hpp>
#include <RootSignatureDynamic.hpp>

[[nodiscard]]
std::unique_ptr<RootSignatureDynamic> CreateGraphicsRootSignature(ID3D12Device* device);

[[nodiscard]]
std::unique_ptr<RootSignatureDynamic> CreateComputeRootSignature(ID3D12Device* device);

[[nodiscard]]
std::unique_ptr<D3DPipelineObject> CreateGraphicsPipelineObject(
	ID3D12Device* device, const std::wstring& shaderPath,
	ID3D12RootSignature* graphicsRootSignature
);

[[nodiscard]]
std::unique_ptr<D3DPipelineObject> CreateComputePipelineObject(
	ID3D12Device* device, const std::wstring& shaderPath,
	ID3D12RootSignature* computeRootSignature
);
#endif
