#ifndef __MODEL_CONTAINER_HPP__
#define __MODEL_CONTAINER_HPP__
#include <IModelContainer.hpp>
#include <IBindInstanceGFX.hpp>
#include <IRootSignature.hpp>
#include <vector>
#include <string>

class ModelContainer : public IModelContainer {
public:
	ModelContainer(const char* shaderPath) noexcept;

	void AddColoredModel(ID3D12Device* device, const IModel* const modelRef) override;
	void AddTexturedModel(ID3D12Device* device, const IModel* const modelRef) override;

	void BindCommands(ID3D12GraphicsCommandList* commandList) noexcept override;

private:
	struct InstanceData {
		bool available;
		std::uint32_t index;
	};

private:
	void InitNewInstance(InstanceData& instanceData) noexcept;
	void CreateRootSignature(ID3D12Device* device);

private:
	std::vector<std::unique_ptr<IBindInstanceGFX>> m_bindInstances;
	InstanceData m_coloredInstanceData;
	InstanceData m_texturedInstanceData;

	std::shared_ptr<IRootSignature> m_pRootSignature;
	std::string m_shaderPath;
};
#endif
