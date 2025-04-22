#include <D3DMeshManager.hpp>

namespace Gaia
{
// Mesh Manager VS Individual
MeshManagerVSIndividual::MeshManagerVSIndividual(
	ID3D12Device5* device, MemoryManager* memoryManager
) : MeshManager{},
	m_vertexBuffer{ device, memoryManager },
	m_indexBuffer{ device, memoryManager }
{}

void MeshManagerVSIndividual::CopyOldBuffers(const D3DCommandList& copyList) noexcept
{
	if (m_oldBufferCopyNecessary)
	{
		m_indexBuffer.CopyOldBuffer(copyList);
		m_vertexBuffer.CopyOldBuffer(copyList);

		m_oldBufferCopyNecessary = false;
	}
}

void MeshManagerVSIndividual::ConfigureMeshBundle(
	std::unique_ptr<MeshBundleTemporary> meshBundle, StagingBufferManager& stagingBufferMan,
	D3DMeshBundleVS& d3dMeshBundle, Callisto::TemporaryDataBufferGPU& tempBuffer
) {
	d3dMeshBundle.SetMeshBundle(
		std::move(meshBundle), stagingBufferMan, m_vertexBuffer, m_indexBuffer, tempBuffer
	);
}

void MeshManagerVSIndividual::ConfigureRemoveMesh(size_t bundleIndex) noexcept
{
	D3DMeshBundleVS& meshBundle = m_meshBundles.at(bundleIndex);

	{
		const SharedBufferData& vertexSharedData = meshBundle.GetVertexSharedData();
		m_vertexBuffer.RelinquishMemory(vertexSharedData);

		const SharedBufferData& indexSharedData  = meshBundle.GetIndexSharedData();
		m_indexBuffer.RelinquishMemory(indexSharedData);
	}
}

// Mesh Manager VS Indirect
MeshManagerVSIndirect::MeshManagerVSIndirect(ID3D12Device5* device, MemoryManager* memoryManager)
	: MeshManager{},
	m_vertexBuffer{ device, memoryManager },
	m_indexBuffer{ device, memoryManager },
	m_perMeshDataBuffer{ device, memoryManager },
	m_perMeshBundleDataBuffer{ device, memoryManager }
{}

void MeshManagerVSIndirect::ConfigureMeshBundle(
	std::unique_ptr<MeshBundleTemporary> meshBundle, StagingBufferManager& stagingBufferMan,
	D3DMeshBundleVS& d3dMeshBundle, Callisto::TemporaryDataBufferGPU& tempBuffer
) {
	d3dMeshBundle.SetMeshBundle(
		std::move(meshBundle), stagingBufferMan, m_vertexBuffer, m_indexBuffer, m_perMeshDataBuffer,
		m_perMeshBundleDataBuffer, tempBuffer
	);
}

void MeshManagerVSIndirect::ConfigureRemoveMesh(size_t bundleIndex) noexcept
{
	D3DMeshBundleVS& meshBundle = m_meshBundles.at(bundleIndex);

	{
		const SharedBufferData& vertexSharedData        = meshBundle.GetVertexSharedData();
		m_vertexBuffer.RelinquishMemory(vertexSharedData);

		const SharedBufferData& indexSharedData         = meshBundle.GetIndexSharedData();
		m_indexBuffer.RelinquishMemory(indexSharedData);

		const SharedBufferData& perMeshSharedData       = meshBundle.GetPerMeshSharedData();
		m_perMeshDataBuffer.RelinquishMemory(perMeshSharedData);

		const SharedBufferData& perMeshBundleSharedData = meshBundle.GetPerMeshBundleSharedData();
		m_perMeshBundleDataBuffer.RelinquishMemory(perMeshBundleSharedData);
	}
}

void MeshManagerVSIndirect::SetDescriptorLayoutCS(
	std::vector<D3DDescriptorManager>& descriptorManagers, size_t csRegisterSpace
) const noexcept {
	for (D3DDescriptorManager& descriptorManager : descriptorManagers)
	{
		descriptorManager.AddRootSRV(
			s_perMeshDataSRVRegisterSlot, csRegisterSpace, D3D12_SHADER_VISIBILITY_ALL
		);
		descriptorManager.AddRootSRV(
			s_perMeshBundleDataSRVRegisterSlot, csRegisterSpace, D3D12_SHADER_VISIBILITY_ALL
		);
	}
}

void MeshManagerVSIndirect::SetDescriptorsCS(
	std::vector<D3DDescriptorManager>& descriptorManagers, size_t csRegisterSpace
) const {
	for (D3DDescriptorManager& descriptorManager : descriptorManagers)
	{
		descriptorManager.SetRootSRV(
			s_perMeshDataSRVRegisterSlot, csRegisterSpace, m_perMeshDataBuffer.GetGPUAddress(), false
		);
		descriptorManager.SetRootSRV(
			s_perMeshBundleDataSRVRegisterSlot, csRegisterSpace,
			m_perMeshBundleDataBuffer.GetGPUAddress(), false
		);
	}
}

void MeshManagerVSIndirect::CopyOldBuffers(const D3DCommandList& copyList) noexcept
{
	if (m_oldBufferCopyNecessary)
	{
		m_vertexBuffer.CopyOldBuffer(copyList);
		m_indexBuffer.CopyOldBuffer(copyList);
		m_perMeshDataBuffer.CopyOldBuffer(copyList);
		m_perMeshBundleDataBuffer.CopyOldBuffer(copyList);

		m_oldBufferCopyNecessary = false;
	}
}

// Mesh Manager MS
MeshManagerMS::MeshManagerMS(ID3D12Device5* device, MemoryManager* memoryManager)
	: MeshManager{},
	m_perMeshletBuffer{ device, memoryManager },
	m_vertexBuffer{ device, memoryManager },
	m_vertexIndicesBuffer{ device, memoryManager },
	m_primIndicesBuffer{ device, memoryManager }
{}

void MeshManagerMS::ConfigureMeshBundle(
	std::unique_ptr<MeshBundleTemporary> meshBundle, StagingBufferManager& stagingBufferMan,
	D3DMeshBundleMS& d3dMeshBundle, Callisto::TemporaryDataBufferGPU& tempBuffer
) {
	d3dMeshBundle.SetMeshBundle(
		std::move(meshBundle), stagingBufferMan, m_vertexBuffer, m_vertexIndicesBuffer,
		m_primIndicesBuffer, m_perMeshletBuffer, tempBuffer
	);
}

void MeshManagerMS::ConfigureRemoveMesh(size_t bundleIndex) noexcept
{
	D3DMeshBundleMS& meshBundle = m_meshBundles.at(bundleIndex);

	{
		const SharedBufferData& vertexSharedData        = meshBundle.GetVertexSharedData();
		m_vertexBuffer.RelinquishMemory(vertexSharedData);

		const SharedBufferData& vertexIndicesSharedData = meshBundle.GetVertexIndicesSharedData();
		m_vertexIndicesBuffer.RelinquishMemory(vertexIndicesSharedData);

		const SharedBufferData& primIndicesSharedData   = meshBundle.GetPrimIndicesSharedData();
		m_primIndicesBuffer.RelinquishMemory(primIndicesSharedData);

		const SharedBufferData& perMeshletSharedData    = meshBundle.GetPerMeshletSharedData();
		m_perMeshletBuffer.RelinquishMemory(perMeshletSharedData);
	}
}

void MeshManagerMS::SetDescriptorLayout(
	std::vector<D3DDescriptorManager>& descriptorManagers, size_t msRegisterSpace
) const noexcept {
	for (D3DDescriptorManager& descriptorManager : descriptorManagers)
	{
		descriptorManager.AddRootSRV(
			s_perMeshletBufferSRVRegisterSlot, msRegisterSpace, D3D12_SHADER_VISIBILITY_ALL
		); // Both the AS and MS will use it.
		descriptorManager.AddRootSRV(
			s_vertexBufferSRVRegisterSlot, msRegisterSpace, D3D12_SHADER_VISIBILITY_MESH
		);
		descriptorManager.AddRootSRV(
			s_vertexIndicesBufferSRVRegisterSlot, msRegisterSpace, D3D12_SHADER_VISIBILITY_MESH
		);
		descriptorManager.AddRootSRV(
			s_primIndicesBufferSRVRegisterSlot, msRegisterSpace, D3D12_SHADER_VISIBILITY_MESH
		);
	}
}

void MeshManagerMS::SetDescriptors(
	std::vector<D3DDescriptorManager>& descriptorManagers, size_t msRegisterSpace
) const {
	for (D3DDescriptorManager& descriptorManager : descriptorManagers)
	{
		descriptorManager.SetRootSRV(
			s_vertexBufferSRVRegisterSlot, msRegisterSpace, m_vertexBuffer.GetGPUAddress(), true
		);
		descriptorManager.SetRootSRV(
			s_vertexIndicesBufferSRVRegisterSlot, msRegisterSpace, m_vertexIndicesBuffer.GetGPUAddress(),
			true
		);
		descriptorManager.SetRootSRV(
			s_primIndicesBufferSRVRegisterSlot, msRegisterSpace, m_primIndicesBuffer.GetGPUAddress(),
			true
		);
		descriptorManager.SetRootSRV(
			s_perMeshletBufferSRVRegisterSlot, msRegisterSpace, m_perMeshletBuffer.GetGPUAddress(), true
		);
	}
}

void MeshManagerMS::CopyOldBuffers(const D3DCommandList& copyList) noexcept
{
	if (m_oldBufferCopyNecessary)
	{
		m_perMeshletBuffer.CopyOldBuffer(copyList);
		m_vertexBuffer.CopyOldBuffer(copyList);
		m_vertexIndicesBuffer.CopyOldBuffer(copyList);
		m_primIndicesBuffer.CopyOldBuffer(copyList);

		m_oldBufferCopyNecessary = false;
	}
}
}
