#ifndef D3D_MESH_BUNDLE_MS_HPP_
#define D3D_MESH_BUNDLE_MS_HPP_
#include <D3DHeaders.hpp>
#include <memory>
#include <D3DResources.hpp>
#include <StagingBufferManager.hpp>
#include <D3DSharedBuffer.hpp>

#include <MeshBundle.hpp>

class D3DMeshBundleMS
{
public:
	// The offset should be enough to identify the mesh. We wouldn't really need the size
	// as the meshlets should already have that data.
	struct MeshBundleDetailsMS
	{
		std::uint32_t vertexOffset;
		std::uint32_t vertexIndicesOffset;
		std::uint32_t primIndicesOffset;
		std::uint32_t meshletOffset;
	};

	struct PerMeshBundleData
	{
		std::uint32_t meshOffset;
	};

public:
	D3DMeshBundleMS();

	void SetMeshBundle(
		std::unique_ptr<MeshBundleTemporary> meshBundle, StagingBufferManager& stagingBufferMan,
		SharedBufferGPU& vertexSharedBuffer, SharedBufferGPU& vertexIndicesSharedBuffer,
		SharedBufferGPU& primIndicesSharedBuffer, SharedBufferGPU& perMeshletSharedBuffer,
		TemporaryDataBufferGPU& tempBuffer
	);
	void SetMeshBundle(
		std::unique_ptr<MeshBundleTemporary> meshBundle, StagingBufferManager& stagingBufferMan,
		SharedBufferGPU& vertexSharedBuffer, SharedBufferGPU& vertexIndicesSharedBuffer,
		SharedBufferGPU& primIndicesSharedBuffer, SharedBufferGPU& perMeshletSharedBuffer,
		SharedBufferGPU& perMeshSharedBuffer, SharedBufferGPU& perMeshBundleSharedBuffer,
		TemporaryDataBufferGPU& tempBuffer
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
	const SharedBufferData& GetPerMeshletSharedData() const noexcept
	{
		return m_perMeshletBufferSharedData;
	}
	[[nodiscard]]
	const SharedBufferData& GetPerMeshSharedData() const noexcept { return m_perMeshSharedData; }
	[[nodiscard]]
	const SharedBufferData& GetPerMeshBundleSharedData() const noexcept
	{
		return m_perMeshBundleSharedData;
	}

	[[nodiscard]]
	MeshBundleDetailsMS GetMeshBundleDetailsMS() const noexcept { return m_meshBundleDetails; }
	[[nodiscard]]
	const MeshTemporaryDetailsMS& GetMeshDetails(size_t index) const noexcept
	{
		return m_bundleDetails.meshTemporaryDetailsMS[index];
	}

	[[nodiscard]]
	static consteval UINT GetConstantCount() noexcept
	{
		return static_cast<UINT>(sizeof(MeshBundleDetailsMS) / sizeof(UINT));
	}

private:
	void _setMeshBundle(
		std::unique_ptr<MeshBundleTemporary> meshBundle, StagingBufferManager& stagingBufferMan,
		SharedBufferGPU& vertexSharedBuffer, SharedBufferGPU& vertexIndicesSharedBuffer,
		SharedBufferGPU& primIndicesSharedBuffer, SharedBufferGPU& perMeshletSharedBuffer,
		TemporaryDataBufferGPU& tempBuffer
	);

private:
	SharedBufferData           m_vertexBufferSharedData;
	SharedBufferData           m_vertexIndicesBufferSharedData;
	SharedBufferData           m_primIndicesBufferSharedData;
	SharedBufferData           m_perMeshletBufferSharedData;
	SharedBufferData           m_perMeshSharedData;
	SharedBufferData           m_perMeshBundleSharedData;
	MeshBundleDetailsMS        m_meshBundleDetails;
	MeshBundleTemporaryDetails m_bundleDetails;

public:
	D3DMeshBundleMS(const D3DMeshBundleMS&) = delete;
	D3DMeshBundleMS& operator=(const D3DMeshBundleMS&) = delete;

	D3DMeshBundleMS(D3DMeshBundleMS&& other) noexcept
		: m_vertexBufferSharedData{ other.m_vertexBufferSharedData },
		m_vertexIndicesBufferSharedData{ other.m_vertexIndicesBufferSharedData },
		m_primIndicesBufferSharedData{ other.m_primIndicesBufferSharedData },
		m_perMeshletBufferSharedData{ other.m_perMeshletBufferSharedData },
		m_perMeshSharedData{ other.m_perMeshSharedData },
		m_perMeshBundleSharedData{ other.m_perMeshBundleSharedData },
		m_meshBundleDetails{ other.m_meshBundleDetails },
		m_bundleDetails{ std::move(other.m_bundleDetails) }
	{}

	D3DMeshBundleMS& operator=(D3DMeshBundleMS&& other) noexcept
	{
		m_vertexBufferSharedData        = other.m_vertexBufferSharedData;
		m_vertexIndicesBufferSharedData = other.m_vertexIndicesBufferSharedData;
		m_primIndicesBufferSharedData   = other.m_primIndicesBufferSharedData;
		m_perMeshletBufferSharedData    = other.m_perMeshletBufferSharedData;
		m_perMeshSharedData             = other.m_perMeshSharedData;
		m_perMeshBundleSharedData       = other.m_perMeshBundleSharedData;
		m_meshBundleDetails             = other.m_meshBundleDetails;
		m_bundleDetails                 = std::move(other.m_bundleDetails);

		return *this;
	}
};
#endif
