#ifndef __MODEL_CONTAINER_HPP__
#define __MODEL_CONTAINER_HPP__
#include <IModelContainer.hpp>
#include <IBindInstanceGFX.hpp>
#include <string>
#include <memory>

class ModelContainer : public IModelContainer {
public:
	ModelContainer(const char* shaderPath) noexcept;

	void AddModel(
		const IModel* const modelRef
	) override;

	void InitPipelines(ID3D12Device* device) override;
	void CreateBuffers(ID3D12Device* device) override;
	void CopyData() override;
	void RecordUploadBuffers(ID3D12GraphicsCommandList* copyList) override;
	void ReleaseUploadBuffers() override;

	void BindCommands(ID3D12GraphicsCommandList* commandList) noexcept override;

private:
	using Pipeline =
		std::pair<std::unique_ptr<IPipelineObject>, std::unique_ptr<IRootSignature>>;

	Pipeline CreatePipeline(
		ID3D12Device* device, const VertexLayout& layout
	) const;

private:
	std::unique_ptr<IBindInstanceGFX> m_bindInstance;

	std::string m_shaderPath;
};
#endif
