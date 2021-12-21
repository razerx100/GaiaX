#ifndef __BIND_INSTANCE_GFX_HPP__
#define __BIND_INSTANCE_GFX_HPP__
#include <IBindInstanceGFX.hpp>
#include <IVertexBuffer.hpp>
#include <IIndexBuffer.hpp>
#include <vector>

class BindInstanceGFX : public IBindInstanceGFX {
public:
	BindInstanceGFX() = default;
	BindInstanceGFX(
		std::unique_ptr<IPipelineObject> pso,
		std::shared_ptr<IRootSignature> signature
	) noexcept;

	void AddPSO(std::unique_ptr<IPipelineObject> pso) noexcept override;
	void AddRootSignature(std::shared_ptr<IRootSignature> signature) noexcept override;
	void AddColoredModel(
		ID3D12Device* device, const IModel* const modelRef
	) noexcept override;
	void AddTexturedModel(
		ID3D12Device* device, const IModel* const modelRef
	) noexcept override;

	void BindCommands(ID3D12GraphicsCommandList* commandList) noexcept override;

private:
	class ModelRaw {
	public:
		ModelRaw(const IModel* const m_modelRef) noexcept;
		ModelRaw(
			std::unique_ptr<IVertexBuffer> vertexBuffer,
			std::unique_ptr<IIndexBuffer> indexBuffer,
			const IModel* const modelRef
		) noexcept;

		void AddVB(std::unique_ptr<IVertexBuffer> vertexBuffer) noexcept;
		void AddIB(std::unique_ptr<IIndexBuffer> indexBuffer) noexcept;

		void Draw(ID3D12GraphicsCommandList* commandList) noexcept;

	private:
		std::unique_ptr<IVertexBuffer> m_vertexBuffer;
		std::unique_ptr<IIndexBuffer> m_indexBuffer;
		const IModel* const m_modelRef;
	};

private:
	std::unique_ptr<IPipelineObject> m_pso;
	std::shared_ptr<IRootSignature> m_rootSignature;
	std::vector<std::unique_ptr<ModelRaw>> m_modelsRaw;
};
#endif
