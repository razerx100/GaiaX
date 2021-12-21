#ifndef __I_MODEL_CONTAINER_HPP__
#define __I_MODEL_CONTAINER_HPP__
#include <D3DHeaders.hpp>
#include <IModel.hpp>
#include <memory>

class IModelContainer {
public:
	virtual ~IModelContainer() = default;

	virtual void AddColoredModel(ID3D12Device* device, const IModel* const modelRef) = 0;
	virtual void AddTexturedModel(ID3D12Device* device, const IModel* const modelRef) = 0;

	virtual void BindCommands(ID3D12GraphicsCommandList* commandList) noexcept = 0;
};

IModelContainer* CreateModelContainerInstance(
	const char* shaderPath
);

#endif
