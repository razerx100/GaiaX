#ifndef MODEL_CONTAINER_HPP_
#define MODEL_CONTAINER_HPP_
#include <string>
#include <memory>
#include <PerFrameBuffers.hpp>
#include <RootSignatureDynamic.hpp>
#include <PipelineObjectGFX.hpp>
#include <IModel.hpp>
#include <RenderPipeline.hpp>

class ModelContainer {
public:
	ModelContainer(
		const std::string& shaderPath, std::uint32_t bufferCount
	) noexcept;

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
		std::pair<std::unique_ptr<PipelineObjectGFX>, std::unique_ptr<RootSignatureDynamic>>;

	Pipeline CreatePipeline(ID3D12Device* device) const;

private:
	std::unique_ptr<RenderPipeline> m_renderPipeline;
	std::unique_ptr<PerFrameBuffers> m_pPerFrameBuffers;

	std::string m_shaderPath;
};
#endif
