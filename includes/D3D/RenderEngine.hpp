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

	virtual void InitiatePipelines(std::uint32_t bufferCount) noexcept = 0;
	virtual void ExecutePreRenderStage(
		ID3D12GraphicsCommandList* graphicsCommandList, size_t frameIndex
	) = 0;
	virtual void RecordDrawCommands(
		ID3D12GraphicsCommandList* graphicsCommandList, size_t frameIndex
	) = 0;
	virtual void Present(ID3D12GraphicsCommandList* graphicsCommandList, size_t frameIndex) = 0;
	virtual void ExecutePostRenderStage() = 0;
	virtual void ConstructPipelines() = 0;

	virtual void RecordModelData(
		const std::vector<std::shared_ptr<IModel>>& models
	) noexcept = 0;
	virtual void CreateBuffers(ID3D12Device* device) = 0;
	virtual void ReserveBuffers(ID3D12Device* device) = 0;
	virtual void RecordResourceUploads(ID3D12GraphicsCommandList* copyList) noexcept = 0;
	virtual void ReleaseUploadResources() noexcept = 0;

	void SetBackgroundColour(const std::array<float, 4>& colour) noexcept;
	void SetShaderPath(const wchar_t* path) noexcept;

protected:
	std::array<float, 4> m_backgroundColour;
	std::wstring m_shaderPath;
};
#endif
