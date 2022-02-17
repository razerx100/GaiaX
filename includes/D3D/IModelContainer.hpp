#ifndef __I_MODEL_CONTAINER_HPP__
#define __I_MODEL_CONTAINER_HPP__
#include <D3DHeaders.hpp>
#include <IModel.hpp>

class IModelContainer {
public:
	virtual ~IModelContainer() = default;

	virtual void AddModel(
		const IModel* const modelRef
	) = 0;
	virtual void InitPipelines(ID3D12Device* device) = 0;

	virtual void CreateBuffers(ID3D12Device* device) = 0;
	virtual void CopyData() = 0;
	virtual void RecordUploadBuffers(ID3D12GraphicsCommandList* copyList) = 0;
	virtual void ReleaseUploadBuffers() = 0;

	virtual void BindCommands(ID3D12GraphicsCommandList* commandList) noexcept = 0;
};

IModelContainer* CreateModelContainerInstance(
	const char* shaderPath
);

#endif
