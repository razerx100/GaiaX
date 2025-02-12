#ifndef EXTERNAL_PIPELINE_HPP_
#define EXTERNAL_PIPELINE_HPP_
#include <Shader.hpp>

class ExternalPipeline
{
public:
	virtual ~ExternalPipeline() = default;

	virtual void EnableDepthTesting(bool value) noexcept = 0;
	virtual void EnableStencilTesting(bool value) noexcept = 0;
	virtual void EnableAlphaBlending(bool value) noexcept = 0;
	virtual void EnableBackfaceCulling(bool value) noexcept = 0;
	virtual void SetRenderTarget(std::uint32_t count) noexcept = 0;

	[[nodiscard]]
	virtual const ShaderName& GetFragmentShader() const noexcept = 0;
	[[nodiscard]]
	virtual std::uint32_t GetRenderTargetCount() const noexcept = 0;
};
#endif
