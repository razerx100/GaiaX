#ifndef __I_BIND_INSTANCE_GFX_HPP__
#define __I_BIND_INSTANCE_GFX_HPP__
#include <D3DHeaders.hpp>
#include <IModel.hpp>
#include <IPipelineObject.hpp>
#include <IRootSignature.hpp>
#include <VertexLayout.hpp>
#include <memory>

class IBindInstanceGFX {
public:
	virtual ~IBindInstanceGFX() = default;

	virtual VertexLayout GetVertexLayout() const noexcept = 0;

	virtual void AddPSO(std::unique_ptr<IPipelineObject> pso) noexcept = 0;
	virtual void AddRootSignature(std::unique_ptr<IRootSignature> signature) noexcept = 0;
	virtual void AddModel(
		const IModel* const modelRef
	) noexcept = 0;

	virtual void SetGPUVirtualAddresses() noexcept = 0;
	virtual void BindCommands(ID3D12GraphicsCommandList* commandList) noexcept = 0;
};
#endif
