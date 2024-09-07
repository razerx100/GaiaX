#ifndef D3D_COMMAND_QUEUE_HPP_
#define D3D_COMMAND_QUEUE_HPP_
#include <D3DHeaders.hpp>
#include <utility>

class D3DCommandList
{
public:
	D3DCommandList() : m_commandList{}, m_commandAllocator{} {}
	D3DCommandList(ID3D12Device4* device, D3D12_COMMAND_LIST_TYPE type);

	void Create(ID3D12Device4* device, D3D12_COMMAND_LIST_TYPE type);

	void Reset() const;
	void Close() const;

	[[nodiscard]]
	ID3D12GraphicsCommandList6* Get() const noexcept { return m_commandList.Get(); }

private:
	ComPtr<ID3D12GraphicsCommandList6> m_commandList;
	ComPtr<ID3D12CommandAllocator>     m_commandAllocator;

public:
	D3DCommandList(const D3DCommandList&) = delete;
	D3DCommandList& operator=(const D3DCommandList&) = delete;

	D3DCommandList(D3DCommandList&& other) noexcept
		: m_commandList{ std::move(other.m_commandList) },
		m_commandAllocator{ std::move(other.m_commandAllocator) }
	{}
	D3DCommandList& operator=(D3DCommandList&& other) noexcept
	{
		m_commandList      = std::move(other.m_commandList);
		m_commandAllocator = std::move(other.m_commandAllocator);

		return *this;
	}
};

// The command list will be reset at creation of an object and closed at destruction.
struct CommandListScope
{
	CommandListScope(const D3DCommandList& commandList) : m_commandList{ commandList }
	{
		m_commandList.Reset();
	}

	~CommandListScope() noexcept { m_commandList.Close(); }

	const D3DCommandList& m_commandList;

	operator const D3DCommandList& () const noexcept { return m_commandList; }
};

class D3DCommandQueue
{
public:
	D3DCommandQueue(ID3D12Device* device, D3D12_COMMAND_LIST_TYPE type);

	void SignalCommandQueue(ID3D12Fence* fence, UINT64 fenceValue) const;
	void WaitOnGPU(ID3D12Fence* fence, UINT64 fenceValue) const;
	void ExecuteCommandLists(ID3D12GraphicsCommandList* commandList) const noexcept;

	[[nodiscard]]
	ID3D12CommandQueue* GetQueue() const noexcept;

private:
	ComPtr<ID3D12CommandQueue> m_pCommandQueue;
};
#endif
