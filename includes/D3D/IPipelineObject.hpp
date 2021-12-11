#ifndef __I_PIPELINE_OBJECT_HPP__
#define __I_PIPELINE_OBJECT_HPP__
#include <D3DHeaders.hpp>

class IPipelineObject {
public:
	virtual ~IPipelineObject() = default;

	virtual ID3D12PipelineState* Get() const noexcept = 0;
};
#endif
