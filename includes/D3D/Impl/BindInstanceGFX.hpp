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
		std::shared_ptr<IRootSignature> signature
	) noexcept;

	void AddPSO(std::unique_ptr<IPipelineObject> pso) noexcept override;
	void AddRootSignature(std::shared_ptr<IRootSignature> signature) noexcept override;
	void AddModel(
		ID3D12Device* device, const IModel* const modelRef
	) noexcept override;

	void CopyData(
		ID3D12Device* device
	) override;
	void RecordUploadBuffers(ID3D12GraphicsCommandList* copyList) override;

	void BindCommands(ID3D12GraphicsCommandList* commandList) noexcept override;

private:
	class ModelRaw {
	public:
		ModelRaw(const IModel* const m_modelRef) noexcept;
		ModelRaw(
			const D3D12_VERTEX_BUFFER_VIEW& vertexBufferView,
			const D3D12_INDEX_BUFFER_VIEW& indexBufferView,
			std::uint64_t indicesCount,
			const IModel* const modelRef
		) noexcept;
		ModelRaw(
			D3D12_VERTEX_BUFFER_VIEW&& vertexBufferView,
			D3D12_INDEX_BUFFER_VIEW&& indexBufferView,
			std::uint64_t indicesCount,
			const IModel* const modelRef
		) noexcept;

		const IModel* const GetModelRef() const noexcept;

		void UpdateVBVGPUOffset(std::uint64_t offset) noexcept;
		void UpdateIBVGPUOffset(std::uint64_t offset) noexcept;

		void AddVBV(const D3D12_VERTEX_BUFFER_VIEW& vertexBufferView) noexcept;
		void AddIBV(
			const D3D12_INDEX_BUFFER_VIEW& indexBufferView,
			std::uint64_t indicesCount
		) noexcept;
		void AddVBV(D3D12_VERTEX_BUFFER_VIEW&& vertexBufferView) noexcept;
		void AddIBV(
			D3D12_INDEX_BUFFER_VIEW&& indexBufferView,
			std::uint64_t indicesCount
		) noexcept;

		void Draw(ID3D12GraphicsCommandList* commandList) noexcept;

	private:
		D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
		D3D12_INDEX_BUFFER_VIEW m_indexBufferView;
		const IModel* const m_modelRef;
		std::uint64_t m_indicesCount;
	};

private:
	std::unique_ptr<IPipelineObject> m_pso;
	std::shared_ptr<IRootSignature> m_rootSignature;
	std::vector<std::unique_ptr<ModelRaw>> m_modelsRaw;
	bool m_textureAvailable;
};
#endif
