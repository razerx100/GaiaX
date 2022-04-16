#ifndef MODEL_CONTAINER_HPP_
#define MODEL_CONTAINER_HPP_
#include <BindInstanceGFX.hpp>
#include <string>
#include <memory>

class ModelContainer {
public:
	ModelContainer(const std::string& shaderPath) noexcept;

	void AddModel(
		const IModel* const modelRef
	);

	void InitPipelines(ID3D12Device* device);
	void CreateBuffers(ID3D12Device* device);
	void CopyData(std::atomic_size_t& workCount);
	void RecordUploadBuffers(ID3D12GraphicsCommandList* copyList);
	void ReleaseUploadBuffers();

	void BindCommands(ID3D12GraphicsCommandList* commandList) noexcept;

private:
	using Pipeline =
		std::pair<std::unique_ptr<PipelineObjectGFX>, std::unique_ptr<RootSignatureDynamic>>;

	Pipeline CreatePipeline(
		ID3D12Device* device, const VertexLayout& layout
	) const;

private:
	std::unique_ptr<BindInstanceGFX> m_bindInstance;

	std::string m_shaderPath;
};
#endif