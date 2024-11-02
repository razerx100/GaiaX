#ifndef MESH_MANAGER_MESH_SHADER_HPP_
#define MESH_MANAGER_MESH_SHADER_HPP_
#include <D3DHeaders.hpp>
#include <memory>
#include <D3DResources.hpp>
#include <StagingBufferManager.hpp>
#include <CommonBuffers.hpp>

#include <MeshBundle.hpp>

class MeshManagerMeshShader
{
public:
	// The offset should be enough to identify the mesh. We wouldn't really need the size
	// as the meshlets should already have that data.
	struct MeshDetailsMS
	{
		std::uint32_t vertexOffset;
		std::uint32_t vertexIndicesOffset;
		std::uint32_t primIndicesOffset;
		std::uint32_t meshletOffset;
	};

public:
	MeshManagerMeshShader();

	// Without bound data.
	void SetMeshBundle(
		std::unique_ptr<MeshBundleMS> meshBundle, StagingBufferManager& stagingBufferMan,
		SharedBufferGPU& vertexSharedBuffer, SharedBufferGPU& vertexIndicesSharedBuffer,
		SharedBufferGPU& primIndicesSharedBuffer, SharedBufferGPU& meshletSharedBuffer,
		TemporaryDataBufferGPU& tempBuffer
	);
	// With bound data when the bound data has shared ownership.
	void SetMeshBundle(
		std::unique_ptr<MeshBundleMS> meshBundle, StagingBufferManager& stagingBufferMan,
		SharedBufferGPU& vertexSharedBuffer, SharedBufferGPU& vertexIndicesSharedBuffer,
		SharedBufferGPU& primIndicesSharedBuffer, SharedBufferGPU& meshletSharedBuffer,
		SharedBufferGPU& boundsSharedBuffer, TemporaryDataBufferGPU& tempBuffer
	);

	[[nodiscard]]
	const SharedBufferData& GetVertexSharedData() const noexcept { return m_vertexBufferSharedData; }
	[[nodiscard]]
	const SharedBufferData& GetVertexIndicesSharedData() const noexcept
	{
		return m_vertexIndicesBufferSharedData;
	}
	[[nodiscard]]
	const SharedBufferData& GetPrimIndicesSharedData() const noexcept
	{
		return m_primIndicesBufferSharedData;
	}
	[[nodiscard]]
	const SharedBufferData& GetMeshletSharedData() const noexcept
	{
		return m_meshletBufferSharedData;
	}
	[[nodiscard]]
	const SharedBufferData& GetBoundsSharedData() const noexcept { return m_meshBoundsSharedData; }

	[[nodiscard]]
	MeshDetailsMS GetMeshDetailsMS() const noexcept { return m_meshDetails; }
	[[nodiscard]]
	const MeshDetails& GetMeshDetails(size_t index) const noexcept
	{
		return m_bundleDetails.meshDetails[index];
	}

	[[nodiscard]]
	static consteval UINT GetConstantCount() noexcept
	{
		return static_cast<UINT>(sizeof(MeshDetails) / sizeof(UINT));
	}

private:
	SharedBufferData  m_vertexBufferSharedData;
	SharedBufferData  m_vertexIndicesBufferSharedData;
	SharedBufferData  m_primIndicesBufferSharedData;
	SharedBufferData  m_meshletBufferSharedData;
	SharedBufferData  m_meshBoundsSharedData;
	MeshDetailsMS     m_meshDetails;
	MeshBundleDetails m_bundleDetails;

public:
	MeshManagerMeshShader(const MeshManagerMeshShader&) = delete;
	MeshManagerMeshShader& operator=(const MeshManagerMeshShader&) = delete;

	MeshManagerMeshShader(MeshManagerMeshShader&& other) noexcept
		: m_vertexBufferSharedData{ other.m_vertexBufferSharedData },
		m_vertexIndicesBufferSharedData{ other.m_vertexIndicesBufferSharedData },
		m_primIndicesBufferSharedData{ other.m_primIndicesBufferSharedData },
		m_meshletBufferSharedData{ other.m_meshletBufferSharedData },
		m_meshBoundsSharedData{ other.m_meshBoundsSharedData },
		m_meshDetails{ other.m_meshDetails },
		m_bundleDetails{ std::move(other.m_bundleDetails) }
	{}

	MeshManagerMeshShader& operator=(MeshManagerMeshShader&& other) noexcept
	{
		m_vertexBufferSharedData        = other.m_vertexBufferSharedData;
		m_vertexIndicesBufferSharedData = other.m_vertexIndicesBufferSharedData;
		m_primIndicesBufferSharedData   = other.m_primIndicesBufferSharedData;
		m_meshletBufferSharedData       = other.m_meshletBufferSharedData;
		m_meshBoundsSharedData          = other.m_meshBoundsSharedData;
		m_meshDetails                   = other.m_meshDetails;
		m_bundleDetails                 = std::move(other.m_bundleDetails);

		return *this;
	}
};
#endif
