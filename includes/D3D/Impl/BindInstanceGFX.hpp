#ifndef __BIND_INSTANCE_GFX_HPP__
#define __BIND_INSTANCE_GFX_HPP__
#include <IBindInstanceGFX.hpp>
#include <IVertexBufferView.hpp>
#include <IIndexBufferView.hpp>
#include <vector>

class BindInstanceGFX : public IBindInstanceGFX {
public:
	BindInstanceGFX(bool textureAvailable) noexcept;
	BindInstanceGFX(
		bool textureAvailable,
		std::unique_ptr<IPipelineObject> pso,
		std::unique_ptr<IRootSignature> signature
	) noexcept;

	void AddPSO(std::unique_ptr<IPipelineObject> pso) noexcept override;
	void AddRootSignature(std::unique_ptr<IRootSignature> signature) noexcept override;
	void AddModel(
		const IModel* const modelRef
	) noexcept override;

	void SetGPUVirtualAddresses() noexcept override;

	void BindCommands(ID3D12GraphicsCommandList* commandList) noexcept override;

private:
	class ModelRaw {
	public:
		ModelRaw(const IModel* const modelRef) noexcept;
		ModelRaw(
			const IModel* const modelRef,
			std::unique_ptr<IVertexBufferView> vertexBuffer,
			std::unique_ptr<IIndexBufferView> indexBuffer,
			size_t indexCount
		) noexcept;

		void SetGPUVirtualAddresses() noexcept;

		void AddVB(std::unique_ptr<IVertexBufferView> vertexBuffer) noexcept;
		void AddIB(
			std::unique_ptr<IIndexBufferView> indexBuffer,
			size_t indexCount
		) noexcept;

		void Draw(ID3D12GraphicsCommandList* commandList) noexcept;

	private:
		const IModel* const m_modelRef;
		std::unique_ptr<IVertexBufferView> m_vertexBuffer;
		std::unique_ptr<IIndexBufferView> m_indexBuffer;
		UINT m_indexCount;
	};

private:
	std::unique_ptr<IPipelineObject> m_pso;
	std::unique_ptr<IRootSignature> m_rootSignature;
	std::vector<std::unique_ptr<ModelRaw>> m_modelsRaw;
	bool m_textureAvailable;
};
#endif
