#ifndef D3D_COMMAND_QUEUE_HPP_
#define D3D_COMMAND_QUEUE_HPP_
#include <D3DHeaders.hpp>
#include <utility>
#include <vector>

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
	D3DCommandQueue() : m_commandQueue{}, m_commandLists{} {}

	void Create(
		ID3D12Device4* device, D3D12_COMMAND_LIST_TYPE type, size_t bufferCount
	);

	void Signal(ID3D12Fence* fence, UINT64 signalValue) const;
	void Wait(ID3D12Fence* fence, UINT64 waitValue) const;

	void ExecuteCommandLists(ID3D12GraphicsCommandList* commandList) const noexcept;

	[[nodiscard]]
	ID3D12CommandQueue* GetQueue() const noexcept { return m_commandQueue.Get(); }
	[[nodiscard]]
	D3DCommandList& GetCommandList(size_t index) noexcept { return m_commandLists[index]; }
	[[nodiscard]]
	const D3DCommandList& GetCommandList(size_t index) const noexcept { return m_commandLists[index]; }

private:
	ComPtr<ID3D12CommandQueue>  m_commandQueue;
	std::vector<D3DCommandList> m_commandLists;

public:
	D3DCommandQueue(const D3DCommandQueue&) = delete;
	D3DCommandQueue& operator=(const D3DCommandQueue&) = delete;

	D3DCommandQueue(D3DCommandQueue&& other) noexcept
		: m_commandQueue{ std::move(other.m_commandQueue) },
		m_commandLists{ std::move(other.m_commandLists) }
	{}
	D3DCommandQueue& operator=(D3DCommandQueue&& other) noexcept
	{
		m_commandQueue = std::move(other.m_commandQueue);
		m_commandLists = std::move(other.m_commandLists);

		return *this;
	}
};
#endif
