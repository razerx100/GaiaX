#include<RenderEngineVertex.hpp>
#include <Gaia.hpp>
#include <D3DResourceBarrier.hpp>

void RenderEngineVertex::ExecutePreRenderStage(
	ID3D12GraphicsCommandList* graphicsCommandList, size_t frameIndex, float* clearColour
) {
	// Compute Stage
	ID3D12GraphicsCommandList* computeCommandList = Gaia::computeCmdList->GetCommandList();
	Gaia::computeCmdList->Reset(frameIndex);
	// Need some barrier stuff

	ID3D12DescriptorHeap* ppHeap[] = { Gaia::descriptorTable->GetDescHeapRef() };
	computeCommandList->SetDescriptorHeaps(1u, ppHeap);

	// Record compute commands
	Gaia::renderPipeline->ResetCounterBuffer(computeCommandList, frameIndex);
	Gaia::renderPipeline->BindComputePipeline(computeCommandList);
	Gaia::bufferManager->BindBuffersToCompute(computeCommandList, frameIndex);
	Gaia::renderPipeline->DispatchCompute(computeCommandList, frameIndex);

	Gaia::computeCmdList->Close();
	Gaia::computeQueue->ExecuteCommandLists(computeCommandList);

	UINT64 fenceValue = Gaia::graphicsFence->GetFrontValue();

	Gaia::computeQueue->SignalCommandQueue(Gaia::computeFence->GetFence(), fenceValue);

	// Graphics Stage
	Gaia::graphicsQueue->WaitOnGPU(Gaia::computeFence->GetFence(), fenceValue);

	Gaia::graphicsCmdList->Reset(frameIndex);

	D3DResourceBarrier<2u>().AddBarrier(
		Gaia::swapChain->GetRTV(frameIndex),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET
	).AddBarrier(
		Gaia::renderPipeline->GetArgumentBuffer(frameIndex),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT
	).RecordBarriers(graphicsCommandList);

	graphicsCommandList->SetDescriptorHeaps(1u, ppHeap);

	graphicsCommandList->RSSetViewports(1u, Gaia::viewportAndScissor->GetViewportRef());
	graphicsCommandList->RSSetScissorRects(1u, Gaia::viewportAndScissor->GetScissorRef());

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = Gaia::swapChain->GetRTVHandle(frameIndex);

	Gaia::swapChain->ClearRTV(graphicsCommandList, clearColour, rtvHandle);

	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = Gaia::Resources::depthBuffer->GetDSVHandle();

	Gaia::Resources::depthBuffer->ClearDSV(graphicsCommandList, dsvHandle);

	graphicsCommandList->OMSetRenderTargets(1u, &rtvHandle, FALSE, &dsvHandle);
}

void RenderEngineVertex::RecordDrawCommands(
	ID3D12GraphicsCommandList* graphicsCommandList, size_t frameIndex
) {
	Gaia::renderPipeline->BindGraphicsPipeline(graphicsCommandList);
	Gaia::textureStorage->BindTextures(graphicsCommandList);
	Gaia::bufferManager->BindBuffersToGraphics(graphicsCommandList, frameIndex);
	Gaia::bufferManager->BindVertexBuffer(graphicsCommandList);
	Gaia::renderPipeline->DrawModels(graphicsCommandList, frameIndex);
}

void RenderEngineVertex::Present(
	ID3D12GraphicsCommandList* graphicsCommandList, size_t frameIndex
) {
	D3DResourceBarrier().AddBarrier(
		Gaia::swapChain->GetRTV(frameIndex),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT
	).RecordBarriers(graphicsCommandList);

	Gaia::graphicsCmdList->Close();
	Gaia::graphicsQueue->ExecuteCommandLists(graphicsCommandList);

	Gaia::swapChain->PresentWithTear();
}

void RenderEngineVertex::ExecutePostRenderStage() {
	UINT64 fenceValue = Gaia::graphicsFence->GetFrontValue();

	Gaia::graphicsQueue->SignalCommandQueue(Gaia::graphicsFence->GetFence(), fenceValue);
	Gaia::graphicsFence->AdvanceValueInQueue();
	Gaia::graphicsFence->WaitOnCPUConditional();
	Gaia::graphicsFence->IncreaseFrontValue(fenceValue);
}
