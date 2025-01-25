#ifndef D3D_RENDER_PASS_MANAGER_HPP_
#define D3D_RENDER_PASS_MANAGER_HPP_
#include <memory>
#include <PipelineManager.hpp>
#include <DepthBuffer.hpp>
#include <D3DRenderTarget.hpp>

template<typename Pipeline_t>
class D3DRenderPassManager
{
public:
	D3DRenderPassManager(ID3D12Device5* device)
		: m_graphicsPipelineManager{ device }, m_depthBuffer{}, m_rtvFormat{ DXGI_FORMAT_UNKNOWN }
	{}

	void SetShaderPath(const std::wstring& shaderPath) noexcept
	{
		m_graphicsPipelineManager.SetShaderPath(shaderPath);
	}

	void SetRootSignature(ID3D12RootSignature* rootSignature) noexcept
	{
		m_graphicsPipelineManager.SetRootSignature(rootSignature);
	}

	void SetRTVFormat(DXGI_FORMAT rtvFormat) noexcept
	{
		m_rtvFormat = rtvFormat;
	}

	void SetDepthTesting(
		ID3D12Device5* device, MemoryManager* memoryManager, D3DReusableDescriptorHeap* dsvHeap
	) {
		m_depthBuffer = std::make_unique<DepthBuffer>(device, memoryManager, dsvHeap);
	}

	void SetPSOOverwritable(const ShaderName& pixelShader) noexcept
	{
		m_graphicsPipelineManager.SetOverwritable(pixelShader);
	}

	void ResizeDepthBuffer(UINT width, UINT height)
	{
		if (m_depthBuffer)
			m_depthBuffer->Create(width, height);
	}

	void BeginRenderingWithDepth(
		const D3DCommandList& graphicsCmdList, const RenderTarget& renderTarget,
		const std::array<float, 4u>& clearValues
	) const noexcept {
		renderTarget.ToRenderState(graphicsCmdList);

		const D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = m_depthBuffer->ClearDSV(graphicsCmdList);

		renderTarget.Set(graphicsCmdList, clearValues, &dsvHandle);
	}

	void BeginRendering(
		const D3DCommandList& graphicsCmdList, const RenderTarget& renderTarget,
		const std::array<float, 4u>& clearValues
	) const noexcept {
		renderTarget.ToRenderState(graphicsCmdList);

		renderTarget.Set(graphicsCmdList, clearValues, nullptr);
	}

	void EndRendering(
		const D3DCommandList& graphicsCmdList, const RenderTarget& renderTarget
	) const noexcept {
		renderTarget.ToPresentState(graphicsCmdList);
	}

	std::uint32_t AddOrGetGraphicsPipeline(const ShaderName& pixelShader)
	{
		return m_graphicsPipelineManager.AddOrGetGraphicsPipeline(
			pixelShader, m_rtvFormat, GetDSVFormat()
		);
	}

	void RecreatePipelines()
	{
		m_graphicsPipelineManager.RecreateAllGraphicsPipelines(m_rtvFormat, GetDSVFormat());
	}

	[[nodiscard]]
	const PipelineManager<Pipeline_t>& GetGraphicsPipelineManager() const noexcept
	{
		return m_graphicsPipelineManager;
	}

private:
	[[nodiscard]]
	DXGI_FORMAT GetDSVFormat() const noexcept
	{
		DXGI_FORMAT dsvFormat = DXGI_FORMAT_UNKNOWN;

		if (m_depthBuffer)
			dsvFormat = m_depthBuffer->GetFormat();

		return dsvFormat;
	}

private:
	PipelineManager<Pipeline_t>  m_graphicsPipelineManager;
	std::unique_ptr<DepthBuffer> m_depthBuffer;
	DXGI_FORMAT                  m_rtvFormat;

public:
	D3DRenderPassManager(const D3DRenderPassManager&) = delete;
	D3DRenderPassManager& operator=(const D3DRenderPassManager&) = delete;

	D3DRenderPassManager(D3DRenderPassManager&& other) noexcept
		: m_graphicsPipelineManager{ std::move(other.m_graphicsPipelineManager) },
		m_depthBuffer{ std::move(other.m_depthBuffer) },
		m_rtvFormat{ other.m_rtvFormat }
	{}
	D3DRenderPassManager& operator=(D3DRenderPassManager&& other) noexcept
	{
		m_graphicsPipelineManager = std::move(other.m_graphicsPipelineManager);
		m_depthBuffer             = std::move(other.m_depthBuffer);
		m_rtvFormat               = other.m_rtvFormat;

		return *this;
	}
};
#endif
