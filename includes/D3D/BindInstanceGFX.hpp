#ifndef BIND_INSTANCE_GFX_HPP_
#define BIND_INSTANCE_GFX_HPP_
#include <IModel.hpp>
#include <D3DHeaders.hpp>
#include <PipelineObjectGFX.hpp>
#include <RootSignatureDynamic.hpp>
#include <VertexLayout.hpp>
#include <ResourceBuffer.hpp>
#include <vector>
#include <ModelSet.hpp>

class BindInstanceGFX {
public:
	BindInstanceGFX() = default;
	BindInstanceGFX(
		std::unique_ptr<PipelineObjectGFX> pso, std::unique_ptr<RootSignatureDynamic> signature
	) noexcept;

	virtual ~BindInstanceGFX() = default;

	void AddPSO(std::unique_ptr<PipelineObjectGFX> pso) noexcept;
	void AddRootSignature(std::unique_ptr<RootSignatureDynamic> signature) noexcept;
	void BindPipelineObjects(ID3D12GraphicsCommandList* commandList) const noexcept;

	virtual void SetGPUVirtualAddresses() noexcept = 0;
	virtual void DrawModels(ID3D12GraphicsCommandList* commandList) const noexcept = 0;
	virtual void AddModels(
		std::vector<std::shared_ptr<IModel>>&& models, std::unique_ptr<IModelInputs> modelInputs
	) noexcept = 0;

private:
	std::unique_ptr<PipelineObjectGFX> m_pso;
	std::unique_ptr<RootSignatureDynamic> m_rootSignature;

protected:
	std::vector<std::unique_ptr<ModelSetVertex>> m_models;
};

class BindInstancePerVertex final : public BindInstanceGFX {
public:
	void AddModels(
		std::vector<std::shared_ptr<IModel>>&& models, std::unique_ptr<IModelInputs> modelInputs
	) noexcept override;
	void SetGPUVirtualAddresses() noexcept override;
	void DrawModels(ID3D12GraphicsCommandList* commandList) const noexcept override;
};

class BindInstanceGVertex final : public BindInstanceGFX {
public:
	void AddModels(
		std::vector<std::shared_ptr<IModel>>&& models, std::unique_ptr<IModelInputs> modelInputs
	) noexcept override;
	void SetGPUVirtualAddresses() noexcept override;
	void DrawModels(ID3D12GraphicsCommandList* commandList) const noexcept override;

private:
	BufferView<D3D12_VERTEX_BUFFER_VIEW> m_vertexBufferView;
};
#endif
