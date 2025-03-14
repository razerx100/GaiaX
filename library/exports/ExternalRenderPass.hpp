#ifndef EXTERNAL_RENDER_PASS_HPP_
#define EXTERNAL_RENDER_PASS_HPP_
#include <ExternalPipeline.hpp>

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

class ExternalRenderPass
{
public:
	virtual ~ExternalRenderPass() = default;

	// All the pipelines in a RenderPass must have the same Attachment Signature.
	virtual void AddPipeline(std::uint32_t pipelineIndex, bool sorted) = 0;

	virtual void AddModelBundle(std::uint32_t bundleIndex) = 0;

	virtual void RemoveModelBundle(std::uint32_t bundleIndex) noexcept = 0;
	virtual void RemovePipeline(std::uint32_t pipelineIndex) noexcept = 0;

	// Should be called after something like a window resize, where the buffer handles would change.
	virtual void ResetAttachmentReferences() = 0;

	// Since Dx12 doesn't support separate depth and stencil, if stencil testing is already enabled
	// and the texture index is different, then the depth buffer will be created on the stencil buffer.
	virtual void SetDepthTesting(
		std::uint32_t externalTextureIndex, ExternalAttachmentLoadOp loadOp,
		ExternalAttachmentStoreOp storeOP
	) = 0;
	// Only necessary if the LoadOP is clear.
	virtual void SetDepthClearColour(float clearColour) noexcept = 0;

	// Since Dx12 doesn't support separate depth and stencil, if depth testing is already enabled
	// and the texture index is different, then the stencil buffer will be created on the depth buffer.
	virtual void SetStencilTesting(
		std::uint32_t externalTextureIndex, ExternalAttachmentLoadOp loadOp,
		ExternalAttachmentStoreOp storeOP
	) = 0;
	// Only necessary if the LoadOP is clear.
	virtual void SetStencilClearColour(std::uint32_t clearColour) noexcept = 0;

	virtual std::uint32_t AddRenderTarget(
		std::uint32_t externalTextureIndex, ExternalAttachmentLoadOp loadOp,
		ExternalAttachmentStoreOp storeOP
	) = 0;
	// Only necessary if the LoadOP is clear.
	virtual void SetRenderTargetClearColour(
		std::uint32_t renderTargetIndex, const DirectX::XMFLOAT4& clearColour
	) noexcept = 0;

	virtual void SetSwapchainCopySource(std::uint32_t renderTargetIndex) noexcept = 0;
};
#endif
