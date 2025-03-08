#ifndef D3D_EXTERNAL_RENDER_PASS_HPP_
#define D3D_EXTERNAL_RENDER_PASS_HPP_
#include <ExternalRenderPass.hpp>

class D3DExternalRenderPass : public ExternalRenderPass
{
public:
	D3DExternalRenderPass() {}

	// All the pipelines in a RenderPass must have the same Attachment Signature.
	void AddPipeline(std::uint32_t pipelineIndex, bool sorted) override;

	void AddModelBundle(std::uint32_t bundleIndex) override;

	void RemoveModelBundle(std::uint32_t bundleIndex) noexcept override;
	void RemovePipeline(std::uint32_t pipelineIndex) noexcept override;

	// Should be called after something like a window resize, where the buffer handles would change.
	void ResetAttachmentReferences() override;

	void SetDepthTesting(
		std::uint32_t externalTextureIndex, ExternalAttachmentLoadOp loadOp,
		ExternalAttachmentStoreOp storeOP
	) override;
	// Only necessary if the LoadOP is clear.
	void SetDepthClearColour(float clearColour) noexcept override;

	void SetStencilTesting(
		std::uint32_t externalTextureIndex, ExternalAttachmentLoadOp loadOp,
		ExternalAttachmentStoreOp storeOP
	) override;
	// Only necessary if the LoadOP is clear.
	void SetStencilClearColour(std::uint32_t clearColour) noexcept override;

	std::uint32_t AddRenderTarget(
		std::uint32_t externalTextureIndex, ExternalAttachmentLoadOp loadOp,
		ExternalAttachmentStoreOp storeOP
	) override;
	// Only necessary if the LoadOP is clear.
	void SetRenderTargetClearColour(
		std::uint32_t renderTargetIndex, const DirectX::XMFLOAT4& clearColour
	) noexcept override;

	void SetSwapchainCopySource(std::uint32_t renderTargetIndex) noexcept override;
};
#endif
