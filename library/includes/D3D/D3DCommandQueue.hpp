#ifndef D3D_COMMAND_QUEUE_HPP_
#define D3D_COMMAND_QUEUE_HPP_
#include <D3DHeaders.hpp>
#include <D3DFence.hpp>
#include <D3DResourceBarrier.hpp>
#include <D3DResources.hpp>
#include <utility>
#include <vector>
#include <array>
#include <cassert>

class D3DCommandList
{
public:
	D3DCommandList() : m_commandList{}, m_commandAllocator{} {}
	D3DCommandList(ID3D12Device4* device, D3D12_COMMAND_LIST_TYPE type);

	void Create(ID3D12Device4* device, D3D12_COMMAND_LIST_TYPE type);

	void Reset() const;
	void Close() const;

	void Copy(
		const Buffer& src, UINT64 srcOffset, const Buffer& dst, UINT64 dstOffset, UINT64 size
	) const noexcept;
	void CopyWhole(const Buffer& src, const Buffer& dst) const noexcept
	{
		Copy(src, 0u, dst, 0u, src.BufferSize());
	}
	void Copy(
		const Buffer& src, UINT64 srcOffset, const Texture& dst, UINT subresourceIndex
	) const noexcept;
	void CopyWhole(
		const Buffer& src, const Texture& dst, UINT subresourceIndex
	) const noexcept {
		Copy(src, 0u, dst, subresourceIndex);
	}

	template<std::uint32_t BarrierCount = 1u>
	const D3DCommandList& AddBarrier(const D3DResourceBarrier<BarrierCount>& barrier) const noexcept
	{
		barrier.RecordBarriers(m_commandList.Get());

		return *this;
	}

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

template<UINT WaitCount = 0u, UINT SignalCount = 0u, UINT CommandListCount = 1u>
class QueueSubmitBuilder
{
public:
	QueueSubmitBuilder()
		: m_commandLists{}, m_waitFences{}, m_waitValues{}, m_signalFences{}, m_signalValues{},
		m_currentWaitIndex{ 0u }, m_currentSignalIndex{ 0u }, m_currentCmdListIndex{ 0u }
	{
		m_waitValues.fill(0u);
		m_signalValues.fill(0u);
	}

	QueueSubmitBuilder& WaitFence(ID3D12Fence* fence, UINT64 waitValue = 1u) noexcept
	{
		assert(m_currentWaitIndex < WaitCount && "More Wait fences than the allowed amount.");

		m_waitFences[m_currentWaitIndex] = fence;
		m_waitValues[m_currentWaitIndex] = waitValue;

		++m_currentWaitIndex;

		return *this;
	}

	QueueSubmitBuilder& WaitFence(const D3DFence& fence, UINT64 waitValue = 1u) noexcept
	{
		return WaitFence(fence.Get(), waitValue);
	}

	QueueSubmitBuilder& SignalFence(ID3D12Fence* fence, UINT64 signalValue = 1u) noexcept
	{
		assert(m_currentSignalIndex < SignalCount && "More Signal fences than the allowed amount.");

		m_signalFences[m_currentSignalIndex] = fence;
		m_signalValues[m_currentSignalIndex] = signalValue;

		++m_currentSignalIndex;

		return *this;
	}

	QueueSubmitBuilder& SignalFence(const D3DFence& fence, UINT64 signalValue = 1u) noexcept
	{
		return SignalFence(fence.Get(), signalValue);
	}

	QueueSubmitBuilder& CommandList(const D3DCommandList& commandList) noexcept
	{
		assert(m_currentCmdListIndex < CommandListCount && "More command lists than the allowed amount.");

		m_commandLists[m_currentCmdListIndex] = commandList.Get();

		++m_currentCmdListIndex;

		return *this;
	}

	[[nodiscard]]
	const auto& GetCommandLists() const noexcept { return m_commandLists; }
	[[nodiscard]]
	const auto& GetWaitFences() const noexcept { return m_waitFences; }
	[[nodiscard]]
	const auto& GetWaitValues() const noexcept { return m_waitValues; }
	[[nodiscard]]
	const auto& GetSignalFences() const noexcept { return m_signalFences; }
	[[nodiscard]]
	const auto& GetSignalValues() const noexcept { return m_signalValues; }

private:
	std::array<ID3D12CommandList*, CommandListCount> m_commandLists;
	std::array<ID3D12Fence*, WaitCount>              m_waitFences;
	std::array<UINT64, WaitCount>                    m_waitValues;
	std::array<ID3D12Fence*, SignalCount>            m_signalFences;
	std::array<UINT64, SignalCount>                  m_signalValues;
	size_t                                           m_currentWaitIndex;
	size_t                                           m_currentSignalIndex;
	size_t                                           m_currentCmdListIndex;
};

class D3DCommandQueue
{
public:
	D3DCommandQueue() : m_commandQueue{}, m_commandLists{} {}

	void Create(
		ID3D12Device4* device, D3D12_COMMAND_LIST_TYPE type, size_t bufferCount
	);

	void Signal(ID3D12Fence* fence, UINT64 signalValue) const noexcept;
	void Wait(ID3D12Fence* fence, UINT64 waitValue) const noexcept;

	void ExecuteCommandLists(ID3D12GraphicsCommandList* commandList) const noexcept;

	template<UINT WaitCount, UINT SignalCount, UINT CommandListCount = 1u>
	void SubmitCommandLists(
		const QueueSubmitBuilder<WaitCount, SignalCount, CommandListCount>& builder
	) const noexcept {
		{
			// GPU Wait.
			const std::array<ID3D12Fence*, WaitCount>& waitFences = builder.GetWaitFences();
			const std::array<UINT64, WaitCount>& waitValues       = builder.GetWaitValues();

			for (size_t index = 0u; index < WaitCount; ++index)
				Wait(waitFences[index], waitValues[index]);
		}

		{
			const std::array<ID3D12CommandList*, CommandListCount>& commandLists = builder.GetCommandLists();

			ExecuteCommandLists(std::data(commandLists), CommandListCount);
		}

		{
			// GPU Signal.
			const std::array<ID3D12Fence*, SignalCount>& signalFences = builder.GetSignalFences();
			const std::array<UINT64, SignalCount>& signalValues       = builder.GetSignalValues();

			for (size_t index = 0u; index < SignalCount; ++index)
				Signal(signalFences[index], signalValues[index]);
		}
	}

	[[nodiscard]]
	ID3D12CommandQueue* GetQueue() const noexcept { return m_commandQueue.Get(); }
	[[nodiscard]]
	D3DCommandList& GetCommandList(size_t index) noexcept { return m_commandLists[index]; }
	[[nodiscard]]
	const D3DCommandList& GetCommandList(size_t index) const noexcept { return m_commandLists[index]; }

private:
	void ExecuteCommandLists(
		ID3D12CommandList* const * commandLists, UINT commandListCount
	) const noexcept;

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
