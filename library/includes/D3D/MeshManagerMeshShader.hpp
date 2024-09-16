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
	struct MeshDetails
	{
		std::uint32_t vertexOffset;
		std::uint32_t vertexIndicesOffset;
		std::uint32_t primIndicesOffset;
		std::uint32_t meshletOffset;
	};

	struct BoundsDetails
	{
		std::uint32_t offset;
		std::uint32_t count;
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
	BoundsDetails GetBoundsDetails() const noexcept;

	[[nodiscard]]
	MeshDetails GetMeshDetails() const noexcept { return m_meshDetails; }

	[[nodiscard]]
	static consteval std::uint32_t GetConstantBufferSize() noexcept
	{
		return static_cast<std::uint32_t>(sizeof(MeshDetails));
	}

private:
	SharedBufferData m_vertexBufferSharedData;
	SharedBufferData m_vertexIndicesBufferSharedData;
	SharedBufferData m_primIndicesBufferSharedData;
	SharedBufferData m_meshletBufferSharedData;
	SharedBufferData m_meshBoundsSharedData;
	MeshDetails      m_meshDetails;

public:
	MeshManagerMeshShader(const MeshManagerMeshShader&) = delete;
	MeshManagerMeshShader& operator=(const MeshManagerMeshShader&) = delete;

	MeshManagerMeshShader(MeshManagerMeshShader&& other) noexcept
		: m_vertexBufferSharedData{ other.m_vertexBufferSharedData },
		m_vertexIndicesBufferSharedData{ other.m_vertexIndicesBufferSharedData },
		m_primIndicesBufferSharedData{ other.m_primIndicesBufferSharedData },
		m_meshletBufferSharedData{ other.m_meshletBufferSharedData },
		m_meshBoundsSharedData{ other.m_meshBoundsSharedData },
		m_meshDetails{ other.m_meshDetails }
	{}

	MeshManagerMeshShader& operator=(MeshManagerMeshShader&& other) noexcept
	{
		m_vertexBufferSharedData        = other.m_vertexBufferSharedData;
		m_vertexIndicesBufferSharedData = other.m_vertexIndicesBufferSharedData;
		m_primIndicesBufferSharedData   = other.m_primIndicesBufferSharedData;
		m_meshletBufferSharedData       = other.m_meshletBufferSharedData;
		m_meshBoundsSharedData          = other.m_meshBoundsSharedData;
		m_meshDetails                   = other.m_meshDetails;

		return *this;
	}
};
#endif
