#ifndef MESH_MANAGER_HPP_
#define MESH_MANAGER_HPP_
#include <D3DMeshBundleMS.hpp>
#include <D3DMeshBundleVS.hpp>
#include <ReusableVector.hpp>
#include <D3DSharedBuffer.hpp>
#include <StagingBufferManager.hpp>

template<
	class Derived,
	class D3DMeshBundle
>
class MeshManager
{
public:
	MeshManager() : m_meshBundles{}, m_oldBufferCopyNecessary{ false } {}

	[[nodiscard]]
	std::uint32_t AddMeshBundle(
		std::unique_ptr<MeshBundleTemporary> meshBundle, StagingBufferManager& stagingBufferMan,
		Callisto::TemporaryDataBufferGPU& tempBuffer
	) {
		D3DMeshBundle d3dMeshBundle{};

		static_cast<Derived*>(this)->ConfigureMeshBundle(
			std::move(meshBundle), stagingBufferMan, d3dMeshBundle, tempBuffer
		);

		const size_t meshIndex   = m_meshBundles.Add(std::move(d3dMeshBundle));

		m_oldBufferCopyNecessary = true;

		return static_cast<std::uint32_t>(meshIndex);
	}

	void RemoveMeshBundle(std::uint32_t bundleIndex) noexcept
	{
		static_cast<Derived*>(this)->ConfigureRemoveMesh(bundleIndex);

		// It is okay to use the non-clear function based RemoveElement, as I will be
		// moving the Buffers out as SharedBuffer.
		m_meshBundles.RemoveElement(bundleIndex);
	}

	[[nodiscard]]
	D3DMeshBundle& GetBundle(size_t index) noexcept { return m_meshBundles.at(index); }
	[[nodiscard]]
	const D3DMeshBundle& GetBundle(size_t index) const noexcept { return m_meshBundles.at(index); }

protected:
	Callisto::ReusableVector<D3DMeshBundle> m_meshBundles;
	bool                                    m_oldBufferCopyNecessary;

public:
	MeshManager(const MeshManager&) = delete;
	MeshManager& operator=(const MeshManager&) = delete;

	MeshManager(MeshManager&& other) noexcept
		: m_meshBundles{ std::move(other.m_meshBundles) },
		m_oldBufferCopyNecessary{ other.m_oldBufferCopyNecessary }
	{}
	MeshManager& operator=(MeshManager&& other) noexcept
	{
		m_meshBundles            = std::move(other.m_meshBundles);
		m_oldBufferCopyNecessary = other.m_oldBufferCopyNecessary;

		return *this;
	}
};

class MeshManagerVSIndividual : public MeshManager<MeshManagerVSIndividual, D3DMeshBundleVS>
{
	friend class MeshManager<MeshManagerVSIndividual, D3DMeshBundleVS>;
public:
	MeshManagerVSIndividual(ID3D12Device5* device, MemoryManager* memoryManager);

	void CopyOldBuffers(const D3DCommandList& copyList) noexcept;

private:
	void ConfigureMeshBundle(
		std::unique_ptr<MeshBundleTemporary> meshBundle, StagingBufferManager& stagingBufferMan,
		D3DMeshBundleVS& d3dMeshBundle, Callisto::TemporaryDataBufferGPU& tempBuffer
	);
	void ConfigureRemoveMesh(size_t bundleIndex) noexcept;

private:
	SharedBufferGPU m_vertexBuffer;
	SharedBufferGPU m_indexBuffer;

public:
	MeshManagerVSIndividual(const MeshManagerVSIndividual&) = delete;
	MeshManagerVSIndividual& operator=(const MeshManagerVSIndividual&) = delete;

	MeshManagerVSIndividual(MeshManagerVSIndividual&& other) noexcept
		: MeshManager{ std::move(other) },
		m_vertexBuffer{ std::move(other.m_vertexBuffer) },
		m_indexBuffer{ std::move(other.m_indexBuffer) }
	{}
	MeshManagerVSIndividual& operator=(MeshManagerVSIndividual&& other) noexcept
	{
		MeshManager::operator=(std::move(other));
		m_vertexBuffer       = std::move(other.m_vertexBuffer);
		m_indexBuffer        = std::move(other.m_indexBuffer);

		return *this;
	}
};

class MeshManagerVSIndirect : public MeshManager<MeshManagerVSIndirect, D3DMeshBundleVS>
{
	friend class MeshManager<MeshManagerVSIndirect, D3DMeshBundleVS>;
public:
	MeshManagerVSIndirect(ID3D12Device5* device, MemoryManager* memoryManager);

	void SetDescriptorLayoutCS(
		std::vector<D3DDescriptorManager>& descriptorManagers, size_t csRegisterSpace
	) const noexcept;
	void SetDescriptorsCS(
		std::vector<D3DDescriptorManager>& descriptorManagers, size_t csRegisterSpace
	) const;

	void CopyOldBuffers(const D3DCommandList& copyList) noexcept;

private:
	void ConfigureMeshBundle(
		std::unique_ptr<MeshBundleTemporary> meshBundle, StagingBufferManager& stagingBufferMan,
		D3DMeshBundleVS& d3dMeshBundle, Callisto::TemporaryDataBufferGPU& tempBuffer
	);
	void ConfigureRemoveMesh(size_t bundleIndex) noexcept;

private:
	SharedBufferGPU m_vertexBuffer;
	SharedBufferGPU m_indexBuffer;
	SharedBufferGPU m_perMeshDataBuffer;
	SharedBufferGPU m_perMeshBundleDataBuffer;

	// Compute Shader ones
	// SRV
	static constexpr size_t s_perMeshDataSRVRegisterSlot       = 4u;
	static constexpr size_t s_perMeshBundleDataSRVRegisterSlot = 5u;

public:
	MeshManagerVSIndirect(const MeshManagerVSIndirect&) = delete;
	MeshManagerVSIndirect& operator=(const MeshManagerVSIndirect&) = delete;

	MeshManagerVSIndirect(MeshManagerVSIndirect&& other) noexcept
		: MeshManager{ std::move(other) },
		m_vertexBuffer{ std::move(other.m_vertexBuffer) },
		m_indexBuffer{ std::move(other.m_indexBuffer) },
		m_perMeshDataBuffer{ std::move(other.m_perMeshDataBuffer) },
		m_perMeshBundleDataBuffer{ std::move(other.m_perMeshBundleDataBuffer) }
	{}
	MeshManagerVSIndirect& operator=(MeshManagerVSIndirect&& other) noexcept
	{
		MeshManager::operator=(std::move(other));
		m_vertexBuffer            = std::move(other.m_vertexBuffer);
		m_indexBuffer             = std::move(other.m_indexBuffer);
		m_perMeshDataBuffer       = std::move(other.m_perMeshDataBuffer);
		m_perMeshBundleDataBuffer = std::move(other.m_perMeshBundleDataBuffer);

		return *this;
	}
};

class MeshManagerMS : public MeshManager<MeshManagerMS, D3DMeshBundleMS>
{
	friend class MeshManager<MeshManagerMS, D3DMeshBundleMS>;
public:
	MeshManagerMS(ID3D12Device5* device, MemoryManager* memoryManager);

	void SetDescriptorLayout(
		std::vector<D3DDescriptorManager>& descriptorManagers, size_t msRegisterSpace
	) const noexcept;

	// Should be called after a new Mesh has been added.
	void SetDescriptors(
		std::vector<D3DDescriptorManager>& descriptorManagers, size_t msRegisterSpace
	) const;

	void CopyOldBuffers(const D3DCommandList& copyList) noexcept;

private:
	void ConfigureMeshBundle(
		std::unique_ptr<MeshBundleTemporary> meshBundle, StagingBufferManager& stagingBufferMan,
		D3DMeshBundleMS& d3dMeshBundle, Callisto::TemporaryDataBufferGPU& tempBuffer
	);
	void ConfigureRemoveMesh(size_t bundleIndex) noexcept;

private:
	SharedBufferGPU m_perMeshletBuffer;
	SharedBufferGPU m_vertexBuffer;
	SharedBufferGPU m_vertexIndicesBuffer;
	SharedBufferGPU m_primIndicesBuffer;

	// SRV
	static constexpr size_t s_perMeshletBufferSRVRegisterSlot    = 1u;
	static constexpr size_t s_vertexBufferSRVRegisterSlot        = 2u;
	static constexpr size_t s_vertexIndicesBufferSRVRegisterSlot = 3u;
	static constexpr size_t s_primIndicesBufferSRVRegisterSlot   = 4u;

public:
	MeshManagerMS(const MeshManagerMS&) = delete;
	MeshManagerMS& operator=(const MeshManagerMS&) = delete;

	MeshManagerMS(MeshManagerMS&& other) noexcept
		: MeshManager{ std::move(other) },
		m_perMeshletBuffer{ std::move(other.m_perMeshletBuffer) },
		m_vertexBuffer{ std::move(other.m_vertexBuffer) },
		m_vertexIndicesBuffer{ std::move(other.m_vertexIndicesBuffer) },
		m_primIndicesBuffer{ std::move(other.m_primIndicesBuffer) }
	{}
	MeshManagerMS& operator=(MeshManagerMS&& other) noexcept
	{
		MeshManager::operator=(std::move(other));
		m_perMeshletBuffer    = std::move(other.m_perMeshletBuffer);
		m_vertexBuffer        = std::move(other.m_vertexBuffer);
		m_vertexIndicesBuffer = std::move(other.m_vertexIndicesBuffer);
		m_primIndicesBuffer   = std::move(other.m_primIndicesBuffer);

		return *this;
	}
};
#endif
