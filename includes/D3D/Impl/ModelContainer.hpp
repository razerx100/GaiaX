#ifndef __MODEL_CONTAINER_HPP__
#define __MODEL_CONTAINER_HPP__
#include <IModelContainer.hpp>
#include <IBindInstanceGFX.hpp>
#include <vector>
#include <string>
#include <memory>

class ModelContainer : public IModelContainer {
public:
	ModelContainer(const char* shaderPath) noexcept;

	void AddModel(
		const IModel* const modelRef, bool texture
	) override;
	void InitPipelines(ID3D12Device* device) override;

	void CreateBuffers(ID3D12Device* device) override;
	void CopyData() override;
	void RecordUploadBuffers(ID3D12GraphicsCommandList* copyList) override;
	void ReleaseUploadBuffers() override;

	void BindCommands(ID3D12GraphicsCommandList* commandList) noexcept override;

private:
	struct InstanceData {
		bool available;
		size_t index;
	};

	using Pipeline = std::pair<std::unique_ptr<IPipelineObject>, std::unique_ptr<IRootSignature>>;

private:
	void InitNewInstance(InstanceData& instanceData) noexcept;
	void AddColoredModel(const IModel* const modelRef);
	void AddTexturedModel(const IModel* const modelRef);

	Pipeline CreateColoredPipeline(
		ID3D12Device* device, const VertexLayout& layout
	) const;
	Pipeline CreateTexturedPipeline(
		ID3D12Device* device, const VertexLayout& layout
	) const;

private:
	std::vector<std::unique_ptr<IBindInstanceGFX>> m_bindInstances;
	InstanceData m_coloredInstanceData;
	InstanceData m_texturedInstanceData;

	std::string m_shaderPath;
};
#endif
