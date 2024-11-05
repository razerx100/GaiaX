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
	struct PerMeshBundleData
	{
		std::uint32_t meshOffset;
	};

public:
	MeshManagerVertexShader();

	void SetMeshBundle(
		std::unique_ptr<MeshBundleVS> meshBundle, StagingBufferManager& stagingBufferMan,
		SharedBufferGPU& vertexSharedBuffer, SharedBufferGPU& indexSharedBuffer,
		TemporaryDataBufferGPU& tempBuffer
	);
	void SetMeshBundle(
		std::unique_ptr<MeshBundleVS> meshBundle, StagingBufferManager& stagingBufferMan,
		SharedBufferGPU& vertexSharedBuffer, SharedBufferGPU& indexSharedBuffer,
		SharedBufferGPU& perMeshSharedBuffer, SharedBufferGPU& perMeshBundleSharedBuffer,
		TemporaryDataBufferGPU& tempBuffer
	);

	void Bind(const D3DCommandList& graphicsCmdList) const noexcept;

	[[nodiscard]]
	const SharedBufferData& GetVertexSharedData() const noexcept { return m_vertexBufferSharedData; }
	[[nodiscard]]
	const SharedBufferData& GetIndexSharedData() const noexcept { return m_indexBufferSharedData; }
	[[nodiscard]]
	const SharedBufferData& GetPerMeshSharedData() const noexcept { return m_perMeshSharedData; }
	[[nodiscard]]
	const SharedBufferData& GetPerMeshBundleSharedData() const noexcept
	{
		return m_perMeshBundleSharedData;
	}

	[[nodiscard]]
	const MeshDetails& GetMeshDetails(size_t index) const noexcept
	{
		return m_bundleDetails.meshDetails[index];
	}

private:
	SharedBufferData  m_vertexBufferSharedData;
	SharedBufferData  m_indexBufferSharedData;
	SharedBufferData  m_perMeshSharedData;
	SharedBufferData  m_perMeshBundleSharedData;
	MeshBundleDetails m_bundleDetails;

public:
	MeshManagerVertexShader(const MeshManagerVertexShader&) = delete;
	MeshManagerVertexShader& operator=(const MeshManagerVertexShader&) = delete;

	MeshManagerVertexShader(MeshManagerVertexShader&& other) noexcept
		: m_vertexBufferSharedData{ other.m_vertexBufferSharedData },
		m_indexBufferSharedData{ other.m_indexBufferSharedData },
		m_perMeshSharedData{ other.m_perMeshSharedData },
		m_perMeshBundleSharedData{ other.m_perMeshBundleSharedData },
		m_bundleDetails{ std::move(other.m_bundleDetails) }
	{}
	MeshManagerVertexShader& operator=(MeshManagerVertexShader&& other) noexcept
	{
		m_vertexBufferSharedData  = other.m_vertexBufferSharedData;
		m_indexBufferSharedData   = other.m_indexBufferSharedData;
		m_perMeshSharedData       = other.m_perMeshSharedData;
		m_perMeshBundleSharedData = other.m_perMeshBundleSharedData;
		m_bundleDetails           = std::move(other.m_bundleDetails);

		return *this;
	}
};
#endif
