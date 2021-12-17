#ifndef __I_BIND_INSTANCE_GFX_HPP__
#define __I_BIND_INSTANCE_GFX_HPP__
#include <D3DHeaders.hpp>
#include <IModel.hpp>
#include <IPipelineObject.hpp>
#include <IRootSignature.hpp>
#include <memory>

class IBindInstanceGFX {
public:
	virtual ~IBindInstanceGFX() = default;

	virtual void AddPSO(std::unique_ptr<IPipelineObject> pso) noexcept = 0;
	virtual void AddRootSignature(std::shared_ptr<IRootSignature> signature) noexcept = 0;
	virtual void AddColoredModel(
		ID3D12Device* device, std::unique_ptr<IModel> model
	) noexcept = 0;
	virtual void AddTexturedModel(
		ID3D12Device* device, std::unique_ptr<IModel> model
	) noexcept = 0;

	virtual void BindCommands(ID3D12GraphicsCommandList* commandList) noexcept = 0;
};
#endif
