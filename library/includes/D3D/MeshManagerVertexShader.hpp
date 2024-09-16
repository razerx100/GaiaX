#ifndef MESH_MANAGER_VERTEX_SHADER_HPP_
#define MESH_MANAGER_VERTEX_SHADER_HPP_
#include <D3DHeaders.hpp>
#include <memory>
#include <D3DResources.hpp>
#include <StagingBufferManager.hpp>
#include <D3DCommandQueue.hpp>
#include <CommonBuffers.hpp>

#include <MeshBundle.hpp>

class MeshManagerVertexShader
{
public:
	struct BoundsDetails
	{
		std::uint32_t offset;
		std::uint32_t count;
	};

public:
	MeshManagerVertexShader();

	// Without bound data.
	void SetMeshBundle(
		std::unique_ptr<MeshBundleVS> meshBundle, StagingBufferManager& stagingBufferMan,
		SharedBufferGPU& vertexSharedBuffer, SharedBufferGPU& indexSharedBuffer,
		TemporaryDataBufferGPU& tempBuffer
	);
	// With bound data when the bound data has shared ownership.
	void SetMeshBundle(
		std::unique_ptr<MeshBundleVS> meshBundle, StagingBufferManager& stagingBufferMan,
		SharedBufferGPU& vertexSharedBuffer, SharedBufferGPU& indexSharedBuffer,
		SharedBufferGPU& boundsSharedBuffer, TemporaryDataBufferGPU& tempBuffer
	);

	void Bind(const D3DCommandList& graphicsCmdList) const noexcept;

	[[nodiscard]]
	const SharedBufferData& GetVertexSharedData() const noexcept { return m_vertexBufferSharedData; }
	[[nodiscard]]
	const SharedBufferData& GetIndexSharedData() const noexcept { return m_indexBufferSharedData; }
	[[nodiscard]]
	const SharedBufferData& GetBoundsSharedData() const noexcept { return m_meshBoundsSharedData; }

	[[nodiscard]]
	BoundsDetails GetBoundsDetails() const noexcept;

private:
	SharedBufferData m_vertexBufferSharedData;
	SharedBufferData m_indexBufferSharedData;
	SharedBufferData m_meshBoundsSharedData;

public:
	MeshManagerVertexShader(const MeshManagerVertexShader&) = delete;
	MeshManagerVertexShader& operator=(const MeshManagerVertexShader&) = delete;

	MeshManagerVertexShader(MeshManagerVertexShader&& other) noexcept
		: m_vertexBufferSharedData{ other.m_vertexBufferSharedData },
		m_indexBufferSharedData{ other.m_indexBufferSharedData },
		m_meshBoundsSharedData{ other.m_meshBoundsSharedData }
	{}
	MeshManagerVertexShader& operator=(MeshManagerVertexShader&& other) noexcept
	{
		m_vertexBufferSharedData = other.m_vertexBufferSharedData;
		m_indexBufferSharedData  = other.m_indexBufferSharedData;
		m_meshBoundsSharedData   = other.m_meshBoundsSharedData;

		return *this;
	}
};
#endif
