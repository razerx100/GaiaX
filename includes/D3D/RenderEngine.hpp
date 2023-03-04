#ifndef RENDER_ENGINE_
#define RENDER_ENGINE_
#include <D3DHeaders.hpp>
#include <vector>
#include <array>
#include <memory>
#include <string>
#include <IModel.hpp>

class RenderEngine {
public:
	RenderEngine() noexcept;
	virtual ~RenderEngine() = default;

	virtual void ExecutePreRenderStage(
		ID3D12GraphicsCommandList* graphicsCommandList, size_t frameIndex
	) = 0;
	virtual void RecordDrawCommands(
		ID3D12GraphicsCommandList* graphicsCommandList, size_t frameIndex
	) = 0;
	virtual void Present(ID3D12GraphicsCommandList* graphicsCommandList, size_t frameIndex) = 0;
	virtual void ExecutePostRenderStage() = 0;
	virtual void ConstructPipelines() = 0;


	virtual void CreateDepthBufferView(
		ID3D12Device* device, std::uint32_t width, std::uint32_t height
	) = 0;
	virtual void ResizeViewportAndScissor(
		std::uint32_t width, std::uint32_t height
	) noexcept = 0;
	virtual void AddGVerticesAndIndices(
		std::vector<Vertex>&& gVertices, std::vector<std::uint32_t>&& gIndices
	) noexcept = 0;
	virtual void RecordModelDataSet(
		const std::vector<std::shared_ptr<IModel>>& models, const std::wstring& pixelShader
	) noexcept = 0;
	virtual void CreateBuffers(ID3D12Device* device) = 0;
	virtual void ReserveBuffers(ID3D12Device* device) = 0;
	virtual void RecordResourceUploads(ID3D12GraphicsCommandList* copyList) noexcept = 0;
	virtual void ReleaseUploadResources() noexcept = 0;
	virtual void CopyData(std::atomic_size_t& workCount) const noexcept = 0;

	void SetBackgroundColour(const std::array<float, 4>& colour) noexcept;
	void SetShaderPath(const wchar_t* path) noexcept;

protected:
	std::array<float, 4> m_backgroundColour;
	std::wstring m_shaderPath;
};
#endif
