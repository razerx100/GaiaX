#ifndef D3D_EXTERNAL_RESOURCE_FACTORY_HPP_
#define D3D_EXTERNAL_RESOURCE_FACTORY_HPP_
#include <utility>
#include <vector>
#include <D3DAllocator.hpp>
#include <D3DExternalBuffer.hpp>
#include <ReusableVector.hpp>

namespace Gaia
{
class D3DExternalResourceFactory
{
	using ExternalBuffer_t  = std::shared_ptr<D3DExternalBuffer>;
	using ExternalTexture_t = std::shared_ptr<D3DExternalTexture>;
public:
	D3DExternalResourceFactory(ID3D12Device* device, MemoryManager* memoryManager)
		: m_device{ device }, m_memoryManager{ memoryManager }, m_externalBuffers{},
		m_externalTextures{}
	{}

	[[nodiscard]]
	size_t CreateExternalBuffer(ExternalBufferType type);
	[[nodiscard]]
	size_t CreateExternalTexture();

	[[nodiscard]]
	D3DExternalBuffer* GetExternalBufferRP(size_t index) const noexcept
	{
		return m_externalBuffers[index].get();
	}
	[[nodiscard]]
	D3DExternalTexture* GetExternalTextureRP(size_t index) const noexcept
	{
		return m_externalTextures[index].get();
	}

	[[nodiscard]]
	std::shared_ptr<D3DExternalBuffer> GetExternalBufferSP(size_t index) const noexcept
	{
		return m_externalBuffers[index];
	}
	[[nodiscard]]
	std::shared_ptr<D3DExternalTexture> GetExternalTextureSP(size_t index) const noexcept
	{
		return m_externalTextures[index];
	}

	[[nodiscard]]
	const Buffer& GetD3DBuffer(size_t index) const noexcept
	{
		return m_externalBuffers[index]->GetBuffer();
	}

	[[nodiscard]]
	const Texture& GetD3DTexture(size_t index) const noexcept
	{
		return m_externalTextures[index]->GetTexture();
	}

	void RemoveExternalBuffer(size_t index) noexcept
	{
		m_externalBuffers[index].reset();
		m_externalBuffers.RemoveElement(index);
	}

	void RemoveExternalTexture(size_t index) noexcept
	{
		m_externalTextures[index].reset();
		m_externalTextures.RemoveElement(index);
	}

private:
	ID3D12Device*                               m_device;
	MemoryManager*                              m_memoryManager;
	Callisto::ReusableVector<ExternalBuffer_t>  m_externalBuffers;
	Callisto::ReusableVector<ExternalTexture_t> m_externalTextures;

public:
	D3DExternalResourceFactory(const D3DExternalResourceFactory&) = delete;
	D3DExternalResourceFactory& operator=(const D3DExternalResourceFactory&) = delete;

	D3DExternalResourceFactory(D3DExternalResourceFactory&& other) noexcept
		: m_device{ other.m_device },
		m_memoryManager{ std::exchange(other.m_memoryManager, nullptr) },
		m_externalBuffers{ std::move(other.m_externalBuffers) },
		m_externalTextures{ std::move(other.m_externalTextures) }
	{}
	D3DExternalResourceFactory& operator=(D3DExternalResourceFactory&& other) noexcept
	{
		m_device           = other.m_device;
		m_memoryManager    = std::exchange(other.m_memoryManager, nullptr);
		m_externalBuffers  = std::move(other.m_externalBuffers);
		m_externalTextures = std::move(other.m_externalTextures);

		return *this;
	}
};
}
#endif
