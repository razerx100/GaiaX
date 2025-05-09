#include <D3DModelBuffer.hpp>

namespace Gaia
{
// Model Buffers
void ModelBuffers::ExtendModelBuffers()
{
	const size_t currentModelCount = m_modelContainer->GetModelCount();

	CreateBuffer(currentModelCount);
}

void ModelBuffers::CreateBuffer(size_t modelCount)
{
	// Vertex Data
	{
		constexpr size_t strideSize = GetVertexStride();

		m_modelBuffersInstanceSize        = static_cast<UINT64>(strideSize * modelCount);
		const UINT64 modelBufferTotalSize = m_modelBuffersInstanceSize * m_bufferInstanceCount;

		m_vertexModelBuffers.Create(modelBufferTotalSize, D3D12_RESOURCE_STATE_GENERIC_READ);
	}

	// Fragment Data
	{
		constexpr size_t strideSize = GetPixelStride();

		m_modelBuffersPixelInstanceSize = static_cast<UINT64>(strideSize * modelCount);
		const UINT64 modelBufferTotalSize
			= m_modelBuffersPixelInstanceSize * m_bufferInstanceCount;

		m_pixelModelBuffers.Create(modelBufferTotalSize, D3D12_RESOURCE_STATE_GENERIC_READ);
	}
}

void ModelBuffers::SetDescriptor(
	D3DDescriptorManager& descriptorManager, UINT64 frameIndex, size_t registerSlot,
	size_t registerSpace, bool graphicsQueue
) const {
	const auto bufferOffset = static_cast<UINT64>(frameIndex * m_modelBuffersInstanceSize);

	descriptorManager.SetRootSRV(
		registerSlot, registerSpace, m_vertexModelBuffers.GetGPUAddress() + bufferOffset,
		graphicsQueue
	);
}

void ModelBuffers::SetPixelDescriptor(
	D3DDescriptorManager& descriptorManager, UINT64 frameIndex, size_t registerSlot,
	size_t registerSpace
) const {
	const auto bufferOffset = static_cast<UINT64>(frameIndex * m_modelBuffersPixelInstanceSize);

	descriptorManager.SetRootSRV(
		registerSlot, registerSpace, m_pixelModelBuffers.GetGPUAddress() + bufferOffset, true
	);
}

void ModelBuffers::Update(UINT64 bufferIndex) const noexcept
{
	// Vertex Data
	std::uint8_t* vertexBufferOffset
		= m_vertexModelBuffers.CPUHandle() + bufferIndex * m_modelBuffersInstanceSize;
	constexpr size_t vertexStrideSize = GetVertexStride();
	size_t vertexModelOffset          = 0u;

	// Pixel Data
	std::uint8_t* pixelBufferOffset
		= m_pixelModelBuffers.CPUHandle() + bufferIndex * m_modelBuffersPixelInstanceSize;
	constexpr size_t pixelStrideSize = GetPixelStride();
	size_t pixelModelOffset          = 0u;

	const Callisto::ReusableVector<Model>& models = m_modelContainer->GetModels();

	// All of the models will be here. Even after multiple models have been removed, there
	// should be invalid models there. It is necessary to keep them to preserve the model indices,
	// which is used to keep track of the models both on the CPU and the GPU side.
	for (const Model& model : models)
	{
		// Vertex Data
		{
			using namespace DirectX;

			const XMMATRIX& modelMat = model.GetModelMatrix();

			const ModelVertexData modelVertexData
			{
				.modelMatrix   = modelMat,
				// The normal matrix is the transpose of the inversed model matrix. Not doing this
				// to flip to Column major.
				.normalMatrix  = XMMatrixTranspose(XMMatrixInverse(nullptr, modelMat)),
				.modelOffset   = model.GetModelOffset(),
				.materialIndex = model.GetMaterialIndex(),
				.meshIndex     = model.GetMeshIndex(),
				.modelScale    = model.GetModelScale()
			};

			memcpy(vertexBufferOffset + vertexModelOffset, &modelVertexData, vertexStrideSize);
		}

		// Pixel Data
		{
			const ModelPixelData modelPixelData
			{
				.diffuseTexUVInfo  = model.GetDiffuseUVInfo(),
				.specularTexUVInfo = model.GetSpecularUVInfo(),
				.diffuseTexIndex   = model.GetDiffuseIndex(),
				.specularTexIndex  = model.GetSpecularIndex()
			};

			memcpy(pixelBufferOffset + pixelModelOffset, &modelPixelData, pixelStrideSize);
		}
		// The offsets need to be always increased to keep them consistent.
		vertexModelOffset += vertexStrideSize;
		pixelModelOffset  += pixelStrideSize;
	}
}
}
