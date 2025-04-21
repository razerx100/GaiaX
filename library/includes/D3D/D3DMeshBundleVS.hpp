#ifndef D3D_MESH_BUNDLE_VS_HPP_
#define D3D_MESH_BUNDLE_VS_HPP_
#include <D3DHeaders.hpp>
#include <memory>
#include <D3DResources.hpp>
#include <StagingBufferManager.hpp>
#include <D3DCommandQueue.hpp>
#include <D3DSharedBuffer.hpp>

#include <MeshBundle.hpp>

class D3DMeshBundleVS
{
	struct PerMeshBundleData
	{
		std::uint32_t meshOffset;
	};

public:
	D3DMeshBundleVS();

	void SetMeshBundle(
		std::unique_ptr<MeshBundleTemporary> meshBundle, StagingBufferManager& stagingBufferMan,
		SharedBufferGPU& vertexSharedBuffer, SharedBufferGPU& indexSharedBuffer,
		Callisto::TemporaryDataBufferGPU& tempBuffer
	);
	void SetMeshBundle(
		std::unique_ptr<MeshBundleTemporary> meshBundle, StagingBufferManager& stagingBufferMan,
		SharedBufferGPU& vertexSharedBuffer, SharedBufferGPU& indexSharedBuffer,
		SharedBufferGPU& perMeshSharedBuffer, SharedBufferGPU& perMeshBundleSharedBuffer,
		Callisto::TemporaryDataBufferGPU& tempBuffer
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
	const MeshTemporaryDetailsVS& GetMeshDetails(size_t index) const noexcept
	{
		return m_bundleDetails.meshTemporaryDetailsVS[index];
	}

private:
	void _setMeshBundle(
		std::unique_ptr<MeshBundleTemporary> meshBundle, StagingBufferManager& stagingBufferMan,
		SharedBufferGPU& vertexSharedBuffer, SharedBufferGPU& indexSharedBuffer,
		Callisto::TemporaryDataBufferGPU& tempBuffer
	);

private:
	SharedBufferData           m_vertexBufferSharedData;
	SharedBufferData           m_indexBufferSharedData;
	SharedBufferData           m_perMeshSharedData;
	SharedBufferData           m_perMeshBundleSharedData;
	MeshBundleTemporaryDetails m_bundleDetails;

public:
	D3DMeshBundleVS(const D3DMeshBundleVS&) = delete;
	D3DMeshBundleVS& operator=(const D3DMeshBundleVS&) = delete;

	D3DMeshBundleVS(D3DMeshBundleVS&& other) noexcept
		: m_vertexBufferSharedData{ other.m_vertexBufferSharedData },
		m_indexBufferSharedData{ other.m_indexBufferSharedData },
		m_perMeshSharedData{ other.m_perMeshSharedData },
		m_perMeshBundleSharedData{ other.m_perMeshBundleSharedData },
		m_bundleDetails{ std::move(other.m_bundleDetails) }
	{}
	D3DMeshBundleVS& operator=(D3DMeshBundleVS&& other) noexcept
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
