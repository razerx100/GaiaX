#ifndef RENDERER_DX12_HPP_
#define RENDERER_DX12_HPP_
#include <Renderer.hpp>
#include <string>
#include <ThreadPool.hpp>
#include <Gaia.hpp>

class RendererDx12 final : public Renderer
{
public:
	RendererDx12(
		const char* appName,
		void* windowHandle, std::uint32_t width, std::uint32_t height, std::uint32_t bufferCount,
		std::shared_ptr<ThreadPool>&& threadPool, RenderEngineType engineType
	);

	void FinaliseInitialisation() override;

	void Resize(std::uint32_t width, std::uint32_t height) override;

	[[nodiscard]]
	Resolution GetFirstDisplayCoordinates() const override;

	void SetBackgroundColour(const std::array<float, 4>& colour) noexcept override;
	void SetShaderPath(const wchar_t* path) override;
	void AddPixelShader(const ShaderName& pixelShader) override;
	void ChangePixelShader(std::uint32_t modelBundleID, const ShaderName& pixelShader) override;
	void MakePixelShaderRemovable(const ShaderName& pixelShader) noexcept override;

	[[nodiscard]]
	// The returned Index is the texture's ID. Not its index in the shader. It should be
	// used to remove or bind the texture.
	size_t AddTexture(STexture&& texture) override;
	void UnbindTexture(size_t index) override;
	[[nodiscard]]
	// The returned index is the index of the texture in the shader.
	std::uint32_t BindTexture(size_t index) override;
	void RemoveTexture(size_t index) override;

	[[nodiscard]]
	std::uint32_t AddModelBundle(
		std::shared_ptr<ModelBundle>&& modelBundle, const ShaderName& pixelShader
	) override;

	void RemoveModelBundle(std::uint32_t bundleID) noexcept override;

	[[nodiscard]]
	std::uint32_t AddMeshBundle(std::unique_ptr<MeshBundleTemporary> meshBundle) override;
	void RemoveMeshBundle(std::uint32_t bundleIndex) noexcept override;

	[[nodiscard]]
	std::uint32_t AddCamera(std::shared_ptr<Camera>&& camera) noexcept override;
	void SetCamera(std::uint32_t index) noexcept override;
	void RemoveCamera(std::uint32_t index) noexcept override;

	[[nodiscard]]
	ExternalResourceManager* GetExternalResourceManager() noexcept override;

	void UpdateExternalBufferDescriptor(const ExternalBufferBindingDetails& bindingDetails) override;

	void UploadExternalBufferGPUOnlyData(
		std::uint32_t externalBufferIndex, std::shared_ptr<void> cpuData, size_t srcDataSizeInBytes,
		size_t dstBufferOffset
	) override;
	void QueueExternalBufferGPUCopy(
		std::uint32_t externalBufferSrcIndex, std::uint32_t externalBufferDstIndex,
		size_t dstBufferOffset, size_t srcBufferOffset = 0, size_t srcDataSizeInBytes = 0
	) override;

	void Render() override;
	void WaitForGPUToFinish() override;

private:
	Gaia m_gaia;

public:
	RendererDx12(const RendererDx12&) = delete;
	RendererDx12& operator=(const RendererDx12&) = delete;

	RendererDx12(RendererDx12&& other) noexcept
		: m_gaia{ std::move(other.m_gaia) }
	{}
	RendererDx12& operator=(RendererDx12&& other) noexcept
	{
		m_gaia = std::move(other.m_gaia);

		return *this;
	}
};
#endif
