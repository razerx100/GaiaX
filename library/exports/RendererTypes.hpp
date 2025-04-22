#ifndef RENDERER_TYPES_HPP_
#define RENDERER_TYPES_HPP_

enum class RenderEngineType
{
	IndirectDraw,
	IndividualDraw,
	MeshDraw
};

struct SolExtent
{
	std::uint32_t width;
	std::uint32_t height;
};
#endif
