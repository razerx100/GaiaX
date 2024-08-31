#include <RenderEngineBase.hpp>
#include <Gaia.hpp>
#include <D3DResourceBarrier.hpp>

RenderEngineBase::RenderEngineBase(ID3D12Device* device) : m_depthBuffer{ device } {
	m_depthBuffer.SetMaxResolution(7680u, 4320u);
}

void RenderEngineBase::Present(size_t frameIndex) {
	ID3D12GraphicsCommandList* graphicsCommandList = Gaia::graphicsCmdList->GetCommandList();

	D3DResourceBarrier().AddBarrier(
		ResourceBarrierBuilder{}.Transition(
			Gaia::swapChain->GetRTV(frameIndex),
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT
		)
	).RecordBarriers(graphicsCommandList);

	Gaia::graphicsCmdList->Close();
	Gaia::graphicsQueue->ExecuteCommandLists(graphicsCommandList);

	Gaia::swapChain->PresentWithTear();
}

void RenderEngineBase::ExecutePostRenderStage() {
	UINT64 fenceValue = Gaia::graphicsFence->GetFrontValue();

	Gaia::graphicsQueue->SignalCommandQueue(Gaia::graphicsFence->GetFence(), fenceValue);
	Gaia::graphicsFence->AdvanceValueInQueue();
	Gaia::graphicsFence->WaitOnCPUConditional();
	Gaia::graphicsFence->IncreaseFrontValue(fenceValue);
}

void RenderEngineBase::ExecutePreGraphicsStage(
	ID3D12GraphicsCommandList* graphicsCommandList, size_t frameIndex
) {
	Gaia::graphicsCmdList->Reset(frameIndex);

	D3DResourceBarrier().AddBarrier(
		ResourceBarrierBuilder{}.Transition(
			Gaia::swapChain->GetRTV(frameIndex),
			D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET
		)
	).RecordBarriers(graphicsCommandList);

	ID3D12DescriptorHeap* descriptorHeap[] = { Gaia::descriptorTable->GetDescHeapRef() };
	graphicsCommandList->SetDescriptorHeaps(1u, descriptorHeap);

	graphicsCommandList->RSSetViewports(1u, m_viewportAndScissor.GetViewportRef());
	graphicsCommandList->RSSetScissorRects(1u, m_viewportAndScissor.GetScissorRef());

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = Gaia::swapChain->GetRTVHandle(frameIndex);

	Gaia::swapChain->ClearRTV(
		graphicsCommandList, std::data(m_backgroundColour), rtvHandle
	);

	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = m_depthBuffer.GetDSVHandle();

	m_depthBuffer.ClearDSV(graphicsCommandList, dsvHandle);

	graphicsCommandList->OMSetRenderTargets(1u, &rtvHandle, FALSE, &dsvHandle);
}

void RenderEngineBase::BindCommonGraphicsBuffers(
	ID3D12GraphicsCommandList* graphicsCommandList, size_t frameIndex
) {
	Gaia::textureStorage->BindTextures(graphicsCommandList);
	/*
	Gaia::bufferManager->BindBuffersToGraphics(graphicsCommandList, frameIndex);
	Gaia::bufferManager->BindPixelOnlyBuffers(graphicsCommandList, frameIndex);
	*/
}

void RenderEngineBase::ConstructGraphicsRootSignature(ID3D12Device* device) {
	/*
	auto graphicsRS = CreateGraphicsRootSignature(device);

	m_graphicsRSLayout = graphicsRS->GetElementLayout();
	m_graphicsRS = std::move(graphicsRS);

	Gaia::bufferManager->SetGraphicsRootSignatureLayout(m_graphicsRSLayout);
	Gaia::textureStorage->SetGraphicsRootSignatureLayout(m_graphicsRSLayout);
	*/
}

void RenderEngineBase::ResizeViewportAndScissor(
	std::uint32_t width, std::uint32_t height
) noexcept {
	m_viewportAndScissor.Resize(width, height);
}

void RenderEngineBase::CreateDepthBufferView(
	ID3D12Device* device, std::uint32_t width, std::uint32_t height
) {
	m_depthBuffer.CreateDepthBufferView(device, width, height);
}

void RenderEngineBase::ReserveBuffers(ID3D12Device* device) {
	m_depthBuffer.ReserveHeapSpace(device);
	ReserveBuffersDerived(device);
}

void RenderEngineBase::ReserveBuffersDerived([[maybe_unused]] ID3D12Device* device) {}

void RenderEngineBase::AddGVerticesAndIndices(
	[[maybe_unused]] std::vector<Vertex>&& gVertices,
	[[maybe_unused]] std::vector<std::uint32_t>&& gIndices
) noexcept {}

void RenderEngineBase::RecordModelDataSet(
	[[maybe_unused]] const std::vector<std::shared_ptr<Model>>& models,
	[[maybe_unused]] const std::wstring& pixelShader
) noexcept {}

/*void RenderEngineBase::AddMeshletModelSet(
	[[maybe_unused]] std::vector<MeshletModel>& meshletModels,
	[[maybe_unused]] const std::wstring& pixelShader
) noexcept {} */

void RenderEngineBase::AddGVerticesAndPrimIndices(
	[[maybe_unused]] std::vector<Vertex>&& gVertices,
	[[maybe_unused]] std::vector<std::uint32_t>&& gVerticesIndices,
	[[maybe_unused]] std::vector<std::uint32_t>&& gPrimIndices
) noexcept {}
