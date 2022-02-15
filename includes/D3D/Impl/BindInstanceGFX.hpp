#ifndef __BIND_INSTANCE_GFX_HPP__
#define __BIND_INSTANCE_GFX_HPP__
#include <IBindInstanceGFX.hpp>
#include <IResourceBuffer.hpp>
#include <vector>

class BindInstanceGFX : public IBindInstanceGFX {
public:
	BindInstanceGFX() noexcept;
	BindInstanceGFX(
		std::unique_ptr<IPipelineObject> pso,
		std::unique_ptr<IRootSignature> signature
	) noexcept;

	VertexLayout GetVertexLayout() const noexcept override;

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
			D3D12_VERTEX_BUFFER_VIEW&& vertexBufferView,
			D3DGPUSharedAddress vbvSharedAddress,
			D3D12_INDEX_BUFFER_VIEW&& indexBufferView,
			D3DGPUSharedAddress ibvSharedAddress,
			size_t indexCount
		) noexcept;

		void UpdateGPUAddressOffsets();

		void AddVBV(
			D3D12_VERTEX_BUFFER_VIEW&& vertexBufferView,
			D3DGPUSharedAddress vbvSharedAddress
		) noexcept;
		void AddIBV(
			D3D12_INDEX_BUFFER_VIEW&& indexBufferView,
			D3DGPUSharedAddress ibvSharedAddress,
			size_t indexCount
		) noexcept;

		void Draw(ID3D12GraphicsCommandList* commandList) noexcept;

	private:
		template<typename TBufferView>
		class BufferView {
		public:
			BufferView() = default;
			BufferView(
				TBufferView&& bufferView,
				D3DGPUSharedAddress sharedAddress
			) : m_bufferView(std::move(bufferView)), m_gpuSharedAddress(sharedAddress) {}

			void SetGPUAddress() noexcept {
				m_bufferView.BufferLocation = *m_gpuSharedAddress;
			}
			void AddBufferView(
				TBufferView&& bufferView
			) noexcept {
				m_bufferView = std::move(bufferView);
			}
			void AddSharedAddress(
				D3DGPUSharedAddress sharedAddress
			) noexcept {
				m_gpuSharedAddress = sharedAddress;
			}

			const TBufferView* GetAddress() const noexcept {
				return &m_bufferView;
			}

		private:
			TBufferView m_bufferView;
			D3DGPUSharedAddress m_gpuSharedAddress;
		};

	private:
		const IModel* const m_modelRef;
		BufferView<D3D12_VERTEX_BUFFER_VIEW> m_vertexBufferView;
		BufferView<D3D12_INDEX_BUFFER_VIEW> m_indexBufferView;
		UINT m_indexCount;
	};

private:
	std::unique_ptr<IPipelineObject> m_pso;
	std::unique_ptr<IRootSignature> m_rootSignature;
	std::vector<std::unique_ptr<ModelRaw>> m_modelsRaw;

	bool m_vertexLayoutAvailable;
	VertexLayout m_vertexLayout;
};
#endif
