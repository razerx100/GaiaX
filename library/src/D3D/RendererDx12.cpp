#include <RendererDx12.hpp>
#include <Gaia.hpp>
#include <D3DHelperFunctions.hpp>
#include <D3DResourceBarrier.hpp>

RendererDx12::RendererDx12(
	const char* appName,
	void* windowHandle, std::uint32_t width, std::uint32_t height, std::uint32_t bufferCount,
	std::shared_ptr<ThreadPool>&& threadPool, RenderEngineType engineType
) : m_appName(appName), m_width(width), m_height(height), m_bufferCount{ bufferCount } {

	m_objectManager.CreateObject(Gaia::device, 3u);

	Gaia::device->GetDebugLogger().AddCallbackType(DebugCallbackType::FileOut);
	Gaia::device->Create();
	ID3D12Device4* deviceRef = Gaia::device.get()->GetDevice();

	Gaia::InitRenderEngine(m_objectManager, engineType, deviceRef, bufferCount);
	//Gaia::renderEngine->ResizeViewportAndScissor(width, height);

	//Gaia::InitResources(m_objectManager, threadPool);

	const bool meshDrawType = engineType == RenderEngineType::MeshDraw ? true : false;

	Gaia::InitGraphicsQueueAndList(m_objectManager, deviceRef, meshDrawType, bufferCount);

	/*
	SwapChainManager::Args swapChainArguments{
		.device = deviceRef,
		.factory = Gaia::device->GetFactory(),
		.graphicsQueue = Gaia::graphicsQueue->GetQueue(),
		.windowHandle = static_cast<HWND>(windowHandle),
		.width = width,
		.height = height,
		.bufferCount = bufferCount,
		.variableRefreshRate = true
	};
	*/

	//m_objectManager.CreateObject(Gaia::swapChain, 1u, swapChainArguments);

	Gaia::InitCopyQueueAndList(m_objectManager, deviceRef);
	Gaia::InitComputeQueueAndList(m_objectManager, deviceRef, bufferCount);

	//m_objectManager.CreateObject(Gaia::descriptorTable, 0u);

	const bool modelDataNoBB = engineType == RenderEngineType::IndirectDraw ? false : true;

	//m_objectManager.CreateObject(Gaia::bufferManager, 1u, bufferCount, modelDataNoBB, sharedContainer);
	//m_objectManager.CreateObject(Gaia::textureStorage, 0u);

	//m_objectManager.CreateObject(Gaia::cameraManager, 0u, sharedContainer);
	//Gaia::cameraManager->SetSceneResolution(width, height);
}

/*void RendererDx12::AddModelSet(
	std::vector<std::shared_ptr<Model>>&& models, const std::wstring& pixelShader
) {
	Gaia::renderEngine->RecordModelDataSet(models, pixelShader + L".cso");
	Gaia::bufferManager->AddOpaqueModels(std::move(models));
}

void RendererDx12::AddMeshletModelSet(
	std::vector<MeshletModel>&& meshletModels, const std::wstring& pixelShader
) {
	Gaia::renderEngine->AddMeshletModelSet(meshletModels, pixelShader + L".cso");
	Gaia::bufferManager->AddOpaqueModels(std::move(meshletModels));
}

void RendererDx12::AddModelInputs(
	std::vector<Vertex>&& gVertices, std::vector<std::uint32_t>&& gIndices
) {
	Gaia::renderEngine->AddGVerticesAndIndices(std::move(gVertices), std::move(gIndices));
}

void RendererDx12::AddModelInputs(
	std::vector<Vertex>&& gVertices, std::vector<std::uint32_t>&& gVerticesIndices,
	std::vector<std::uint32_t>&& gPrimIndices
) {
	Gaia::renderEngine->AddGVerticesAndPrimIndices(
		std::move(gVertices), std::move(gVerticesIndices), std::move(gPrimIndices)
	);
}*/

/*
void RendererDx12::Update() {
	const size_t currentBackIndex = Gaia::swapChain->GetCurrentBackBufferIndex();

	Gaia::renderEngine->UpdateModelBuffers(currentBackIndex);
}
*/

void RendererDx12::Render()
{
	/*
	const size_t currentBackIndex = Gaia::swapChain->GetCurrentBackBufferIndex();

	Gaia::renderEngine->ExecuteRenderStage(currentBackIndex);
	Gaia::renderEngine->Present(currentBackIndex);
	Gaia::renderEngine->ExecutePostRenderStage();
	*/
}

void RendererDx12::Resize(std::uint32_t width, std::uint32_t height) {
	if (m_width != width || m_height != height) {
		m_width = width;
		m_height = height;

		ID3D12Device* deviceRef = Gaia::device->GetDevice();

		//UINT64 fenceValue = Gaia::graphicsFence->GetFrontValue();

		//Gaia::graphicsQueue->SignalCommandQueue(Gaia::graphicsFence->GetFence(), fenceValue);
		//Gaia::graphicsFence->WaitOnCPU();

		//Gaia::swapChain->Resize(deviceRef, width, height);

		//Gaia::graphicsFence->ResetFenceValues(fenceValue + 1u);

		//Gaia::renderEngine->CreateDepthBufferView(deviceRef, width, height);
		//Gaia::renderEngine->ResizeViewportAndScissor(width, height);

		//Gaia::cameraManager->SetSceneResolution(width, height);
	}
}

Renderer::Resolution RendererDx12::GetFirstDisplayCoordinates() const {
	auto [width, height] = GetDisplayResolution(
		Gaia::device->GetDevice(), Gaia::device->GetFactory(), 0u
	);

	return { width, height };
}

void RendererDx12::SetBackgroundColour(const std::array<float, 4>& colour) noexcept {
	Gaia::renderEngine->SetBackgroundColour(colour);
}

void RendererDx12::SetShaderPath(const wchar_t* path)
{
	Gaia::renderEngine->SetShaderPath(path);
}

void RendererDx12::AddPixelShader(const ShaderName& pixelShader) {}
void RendererDx12::ChangePixelShader(std::uint32_t modelBundleID, const ShaderName& pixelShader) {}

/*
void RendererDx12::ProcessData() {
	ID3D12Device* device = Gaia::device->GetDeviceRef();

	// Reserve Heap Space start
	Gaia::renderEngine->ReserveBuffers(device);
	Gaia::bufferManager->ReserveBuffers(device);
	Gaia::Resources::cpuWriteBuffer->ReserveHeapSpace(device);
	// Reserve Heap Space end

	// Create heaps start
	Gaia::Resources::uploadHeap->CreateHeap(device);
	Gaia::Resources::gpuOnlyHeap->CreateHeap(device);
	Gaia::Resources::cpuWriteHeap->CreateHeap(device);
	// Create heaps end

	Gaia::descriptorTable->CreateDescriptorTable(device);

	// Create Buffers start
	Gaia::renderEngine->CreateDepthBufferView(device, m_width, m_height);
	Gaia::Resources::cpuWriteBuffer->CreateResource(device);
	Gaia::renderEngine->CreateBuffers(device);
	Gaia::bufferManager->CreateBuffers(device);
	Gaia::textureStorage->CreateBufferViews(device);
	// Create Buffers end

	// Async copy start
	std::atomic_size_t workCount = 0u;

	Gaia::Resources::uploadContainer->CopyData(workCount);

	Gaia::descriptorTable->CopyUploadHeap(device);

	while (workCount != 0u);
	// Async copy end

	// GPU upload start
	Gaia::copyCmdList->ResetFirst();
	ID3D12GraphicsCommandList* copyList = Gaia::copyCmdList->GetCommandList();

	Gaia::renderEngine->RecordResourceUploads(copyList);
	Gaia::textureStorage->RecordResourceUpload(copyList);

	Gaia::copyCmdList->Close();

	Gaia::copyQueue->ExecuteCommandLists(copyList);

	UINT64 fenceValue = Gaia::graphicsFence->GetFrontValue();
	Gaia::copyQueue->SignalCommandQueue(Gaia::graphicsFence->GetFence(), fenceValue);
	Gaia::graphicsFence->WaitOnCPU();
	Gaia::graphicsFence->SignalFence(fenceValue - 1u);
	// GPU upload end

	Gaia::renderEngine->ConstructPipelines();

	// Release Upload Resource start
	Gaia::renderEngine->ReleaseUploadResources();
	Gaia::textureStorage->ReleaseUploadResource();
	Gaia::descriptorTable->ReleaseUploadHeap();
	Gaia::Resources::uploadHeap.reset();
	// Release Upload Resource end
}
*/

size_t RendererDx12::AddTexture(STexture&& texture)
{
	return 0u;
	/*
	return Gaia::textureStorage->AddTexture(
		Gaia::device->GetDevice(), std::move(textureData), width, height
	);
	*/
}

void RendererDx12::UnbindTexture(size_t index) {}
std::uint32_t RendererDx12::BindTexture(size_t index) { return 0u; }
void RendererDx12::RemoveTexture(size_t index) {}

void RendererDx12::WaitForGPUToFinish()
{
	// Current frame's value is already checked. So, check the rest
	/*
	for (std::uint32_t _ = 0u; _ < m_bufferCount - 1u; ++_) {
		Gaia::graphicsFence->AdvanceValueInQueue();
		Gaia::graphicsFence->WaitOnCPUConditional();
		Gaia::computeFence->AdvanceValueInQueue();
		Gaia::computeFence->WaitOnCPUConditional();
	}
	*/
}

std::uint32_t RendererDx12::AddModelBundle(
	std::shared_ptr<ModelBundleVS>&& modelBundle, const ShaderName& pixelShader
) {
	return 0u;
}

std::uint32_t RendererDx12::AddModelBundle(
	std::shared_ptr<ModelBundleMS>&& modelBundle, const ShaderName& pixelShader
) {
	return 0u;
}

void RendererDx12::RemoveModelBundle(std::uint32_t bundleID) noexcept {}

std::uint32_t RendererDx12::AddMeshBundle(std::unique_ptr<MeshBundleVS> meshBundle)
{
	return 0u;
}

std::uint32_t RendererDx12::AddMeshBundle(std::unique_ptr<MeshBundleMS> meshBundle)
{
	return 0u;
}

void RendererDx12::RemoveMeshBundle(std::uint32_t bundleIndex) noexcept {}

size_t RendererDx12::AddMaterial(std::shared_ptr<Material> material)
{
	return 0u;
}

std::vector<size_t> RendererDx12::AddMaterials(std::vector<std::shared_ptr<Material>>&& materials)
{
	return {};
}

void RendererDx12::UpdateMaterial(size_t index) const noexcept {}

void RendererDx12::RemoveMaterial(size_t index) noexcept {}

std::uint32_t RendererDx12::AddCamera(std::shared_ptr<Camera>&& camera) noexcept
{
	return 0u;
}

void RendererDx12::SetCamera(std::uint32_t index) noexcept {}

void RendererDx12::RemoveCamera(std::uint32_t index) noexcept {}
