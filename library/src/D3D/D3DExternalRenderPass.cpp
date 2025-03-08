#include <D3DExternalRenderPass.hpp>

void D3DExternalRenderPass::AddPipeline(std::uint32_t pipelineIndex, bool sorted)
{

}

void D3DExternalRenderPass::AddModelBundle(std::uint32_t bundleIndex)
{

}

void D3DExternalRenderPass::RemoveModelBundle(std::uint32_t bundleIndex) noexcept
{

}

void D3DExternalRenderPass::RemovePipeline(std::uint32_t pipelineIndex) noexcept
{

}

void D3DExternalRenderPass::ResetAttachmentReferences()
{

}

void D3DExternalRenderPass::SetDepthTesting(
	std::uint32_t externalTextureIndex, ExternalAttachmentLoadOp loadOp,
	ExternalAttachmentStoreOp storeOP
) {

}

void D3DExternalRenderPass::SetDepthClearColour(float clearColour) noexcept
{

}

void D3DExternalRenderPass::SetStencilTesting(
	std::uint32_t externalTextureIndex, ExternalAttachmentLoadOp loadOp,
	ExternalAttachmentStoreOp storeOP
) {

}

void D3DExternalRenderPass::SetStencilClearColour(std::uint32_t clearColour) noexcept
{

}

std::uint32_t D3DExternalRenderPass::AddRenderTarget(
	std::uint32_t externalTextureIndex, ExternalAttachmentLoadOp loadOp,
	ExternalAttachmentStoreOp storeOP
) {
	return 0u;
}

void D3DExternalRenderPass::SetRenderTargetClearColour(
	std::uint32_t renderTargetIndex, const DirectX::XMFLOAT4& clearColour
) noexcept {

}

void D3DExternalRenderPass::SetSwapchainCopySource(std::uint32_t renderTargetIndex) noexcept
{

}
