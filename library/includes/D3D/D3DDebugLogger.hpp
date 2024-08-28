#ifndef D3D_DEBUG_LOGGER_HPP_
#define D3D_DEBUG_LOGGER_HPP_
#include <D3DHeaders.hpp>
#include <optional>
#include <vector>
#include <bitset>
#include <type_traits>

enum class DebugCallbackType
{
	StandardError,
	FileOut,
	None
};

class DebugCallbackManager
{
public:
	DebugCallbackManager() : m_callbackTypes{} {}

	void AddDebugCallback(DebugCallbackType type) noexcept;

	[[nodiscard]]
	size_t GetCallbackCount() const noexcept { return std::size(m_callbackTypes); }
	[[nodiscard]]
	bool DoesCallbackExist(size_t index) const noexcept { return m_callbackTypes.test(index); }
	[[nodiscard]]
	D3D12MessageFunc GetCallback(size_t index) const noexcept;

	static void __stdcall MessageCallbackTxt(
		D3D12_MESSAGE_CATEGORY category, D3D12_MESSAGE_SEVERITY severity,
		D3D12_MESSAGE_ID id, LPCSTR pDescription, void* pContext
	);
	static void __stdcall MessageCallbackStdError(
		D3D12_MESSAGE_CATEGORY category, D3D12_MESSAGE_SEVERITY severity,
		D3D12_MESSAGE_ID id, LPCSTR pDescription, void* pContext
	);

private:
	[[nodiscard]]
	static std::string __stdcall FormatDebugMessage(
		D3D12_MESSAGE_CATEGORY category, D3D12_MESSAGE_SEVERITY severity,
		D3D12_MESSAGE_ID id, LPCSTR pDescription
	);

private:
	std::bitset<static_cast<size_t>(DebugCallbackType::None)> m_callbackTypes;
};

class D3DInfoQueueLogger
{
public:
	D3DInfoQueueLogger() : m_debugInfoQueue{}, m_callbackManager{} {}
	~D3DInfoQueueLogger() noexcept;

	void CreateInfoQueue(ID3D12Device* device);
	void AddCallbackType(DebugCallbackType type) noexcept;
	// Since callbacks aren't supported until InfoQueue1,
	// the messages need to be fectched manually.
	void OutputCurrentMessages();

private:
	ComPtr<ID3D12InfoQueue> m_debugInfoQueue;
	DebugCallbackManager    m_callbackManager;

public:
	D3DInfoQueueLogger(const D3DInfoQueueLogger&) = delete;
	D3DInfoQueueLogger& operator=(const D3DInfoQueueLogger&) = delete;

	D3DInfoQueueLogger(D3DInfoQueueLogger&& other) noexcept
		: m_debugInfoQueue{ std::move(other.m_debugInfoQueue) },
		m_callbackManager{ std::move(other.m_callbackManager) }
	{}
	D3DInfoQueueLogger& operator=(D3DInfoQueueLogger&& other) noexcept
	{
		m_debugInfoQueue  = std::move(other.m_debugInfoQueue);
		m_callbackManager = std::move(other.m_callbackManager);

		return *this;
	}
};

#ifdef __ID3D12InfoQueue1_INTERFACE_DEFINED__
class D3DInfoQueue1Logger
{
public:
	D3DInfoQueue1Logger() : m_debugInfoQueue{}, m_callbackManager{}, m_callbackCookies{} {}
	~D3DInfoQueue1Logger() noexcept;

	void CreateInfoQueue(ID3D12Device* device);
	void AddCallbackType(DebugCallbackType type) noexcept;

private:
	void UnregisterCallbacks() noexcept;
	void CreateDebugCallbacks();

private:
	ComPtr<ID3D12InfoQueue1> m_debugInfoQueue;
	DebugCallbackManager     m_callbackManager;
	std::vector<DWORD>       m_callbackCookies;

public:
	D3DInfoQueue1Logger(const D3DInfoQueue1Logger&) = delete;
	D3DInfoQueue1Logger& operator=(const D3DInfoQueue1Logger&) = delete;

	D3DInfoQueue1Logger(D3DInfoQueue1Logger&& other) noexcept
		: m_debugInfoQueue{ std::move(other.m_debugInfoQueue) },
		m_callbackManager{ std::move(other.m_callbackManager) },
		m_callbackCookies{ std::move(other.m_callbackCookies) }
	{}
	D3DInfoQueue1Logger& operator=(D3DInfoQueue1Logger&& other) noexcept
	{
		m_debugInfoQueue  = std::move(other.m_debugInfoQueue);
		m_callbackManager = std::move(other.m_callbackManager);
		m_callbackCookies = std::move(other.m_callbackCookies);

		return *this;
	}
};
#endif // __ID3D12InfoQueue1_INTERFACE_DEFINED__

template<typename InfoQueue>
class D3DDebugLoggerGeneric
{
public:
	D3DDebugLoggerGeneric() : m_debugLogger{} {}

	void CreateInfoQueue(ID3D12Device* device)
	{
		m_debugLogger.CreateInfoQueue(device);
	}
	D3DDebugLoggerGeneric& AddCallbackType(DebugCallbackType type) noexcept
	{
		m_debugLogger.AddCallbackType(type);

		return *this;
	}
	void OutputCurrentMessages()
	{
		if constexpr (std::is_same_v<InfoQueue, ID3D12InfoQueue>)
			m_debugLogger.OutputCurrentMessages();
	}

private:
	InfoQueue m_debugLogger;

public:
	D3DDebugLoggerGeneric(const D3DDebugLoggerGeneric&) = delete;
	D3DDebugLoggerGeneric& operator=(const D3DDebugLoggerGeneric&) = delete;

	D3DDebugLoggerGeneric(D3DDebugLoggerGeneric&& other) noexcept
		: m_debugLogger{ std::move(other.m_debugLogger) }
	{}
	D3DDebugLoggerGeneric& operator=(D3DDebugLoggerGeneric&& other) noexcept
	{
		m_debugLogger = std::move(other.m_debugLogger);

		return *this;
	}
};

#ifdef __ID3D12InfoQueue1_INTERFACE_DEFINED__
using D3DDebugLogger = D3DDebugLoggerGeneric<D3DInfoQueue1Logger>;
#else
using D3DDebugLogger = D3DDebugLoggerGeneric<D3DInfoQueueLogger>;
#endif // __ID3D12InfoQueue1_INTERFACE_DEFINED__
#endif
