#ifndef D3D_EXTERNAL_RENDER_PASS_HPP_
#define D3D_EXTERNAL_RENDER_PASS_HPP_
#include <vector>
#include <limits>
#include <utility>
#include <ranges>
#include <algorithm>
#include <ExternalRenderPass.hpp>
#include <D3DRenderPassManager.hpp>
#include <D3DExternalResourceFactory.hpp>

class D3DExternalRenderPass : public ExternalRenderPass
{
public:
	struct PipelineDetails
	{
		std::uint32_t              pipelineGlobalIndex;
		std::vector<std::uint32_t> modelBundleIndices;
		std::vector<std::uint32_t> pipelineLocalIndices;
	};

public:
	D3DExternalRenderPass(
		D3DExternalResourceFactory* resourceFactory, D3DReusableDescriptorHeap* rtvHeap,
		D3DReusableDescriptorHeap* dsvHeap
	);

	// All the pipelines in a RenderPass must have the same Attachment Signature.
	void AddPipeline(std::uint32_t pipelineIndex) override;

	void RemoveModelBundle(std::uint32_t bundleIndex) noexcept override;
	void RemovePipeline(std::uint32_t pipelineIndex) noexcept override;

	// Should be called after something like a window resize, where the buffer handles would change.
	void ResetAttachmentReferences() override;

	void SetDepthTesting(
		std::uint32_t externalTextureIndex, ExternalAttachmentLoadOp loadOp,
		ExternalAttachmentStoreOp storeOP
	) override;
	// Only necessary if the LoadOP is clear.
	// The resource will be recreated if the colour doesn't match with the previous one.
	void SetDepthClearColour(float clearColour) override;

	void SetStencilTesting(
		std::uint32_t externalTextureIndex, ExternalAttachmentLoadOp loadOp,
		ExternalAttachmentStoreOp storeOP
	) override;
	// Only necessary if the LoadOP is clear.
	// The resource will be recreated if the colour doesn't match with the previous one.
	void SetStencilClearColour(std::uint32_t clearColour) override;

	std::uint32_t AddRenderTarget(
		std::uint32_t externalTextureIndex, ExternalAttachmentLoadOp loadOp,
		ExternalAttachmentStoreOp storeOP
	) override;
	// Only necessary if the LoadOP is clear.
	// The resource will be recreated if the colour doesn't match with the previous one.
	void SetRenderTargetClearColour(
		std::uint32_t renderTargetIndex, const DirectX::XMFLOAT4& clearColour
	) override;

	void SetSwapchainCopySource(std::uint32_t renderTargetIndex) noexcept override;

	void StartPass(const D3DCommandList& graphicsCmdList) const noexcept;

	void EndPassForSwapchain(
		const D3DCommandList& graphicsCmdList, ID3D12Resource* swapchainBackBuffer
	) const noexcept;

	[[nodiscard]]
	const std::vector<PipelineDetails>& GetPipelineDetails() const noexcept
	{
		return m_pipelineDetails;
	}

private:
	void SetDepthStencil(
		std::uint32_t externalTextureIndex, D3D12_RESOURCE_STATES newState, D3D12_DSV_FLAGS dsvFlags,
		bool clearDepth, bool clearStencil
	);

protected:
	D3DExternalResourceFactory*  m_resourceFactory;
	D3DRenderPassManager         m_renderPassManager;
	std::vector<PipelineDetails> m_pipelineDetails;
	std::vector<std::uint32_t>   m_renderTargetTextureIndices;
	std::uint32_t                m_depthStencilTextureIndex;
	std::uint32_t                m_swapchainCopySource;

public:
	D3DExternalRenderPass(const D3DExternalRenderPass&) = delete;
	D3DExternalRenderPass& operator=(const D3DExternalRenderPass&) = delete;

	D3DExternalRenderPass(D3DExternalRenderPass&& other) noexcept
		: m_resourceFactory{ std::exchange(other.m_resourceFactory, nullptr) },
		m_renderPassManager{ std::move(other.m_renderPassManager) },
		m_pipelineDetails{ std::move(other.m_pipelineDetails) },
		m_renderTargetTextureIndices{ std::move(other.m_renderTargetTextureIndices) },
		m_depthStencilTextureIndex{ other.m_depthStencilTextureIndex },
		m_swapchainCopySource{ other.m_swapchainCopySource }
	{
		other.m_resourceFactory = nullptr;
	}
	D3DExternalRenderPass& operator=(D3DExternalRenderPass&& other) noexcept
	{
		m_resourceFactory            = std::exchange(other.m_resourceFactory, nullptr);
		m_renderPassManager          = std::move(other.m_renderPassManager);
		m_pipelineDetails            = std::move(other.m_pipelineDetails);
		m_renderTargetTextureIndices = std::move(other.m_renderTargetTextureIndices);
		m_depthStencilTextureIndex   = other.m_depthStencilTextureIndex;
		m_swapchainCopySource        = other.m_swapchainCopySource;

		return *this;
	}
};

template<typename ModelManager_t>
class D3DExternalRenderPassCommon : public D3DExternalRenderPass
{
public:
	D3DExternalRenderPassCommon(
		ModelManager_t* modelManager, D3DExternalResourceFactory* resourceFactory,
		D3DReusableDescriptorHeap* rtvHeap, D3DReusableDescriptorHeap* dsvHeap
	) : D3DExternalRenderPass{ resourceFactory, rtvHeap, dsvHeap }, m_modelManager{ modelManager }
	{}

	void AddModelBundle(std::uint32_t bundleIndex) override
	{
		for (PipelineDetails& pipelineDetails : m_pipelineDetails)
		{
			std::optional<size_t> oLocalIndex = m_modelManager->GetPipelineLocalIndex(
				bundleIndex, pipelineDetails.pipelineGlobalIndex
			);

			auto localIndex = std::numeric_limits<std::uint32_t>::max();

			if (!oLocalIndex)
				localIndex = m_modelManager->AddPipelineToModelBundle(
					bundleIndex, pipelineDetails.pipelineGlobalIndex
				);
			else
				localIndex = static_cast<std::uint32_t>(*oLocalIndex);

			std::vector<std::uint32_t>& modelIndices = pipelineDetails.modelBundleIndices;
			std::vector<std::uint32_t>& localIndices = pipelineDetails.pipelineLocalIndices;

			auto result = std::ranges::find(modelIndices, bundleIndex);

			size_t resultIndex = std::size(modelIndices);

			if (result == std::end(modelIndices))
			{
				modelIndices.emplace_back(0u);
				localIndices.emplace_back(0u);
			}
			else
				resultIndex = std::distance(std::begin(modelIndices), result);

			modelIndices[resultIndex] = bundleIndex;
			localIndices[resultIndex] = localIndex;
		}
	}

private:
	ModelManager_t* m_modelManager;

public:
	D3DExternalRenderPassCommon(const D3DExternalRenderPassCommon&) = delete;
	D3DExternalRenderPassCommon& operator=(const D3DExternalRenderPassCommon&) = delete;

	D3DExternalRenderPassCommon(D3DExternalRenderPassCommon&& other) noexcept
		: D3DExternalRenderPass{ std::move(other) },
		m_modelManager{ std::exchange(other.m_modelManager, nullptr) }
	{}
	D3DExternalRenderPassCommon& operator=(D3DExternalRenderPassCommon&& other) noexcept
	{
		D3DExternalRenderPass::operator=(std::move(other));
		m_modelManager = std::exchange(other.m_modelManager, nullptr);

		return *this;
	}
};
#endif
