#ifndef D3D_EXTERNAL_RENDER_PASS_HPP_
#define D3D_EXTERNAL_RENDER_PASS_HPP_
#include <vector>
#include <bitset>
#include <limits>
#include <utility>
#include <ranges>
#include <algorithm>
#include <ExternalRenderPass.hpp>
#include <D3DRenderPassManager.hpp>
#include <D3DExternalResourceFactory.hpp>

namespace Gaia
{
class D3DExternalRenderPass
{
	struct AttachmentDetails
	{
		std::uint32_t textureIndex = std::numeric_limits<std::uint32_t>::max();
		std::uint32_t barrierIndex = std::numeric_limits<std::uint32_t>::max();
	};

public:
	struct PipelineDetails
	{
		std::uint32_t              pipelineGlobalIndex;
		std::vector<std::uint32_t> modelBundleIndices;
		std::vector<std::uint32_t> pipelineLocalIndices;
	};

public:
	D3DExternalRenderPass(D3DReusableDescriptorHeap* rtvHeap, D3DReusableDescriptorHeap* dsvHeap);

	// All the pipelines in a RenderPass must have the same Attachment Signature.
	void AddPipeline(std::uint32_t pipelineIndex);

	template<class ModelManager_t>
	void AddLocalPipelinesOfModelBundle(
		std::uint32_t modelBundleIndex, const ModelManager_t& modelManager
	) {
		for (PipelineDetails& pipelineDetails : m_pipelineDetails)
		{
			std::optional<size_t> oLocalIndex = modelManager.GetPipelineLocalIndex(
				modelBundleIndex, pipelineDetails.pipelineGlobalIndex
			);

			// I guess it should be okay for a model bundle to not have all the Pipelines?
			if (!oLocalIndex)
				continue;

			auto localIndex = static_cast<std::uint32_t>(*oLocalIndex);

			std::vector<std::uint32_t>& modelBundleIndices = pipelineDetails.modelBundleIndices;
			std::vector<std::uint32_t>& pipelineIndicesInBundle
				= pipelineDetails.pipelineLocalIndices;

			auto result = std::ranges::find(modelBundleIndices, modelBundleIndex);

			size_t resultIndex = std::size(modelBundleIndices);

			if (result == std::end(modelBundleIndices))
			{
				modelBundleIndices.emplace_back(0u);
				pipelineIndicesInBundle.emplace_back(0u);
			}
			else
				resultIndex = std::distance(std::begin(modelBundleIndices), result);

			modelBundleIndices[resultIndex]      = modelBundleIndex;
			pipelineIndicesInBundle[resultIndex] = localIndex;
		}
	}

	void RemoveModelBundle(std::uint32_t bundleIndex) noexcept;
	void RemovePipeline(std::uint32_t pipelineIndex) noexcept;

	// Should be called after something like a window resize, where the buffer handles would change.
	void ResetAttachmentReferences(D3DExternalResourceFactory& resourceFactory);

	[[nodiscard]]
	std::uint32_t AddStartBarrier(
		std::uint32_t externalTextureIndex, ExternalTextureTransition transitionState,
		D3DExternalResourceFactory& resourceFactory
	) noexcept;
	void UpdateStartBarrierResource(
		std::uint32_t barrierIndex, std::uint32_t externalTextureIndex,
		D3DExternalResourceFactory& resourceFactory
	) noexcept;

	void SetDepthTesting(
		std::uint32_t externalTextureIndex, ExternalAttachmentLoadOp loadOp,
		ExternalAttachmentStoreOp storeOP, D3DExternalResourceFactory& resourceFactory
	);
	// Only necessary if the LoadOP is clear.
	// The resource will be recreated if the colour doesn't match with the previous one.
	void SetDepthClearColour(float clearColour, D3DExternalResourceFactory& resourceFactory);

	void SetStencilTesting(
		std::uint32_t externalTextureIndex, ExternalAttachmentLoadOp loadOp,
		ExternalAttachmentStoreOp storeOP, D3DExternalResourceFactory& resourceFactory
	);
	// Only necessary if the LoadOP is clear.
	// The resource will be recreated if the colour doesn't match with the previous one.
	void SetStencilClearColour(
		std::uint32_t clearColour, D3DExternalResourceFactory& resourceFactory
	);

	std::uint32_t AddRenderTarget(
		std::uint32_t externalTextureIndex, ExternalAttachmentLoadOp loadOp,
		ExternalAttachmentStoreOp storeOP, D3DExternalResourceFactory& resourceFactory
	);
	// Only necessary if the LoadOP is clear.
	// The resource will be recreated if the colour doesn't match with the previous one.
	void SetRenderTargetClearColour(
		std::uint32_t renderTargetIndex, const DirectX::XMFLOAT4& clearColour,
		D3DExternalResourceFactory& resourceFactory
	);

	void SetSwapchainCopySource(
		std::uint32_t renderTargetIndex, D3DExternalResourceFactory& resourceFactory
	) noexcept;

	void StartPass(const D3DCommandList& graphicsCmdList) const noexcept;

	void EndPassForSwapchain(
		const D3DCommandList& graphicsCmdList, ID3D12Resource* swapchainBackBuffer,
		D3DExternalResourceFactory& resourceFactory
	) const noexcept;

	[[nodiscard]]
	const std::vector<PipelineDetails>& GetPipelineDetails() const noexcept
	{
		return m_pipelineDetails;
	}

private:
	struct ResourceStates
	{
		D3D12_RESOURCE_STATES beforeState;
		D3D12_RESOURCE_STATES afterState;
	};

private:
	void SetDepthStencil(
		std::uint32_t externalTextureIndex, D3D12_RESOURCE_STATES newState,
		D3D12_DSV_FLAGS dsvFlag, bool clearDepth, bool clearStencil,
		D3DExternalResourceFactory& resourceFactory
	);

	static constexpr size_t s_maxAttachmentCount = 9u;

protected:
	D3DReusableDescriptorHeap*        m_rtvHeap;
	D3DReusableDescriptorHeap*        m_dsvHeap;
	D3DRenderPassManager              m_renderPassManager;
	std::vector<PipelineDetails>      m_pipelineDetails;
	std::vector<AttachmentDetails>    m_renderTargetAttachmentDetails;
	AttachmentDetails                 m_depthStencilAttachmentDetails;
	std::uint32_t                     m_swapchainCopySource;
	std::bitset<s_maxAttachmentCount> m_firstUseFlags;

	// We would want to create the item in the last state and would also want to
	// remove unnecessary barriers. So, have to save the desired states until the
	// resources are created.
	std::vector<ResourceStates>       m_tempResourceStates;

	static constexpr size_t s_depthAttachmentIndex = 8u;
	// And the first 8 will be used by the render targets.

public:
	D3DExternalRenderPass(const D3DExternalRenderPass&) = delete;
	D3DExternalRenderPass& operator=(const D3DExternalRenderPass&) = delete;

	D3DExternalRenderPass(D3DExternalRenderPass&& other) noexcept
		: m_rtvHeap{ std::exchange(other.m_rtvHeap, nullptr) },
		m_dsvHeap{ std::exchange(other.m_dsvHeap, nullptr) },
		m_renderPassManager{ std::move(other.m_renderPassManager) },
		m_pipelineDetails{ std::move(other.m_pipelineDetails) },
		m_renderTargetAttachmentDetails{ std::move(other.m_renderTargetAttachmentDetails) },
		m_depthStencilAttachmentDetails{ other.m_depthStencilAttachmentDetails },
		m_swapchainCopySource{ other.m_swapchainCopySource },
		m_firstUseFlags{ other.m_firstUseFlags },
		m_tempResourceStates{ std::move(other.m_tempResourceStates) }
	{}
	D3DExternalRenderPass& operator=(D3DExternalRenderPass&& other) noexcept
	{
		m_rtvHeap                       = std::exchange(other.m_rtvHeap, nullptr);
		m_dsvHeap                       = std::exchange(other.m_dsvHeap, nullptr);
		m_renderPassManager             = std::move(other.m_renderPassManager);
		m_pipelineDetails               = std::move(other.m_pipelineDetails);
		m_renderTargetAttachmentDetails = std::move(other.m_renderTargetAttachmentDetails);
		m_depthStencilAttachmentDetails = other.m_depthStencilAttachmentDetails;
		m_swapchainCopySource           = other.m_swapchainCopySource;
		m_firstUseFlags                 = other.m_firstUseFlags;
		m_tempResourceStates            = std::move(other.m_tempResourceStates);

		return *this;
	}
};
}
#endif
