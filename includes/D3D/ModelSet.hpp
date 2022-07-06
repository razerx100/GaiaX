#ifndef MODEL_SET_HPP_
#define MODEL_SET_HPP_
#include <D3DHeaders.hpp>
#include <memory>
#include <vector>
#include <ResourceBuffer.hpp>
#include <IModel.hpp>

template<typename TBufferView>
class BufferView {
public:
	BufferView() = default;
	BufferView(
		TBufferView&& bufferView, D3DGPUSharedAddress sharedAddress
	) : m_bufferView(std::move(bufferView)), m_gpuSharedAddress(sharedAddress) {}

	void SetGPUAddress() noexcept {
		m_bufferView.BufferLocation = *m_gpuSharedAddress;
	}
	void AddBufferView(TBufferView&& bufferView) noexcept {
		m_bufferView = std::move(bufferView);
	}
	void AddSharedAddress(D3DGPUSharedAddress sharedAddress) noexcept {
		m_gpuSharedAddress = sharedAddress;
	}

	const TBufferView* GetAddress() const noexcept {
		return &m_bufferView;
	}

private:
	TBufferView m_bufferView;
	D3DGPUSharedAddress m_gpuSharedAddress;
};

class ModelInstance {
public:
	ModelInstance(std::shared_ptr<IModel>&& model) noexcept;

	void Draw(ID3D12GraphicsCommandList* commandList) const noexcept;

private:
	std::shared_ptr<IModel> m_model;
};

class ModelSetVertex {
public:
	virtual ~ModelSetVertex() = default;

	void AddInstance(std::shared_ptr<IModel>&& model) noexcept;
	void DrawInstances(ID3D12GraphicsCommandList* commandList) const noexcept;

	virtual void UpdateGPUAddressOffsets() noexcept = 0;
	virtual void BindInputs(ID3D12GraphicsCommandList* commandList) const noexcept = 0;

private:
	std::vector<ModelInstance> m_modelInstances;
};

class ModelSetPerVertex final : public ModelSetVertex {
public:
	ModelSetPerVertex(
		D3D12_VERTEX_BUFFER_VIEW&& vertexBufferView, D3DGPUSharedAddress vbvSharedAddress,
		D3D12_INDEX_BUFFER_VIEW&& indexBufferView, D3DGPUSharedAddress ibvSharedAddress
	) noexcept;

	void UpdateGPUAddressOffsets() noexcept override;
	void BindInputs(ID3D12GraphicsCommandList* commandList) const noexcept override;

private:
	BufferView<D3D12_VERTEX_BUFFER_VIEW> m_vertexBufferView;
	BufferView<D3D12_INDEX_BUFFER_VIEW> m_indexBufferView;
};

class ModelSetGVertex final : public ModelSetVertex {
public:
	ModelSetGVertex(
		D3D12_INDEX_BUFFER_VIEW&& indexBufferView, D3DGPUSharedAddress ibvSharedAddress
	) noexcept;

	void UpdateGPUAddressOffsets() noexcept override;
	void BindInputs(ID3D12GraphicsCommandList* commandList) const noexcept override;

private:
	BufferView<D3D12_INDEX_BUFFER_VIEW> m_indexBufferView;
};
#endif
