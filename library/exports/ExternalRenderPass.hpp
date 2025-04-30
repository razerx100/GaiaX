#ifndef EXTERNAL_RENDER_PASS_HPP_
#define EXTERNAL_RENDER_PASS_HPP_
#include <utility>
#include <memory>
#include <DirectXMath.h>

enum class ExternalAttachmentLoadOp
{
	Load,
	Clear,
	DontCare
};

enum class ExternalAttachmentStoreOp
{
	Store,
	DontCare
};

enum class ExternalTextureTransition
{
	FragmentShaderReadOnly
};

template<class ExternalRenderPass_t>
class ExternalRenderPass
{
public:
	ExternalRenderPass() : m_renderPass{} {}
	ExternalRenderPass(std::shared_ptr<ExternalRenderPass_t> renderPass)
		: m_renderPass{ std::move(renderPass) }
	{}

	void SetRenderPassImpl(std::shared_ptr<ExternalRenderPass_t> renderPass) noexcept
	{
		m_renderPass = std::move(renderPass);
	}

	// All the pipelines in a RenderPass must have the same Attachment Signature.
	void AddPipeline(std::uint32_t pipelineIndex)
	{
		m_renderPass->AddPipeline(pipelineIndex);
	}

	// To add a new model, we must do it via the Renderer class, as we
	// would need more information than which is stored in the implementation.

	void RemoveModelBundle(std::uint32_t bundleIndex) noexcept
	{
		m_renderPass->RemoveModelBundle(bundleIndex);
	}
	void RemovePipeline(std::uint32_t pipelineIndex) noexcept
	{
		m_renderPass->RemovePipeline(pipelineIndex);
	}

	// Should be called after something like a window resize, where the buffer handles would
	// change.
	template<class ResourceFactory_t>
	void ResetAttachmentReferences(ResourceFactory_t& resourceFactory)
	{
		m_renderPass->ResetAttachmentReferences(resourceFactory);
	}

	template<class ResourceFactory_t>
	[[nodiscard]]
	std::uint32_t AddStartBarrier(
		std::uint32_t externalTextureIndex, ExternalTextureTransition transitionState,
		ResourceFactory_t& resourceFactory
	) noexcept {
		return m_renderPass->AddStartBarrier(
			externalTextureIndex, transitionState, resourceFactory
		);
	}

	template<class ResourceFactory_t>
	void UpdateStartBarrierResource(
		std::uint32_t barrierIndex, std::uint32_t externalTextureIndex,
		ResourceFactory_t& resourceFactory
	) noexcept {
		return m_renderPass->UpdateStartBarrierResource(
			barrierIndex, externalTextureIndex, resourceFactory
		);
	}

	// Since Dx12 doesn't support separate depth and stencil, if stencil testing is already enabled
	// and the texture index is different, then the depth buffer will be created on the stencil
	// buffer.
	template<class ResourceFactory_t>
	void SetDepthTesting(
		std::uint32_t externalTextureIndex, ExternalAttachmentLoadOp loadOp,
		ExternalAttachmentStoreOp storeOP, ResourceFactory_t& resourceFactory
	) {
		m_renderPass->SetDepthTesting(
			externalTextureIndex, loadOp, storeOP, resourceFactory
		);
	}
	// Only necessary if the LoadOP is clear.
	// In Dx12, the resource needs to be recreated after setting the colour. So, the resource
	// will be recreated.
	template<class ResourceFactory_t>
	void SetDepthClearColour(float clearColour, ResourceFactory_t& resourceFactory)
	{
		m_renderPass->SetDepthClearColour(clearColour, resourceFactory);
	}

	// Since Dx12 doesn't support separate depth and stencil, if depth testing is already enabled
	// and the texture index is different, then the stencil buffer will be created on the depth buffer.
	template<class ResourceFactory_t>
	void SetStencilTesting(
		std::uint32_t externalTextureIndex, ExternalAttachmentLoadOp loadOp,
		ExternalAttachmentStoreOp storeOP, ResourceFactory_t& resourceFactory
	) {
		m_renderPass->SetStencilTesting(
			externalTextureIndex, loadOp, storeOP, resourceFactory
		);
	}
	// Only necessary if the LoadOP is clear.
	// In Dx12, the resource needs to be recreated after setting the colour. So, the resource
	// will be recreated.
	template<class ResourceFactory_t>
	void SetStencilClearColour(std::uint32_t clearColour, ResourceFactory_t& resourceFactory)
	{
		m_renderPass->SetStencilClearColour(clearColour, resourceFactory);
	}

	template<class ResourceFactory_t>
	std::uint32_t AddRenderTarget(
		std::uint32_t externalTextureIndex, ExternalAttachmentLoadOp loadOp,
		ExternalAttachmentStoreOp storeOP, ResourceFactory_t& resourceFactory
	) {
		return m_renderPass->AddRenderTarget(
			externalTextureIndex, loadOp, storeOP, resourceFactory
		);
	}
	// Only necessary if the LoadOP is clear.
	// In Dx12, the resource needs to be recreated after setting the colour. So, the resource
	// will be recreated.
	template<class ResourceFactory_t>
	void SetRenderTargetClearColour(
		std::uint32_t renderTargetIndex, const DirectX::XMFLOAT4& clearColour,
		ResourceFactory_t& resourceFactory
	) {
		m_renderPass->SetRenderTargetClearColour(renderTargetIndex, clearColour, resourceFactory);
	}

	template<class ResourceFactory_t>
	void SetSwapchainCopySource(
		std::uint32_t renderTargetIndex, ResourceFactory_t& resourceFactory
	) noexcept {
		m_renderPass->SetSwapchainCopySource(renderTargetIndex, resourceFactory);
	}

private:
	std::shared_ptr<ExternalRenderPass_t> m_renderPass;

public:
	ExternalRenderPass(const ExternalRenderPass&) = delete;
	ExternalRenderPass& operator=(const ExternalRenderPass&) = delete;

	ExternalRenderPass(ExternalRenderPass&& other) noexcept
		: m_renderPass{ std::move(other.m_renderPass) }
	{}
	ExternalRenderPass& operator=(ExternalRenderPass&& other) noexcept
	{
		m_renderPass = std::move(other.m_renderPass);

		return *this;
	}
};
#endif
