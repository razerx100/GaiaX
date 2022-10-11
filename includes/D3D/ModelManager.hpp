#ifndef MODEL_MANAGER_HPP_
#define MODEL_MANAGER_HPP_
#include <string>
#include <memory>
#include <PerFrameBuffers.hpp>
#include <RootSignatureDynamic.hpp>
#include <D3DPipelineObject.hpp>
#include <IModel.hpp>
#include <RenderPipeline.hpp>

class ModelManager {
public:
	ModelManager(std::uint32_t bufferCount) noexcept;

	void SetShaderPath(std::wstring path) noexcept;
	void AddModels(std::vector<std::shared_ptr<IModel>>&& models);
	void AddModelInputs(
		std::unique_ptr<std::uint8_t> vertices, size_t vertexBufferSize, size_t strideSize,
		std::unique_ptr<std::uint8_t> indices, size_t indexBufferSize
	);

	void InitPipelines(ID3D12Device* device);
	void ReserveBuffers(ID3D12Device* device);
	void CreateBuffers(ID3D12Device* device);

	void RecordResourceUpload(ID3D12GraphicsCommandList* copyList) noexcept;
	void ReleaseUploadResource() noexcept;

	void BindCommands(
		ID3D12GraphicsCommandList* commandList, size_t frameIndex
	) const noexcept;

private:
	using Pipeline =
		std::pair<std::unique_ptr<D3DPipelineObject>, std::unique_ptr<RootSignatureDynamic>>;

	Pipeline CreateGraphicsPipeline(ID3D12Device* device) const;
	Pipeline CreateComputePipeline(ID3D12Device* device) const;

private:
	RenderPipeline m_renderPipeline;
	PerFrameBuffers m_pPerFrameBuffers;

	std::wstring m_shaderPath;
};
#endif
