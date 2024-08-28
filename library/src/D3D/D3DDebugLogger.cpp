#include <D3DDebugLogger.hpp>
#include <fstream>
#include <iostream>
#include <array>
#include <format>

static std::array<const char*, 11u> messageCategories
{
	"D3D12_MESSAGE_CATEGORY_APPLICATION_DEFINED",
	"D3D12_MESSAGE_CATEGORY_MISCELLANEOUS",
	"D3D12_MESSAGE_CATEGORY_INITIALIZATION",
	"D3D12_MESSAGE_CATEGORY_CLEANUP",
	"D3D12_MESSAGE_CATEGORY_COMPILATION",
	"D3D12_MESSAGE_CATEGORY_STATE_CREATION",
	"D3D12_MESSAGE_CATEGORY_STATE_SETTING",
	"D3D12_MESSAGE_CATEGORY_STATE_GETTING",
	"D3D12_MESSAGE_CATEGORY_RESOURCE_MANIPULATION",
	"D3D12_MESSAGE_CATEGORY_EXECUTION",
	"D3D12_MESSAGE_CATEGORY_SHADER"
};

static std::array<const char*, 5u> messageSeverity
{
	"D3D12_MESSAGE_SEVERITY_CORRUPTION",
	"D3D12_MESSAGE_SEVERITY_ERROR",
	"D3D12_MESSAGE_SEVERITY_WARNING",
	"D3D12_MESSAGE_SEVERITY_INFO",
	"D3D12_MESSAGE_SEVERITY_MESSAGE"
};

// Debug Callback Manager
void DebugCallbackManager::AddDebugCallback(DebugCallbackType type) noexcept
{
	m_callbackTypes.set(static_cast<size_t>(type));
}

D3D12MessageFunc DebugCallbackManager::GetCallback(size_t index) const noexcept
{
	auto type = static_cast<DebugCallbackType>(index);

	if (type == DebugCallbackType::FileOut)
		return &DebugCallbackManager::MessageCallbackTxt;
	else
		return &DebugCallbackManager::MessageCallbackStdError;
}

std::string DebugCallbackManager::FormatDebugMessage(
	D3D12_MESSAGE_CATEGORY category, D3D12_MESSAGE_SEVERITY severity,
	D3D12_MESSAGE_ID id, LPCSTR pDescription
) {
	return std::format(
		"Category : {}    Severity : {}    ID : {}.\nDescription : {}\n",
		messageCategories[category], messageSeverity[severity], static_cast<int>(id),
		pDescription
	);
}

void __stdcall DebugCallbackManager::MessageCallbackTxt(
	D3D12_MESSAGE_CATEGORY category, D3D12_MESSAGE_SEVERITY severity,
	D3D12_MESSAGE_ID id, LPCSTR pDescription, [[maybe_unused]] void* pContext
) {
	std::ofstream log("ErrorLog.txt", std::ios_base::app | std::ios_base::out);
	log << FormatDebugMessage(category, severity, id, pDescription);
}

void __stdcall DebugCallbackManager::MessageCallbackStdError(
	D3D12_MESSAGE_CATEGORY category, D3D12_MESSAGE_SEVERITY severity,
	D3D12_MESSAGE_ID id, LPCSTR pDescription, [[maybe_unused]] void* pContext
) {
	std::cerr << FormatDebugMessage(category, severity, id, pDescription);
}

// D3DInfoQueueLogger
D3DInfoQueueLogger::~D3DInfoQueueLogger() noexcept
{
	OutputCurrentMessages();
}

void D3DInfoQueueLogger::CreateInfoQueue(ID3D12Device* device)
{
	device->QueryInterface(IID_PPV_ARGS(&m_debugInfoQueue));
}

void D3DInfoQueueLogger::AddCallbackType(DebugCallbackType type) noexcept
{
	m_callbackManager.AddDebugCallback(type);
}

void D3DInfoQueueLogger::OutputCurrentMessages()
{
	const UINT64 storedMessageCount = m_debugInfoQueue->GetNumStoredMessages();

	std::vector<std::uint8_t> messageBuffer{};

	for (UINT64 index = 0u; index < storedMessageCount; ++index)
	{
		SIZE_T messageLength = 0u;
		m_debugInfoQueue->GetMessage(index, nullptr, &messageLength);

		if (std::size(messageBuffer) < messageLength)
			messageBuffer.resize(messageLength);

		auto message = reinterpret_cast<D3D12_MESSAGE*>(std::data(messageBuffer));
		m_debugInfoQueue->GetMessage(index, message, &messageLength);

		for (size_t index = 0u; index < m_callbackManager.GetCallbackCount(); ++index)
			if (m_callbackManager.DoesCallbackExist(index))
			{
				D3D12MessageFunc callback = m_callbackManager.GetCallback(index);

				callback(
					message->Category, message->Severity, message->ID, message->pDescription, nullptr
				);
			}
	}
}

// D3DInfoQueue1Logger
#ifdef __ID3D12InfoQueue1_INTERFACE_DEFINED__
D3DInfoQueue1Logger::~D3DInfoQueue1Logger() noexcept
{
	UnregisterCallbacks();
}

void D3DInfoQueue1Logger::CreateInfoQueue(ID3D12Device* device)
{
	device->QueryInterface(IID_PPV_ARGS(&m_debugInfoQueue));

	CreateDebugCallbacks();
}

void D3DInfoQueue1Logger::AddCallbackType(DebugCallbackType type) noexcept
{
	m_callbackManager.AddDebugCallback(type);
}

void D3DInfoQueue1Logger::CreateDebugCallbacks()
{
	for (size_t index = 0u; index < m_callbackManager.GetCallbackCount(); ++index)
		if (m_callbackManager.DoesCallbackExist(index))
		{
			DWORD& callbackCookie     = m_callbackCookies.emplace_back(0u);
			D3D12MessageFunc callback = m_callbackManager.GetCallback(index);

			// This could be some user defined data. So, it can be used to send in an
			// object to a custom logger.
			// I would want to set it to null but the third parameter has an _In_, so
			// if I put a nullptr there, it shows a warning. Sending the address to
			// the cookie, as it should be alive as long as the callbacks are. And
			// for now that argument isn't used.
			void* context = &callbackCookie;

			m_debugInfoQueue->RegisterMessageCallback(
				callback, D3D12_MESSAGE_CALLBACK_FLAG_NONE, context, &callbackCookie
			);
		}
}

void D3DInfoQueue1Logger::UnregisterCallbacks() noexcept
{
	for (DWORD callbackCookie : m_callbackCookies)
		m_debugInfoQueue->UnregisterMessageCallback(callbackCookie);
}
#endif // __ID3D12InfoQueue1_INTERFACE_DEFINED__
