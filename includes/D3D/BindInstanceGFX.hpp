#ifndef BIND_INSTANCE_GFX_HPP_
#define BIND_INSTANCE_GFX_HPP_
#include <IModel.hpp>
#include <D3DHeaders.hpp>
#include <PipelineObjectGFX.hpp>
#include <RootSignatureDynamic.hpp>
#include <VertexLayout.hpp>
#include <ResourceBuffer.hpp>
#include <vector>

class BindInstanceGFX {
public:
	BindInstanceGFX() noexcept;
	BindInstanceGFX(
		std::unique_ptr<PipelineObjectGFX> pso,
		std::unique_ptr<RootSignatureDynamic> signature
	) noexcept;

	[[nodiscard]]
	VertexLayout GetVertexLayout() const noexcept;

	void AddPSO(std::unique_ptr<PipelineObjectGFX> pso) noexcept;
	void AddRootSignature(std::unique_ptr<RootSignatureDynamic> signature) noexcept;
	void AddModel(std::shared_ptr<IModel>&& model) noexcept;

	void SetGPUVirtualAddresses() noexcept;

	void BindModels(ID3D12GraphicsCommandList* commandList) const noexcept;
	void BindPipelineObjects(ID3D12GraphicsCommandList* commandList) const noexcept;

private:
	class ModelRaw {
	public:
		ModelRaw(std::shared_ptr<IModel>&& model) noexcept;
		ModelRaw(
			D3D12_VERTEX_BUFFER_VIEW&& vertexBufferView,
			D3DGPUSharedAddress vbvSharedAddress,
			D3D12_INDEX_BUFFER_VIEW&& indexBufferView,
			D3DGPUSharedAddress ibvSharedAddress,
			size_t indexCount,
			std::shared_ptr<IModel>&& model
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

		void Draw(ID3D12GraphicsCommandList* commandList) const noexcept;

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
		std::shared_ptr<IModel> m_model;
		BufferView<D3D12_VERTEX_BUFFER_VIEW> m_vertexBufferView;
		BufferView<D3D12_INDEX_BUFFER_VIEW> m_indexBufferView;
		UINT m_indexCount;
	};

private:
	std::unique_ptr<PipelineObjectGFX> m_pso;
	std::unique_ptr<RootSignatureDynamic> m_rootSignature;
	std::vector<std::unique_ptr<ModelRaw>> m_modelsRaw;

	VertexLayout m_vertexLayout;
};
#endif
