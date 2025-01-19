#include <RendererDx12.hpp>

RendererDx12::RendererDx12(
	[[maybe_unused]] const char* appName,
	void* windowHandle, std::uint32_t width, std::uint32_t height, std::uint32_t bufferCount,
	std::shared_ptr<ThreadPool>&& threadPool, RenderEngineType engineType
) : m_gaia{
		windowHandle, width, height, bufferCount, std::move(threadPool), engineType
	}
{}

void RendererDx12::FinaliseInitialisation()
{
	m_gaia.FinaliseInitialisation();
}

void RendererDx12::Render()
{
	m_gaia.Render();
}

void RendererDx12::Resize(std::uint32_t width, std::uint32_t height)
{
	m_gaia.Resize(width, height);
}

Renderer::Resolution RendererDx12::GetFirstDisplayCoordinates() const
{
	auto [width, height] = m_gaia.GetFirstDisplayCoordinates();

	return Renderer::Resolution{ .width = width, .height = height };
}

void RendererDx12::SetBackgroundColour(const std::array<float, 4>& colour) noexcept
{
	m_gaia.GetRenderEngine().SetBackgroundColour(colour);
}

void RendererDx12::SetShaderPath(const wchar_t* path)
{
	m_gaia.GetRenderEngine().SetShaderPath(path);
}

void RendererDx12::AddPixelShader(const ShaderName& pixelShader)
{
	m_gaia.GetRenderEngine().AddPixelShader(pixelShader);
}

void RendererDx12::ChangePixelShader(std::uint32_t modelBundleID, const ShaderName& pixelShader)
{
	m_gaia.GetRenderEngine().ChangePixelShader(modelBundleID, pixelShader);
}

size_t RendererDx12::AddTexture(STexture&& texture)
{
	return m_gaia.GetRenderEngine().AddTexture(std::move(texture));
}

void RendererDx12::UnbindTexture(size_t index)
{
	m_gaia.GetRenderEngine().UnbindTexture(index);
}

std::uint32_t RendererDx12::BindTexture(size_t index)
{
	return m_gaia.GetRenderEngine().BindTexture(index);
}

void RendererDx12::RemoveTexture(size_t index)
{
	m_gaia.GetRenderEngine().RemoveTexture(index);
}

void RendererDx12::WaitForGPUToFinish()
{
	m_gaia.GetRenderEngine().WaitForGPUToFinish();
}

std::uint32_t RendererDx12::AddModelBundle(
	std::shared_ptr<ModelBundle>&& modelBundle, const ShaderName& pixelShader
) {
	return m_gaia.GetRenderEngine().AddModelBundle(std::move(modelBundle), pixelShader);
}

void RendererDx12::RemoveModelBundle(std::uint32_t bundleID) noexcept
{
	m_gaia.GetRenderEngine().RemoveModelBundle(bundleID);
}

std::uint32_t RendererDx12::AddMeshBundle(std::unique_ptr<MeshBundleTemporary> meshBundle)
{
	return m_gaia.GetRenderEngine().AddMeshBundle(std::move(meshBundle));
}

void RendererDx12::RemoveMeshBundle(std::uint32_t bundleIndex) noexcept
{
	m_gaia.GetRenderEngine().RemoveMeshBundle(bundleIndex);
}

size_t RendererDx12::AddMaterial(std::shared_ptr<Material> material)
{
	return m_gaia.GetRenderEngine().AddMaterial(std::move(material));
}

std::vector<size_t> RendererDx12::AddMaterials(std::vector<std::shared_ptr<Material>>&& materials)
{
	return m_gaia.GetRenderEngine().AddMaterials(std::move(materials));
}

void RendererDx12::UpdateMaterial(size_t index) const noexcept
{
	m_gaia.GetRenderEngine().UpdateMaterial(index);
}

void RendererDx12::RemoveMaterial(size_t index) noexcept
{
	m_gaia.GetRenderEngine().RemoveMaterial(index);
}

std::uint32_t RendererDx12::AddCamera(std::shared_ptr<Camera>&& camera) noexcept
{
	return m_gaia.GetRenderEngine().AddCamera(std::move(camera));
}

void RendererDx12::SetCamera(std::uint32_t index) noexcept
{
	m_gaia.GetRenderEngine().SetCamera(index);
}

void RendererDx12::RemoveCamera(std::uint32_t index) noexcept
{
	m_gaia.GetRenderEngine().RemoveCamera(index);
}

ExternalResourceManager* RendererDx12::GetExternalResourceManager() noexcept
{
	return m_gaia.GetRenderEngine().GetExternalResourceManager();
}

void RendererDx12::UpdateExternalBufferDescriptor(const ExternalBufferBindingDetails& bindingDetails)
{
	m_gaia.GetRenderEngine().UpdateExternalBufferDescriptor(bindingDetails);
}
