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

enum class ExternalTextureTransition
{
	FragmentShaderReadOnly
};

class ExternalRenderPass
{
public:
	virtual ~ExternalRenderPass() = default;

	// All the pipelines in a RenderPass must have the same Attachment Signature.
	virtual void AddPipeline(std::uint32_t pipelineIndex) = 0;

	virtual void AddModelBundle(std::uint32_t bundleIndex) = 0;

	virtual void RemoveModelBundle(std::uint32_t bundleIndex) noexcept = 0;
	virtual void RemovePipeline(std::uint32_t pipelineIndex) noexcept = 0;

	// Should be called after something like a window resize, where the buffer handles would change.
	virtual void ResetAttachmentReferences() = 0;

	[[nodiscard]]
	virtual std::uint32_t AddStartBarrier(
		std::uint32_t externalTextureIndex, ExternalTextureTransition transitionState
	) noexcept = 0;
	virtual void UpdateStartBarrierResource(
		std::uint32_t barrierIndex, std::uint32_t externalTextureIndex
	) noexcept = 0;

	// Since Dx12 doesn't support separate depth and stencil, if stencil testing is already enabled
	// and the texture index is different, then the depth buffer will be created on the stencil buffer.
	virtual void SetDepthTesting(
		std::uint32_t externalTextureIndex, ExternalAttachmentLoadOp loadOp,
		ExternalAttachmentStoreOp storeOP
	) = 0;
	// Only necessary if the LoadOP is clear.
	// In Dx12, the resource needs to be recreated after setting the colour. So, the resource
	// will be recreated.
	virtual void SetDepthClearColour(float clearColour) = 0;

	// Since Dx12 doesn't support separate depth and stencil, if depth testing is already enabled
	// and the texture index is different, then the stencil buffer will be created on the depth buffer.
	virtual void SetStencilTesting(
		std::uint32_t externalTextureIndex, ExternalAttachmentLoadOp loadOp,
		ExternalAttachmentStoreOp storeOP
	) = 0;
	// Only necessary if the LoadOP is clear.
	// In Dx12, the resource needs to be recreated after setting the colour. So, the resource
	// will be recreated.
	virtual void SetStencilClearColour(std::uint32_t clearColour) = 0;

	virtual std::uint32_t AddRenderTarget(
		std::uint32_t externalTextureIndex, ExternalAttachmentLoadOp loadOp,
		ExternalAttachmentStoreOp storeOP
	) = 0;
	// Only necessary if the LoadOP is clear.
	// In Dx12, the resource needs to be recreated after setting the colour. So, the resource
	// will be recreated.
	virtual void SetRenderTargetClearColour(
		std::uint32_t renderTargetIndex, const DirectX::XMFLOAT4& clearColour
	) = 0;

	virtual void SetSwapchainCopySource(std::uint32_t renderTargetIndex) noexcept = 0;
};
#endif
