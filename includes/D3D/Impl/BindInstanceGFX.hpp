#ifndef __BIND_INSTANCE_GFX_HPP__
#define __BIND_INSTANCE_GFX_HPP__
#include <IBindInstanceGFX.hpp>
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

	void UpldateBufferViewAddresses(
		size_t vertexAddress,
		size_t indexAddress
	) noexcept override;

	void BindCommands(ID3D12GraphicsCommandList* commandList) noexcept override;

private:
	class ModelRaw {
	public:
		ModelRaw() noexcept;
		ModelRaw(
			const D3D12_VERTEX_BUFFER_VIEW& vertexBufferView,
			const D3D12_INDEX_BUFFER_VIEW& indexBufferView,
			size_t indicesCount
		) noexcept;
		ModelRaw(
			D3D12_VERTEX_BUFFER_VIEW&& vertexBufferView,
			D3D12_INDEX_BUFFER_VIEW&& indexBufferView,
			size_t indicesCount
		) noexcept;

		void UpdateVBVGPUOffset(size_t offset) noexcept;
		void UpdateIBVGPUOffset(size_t offset) noexcept;

		void AddVBV(const D3D12_VERTEX_BUFFER_VIEW& vertexBufferView) noexcept;
		void AddIBV(
			const D3D12_INDEX_BUFFER_VIEW& indexBufferView,
			size_t indexCount
		) noexcept;
		void AddVBV(D3D12_VERTEX_BUFFER_VIEW&& vertexBufferView) noexcept;
		void AddIBV(
			D3D12_INDEX_BUFFER_VIEW&& indexBufferView,
			size_t indexCount
		) noexcept;

		void Draw(ID3D12GraphicsCommandList* commandList) noexcept;

	private:
		D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
		D3D12_INDEX_BUFFER_VIEW m_indexBufferView;
		UINT m_indexCount;
	};

private:
	std::unique_ptr<IPipelineObject> m_pso;
	std::unique_ptr<IRootSignature> m_rootSignature;
	std::vector<std::unique_ptr<ModelRaw>> m_modelsRaw;
	bool m_textureAvailable;
};
#endif
