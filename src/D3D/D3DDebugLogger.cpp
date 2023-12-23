#include <D3DDebugLogger.hpp>
#include <fstream>
#include <array>
#include <format>

static std::array<const char*, 11u> messageCategories{
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

static std::array<const char*, 5u> messageSeverity{
	"D3D12_MESSAGE_SEVERITY_CORRUPTION",
	"D3D12_MESSAGE_SEVERITY_ERROR",
	"D3D12_MESSAGE_SEVERITY_WARNING",
	"D3D12_MESSAGE_SEVERITY_INFO",
	"D3D12_MESSAGE_SEVERITY_MESSAGE"
};
D3DDebugLogger::D3DDebugLogger(ID3D12Device* device)
	: m_callBackCookie(0)
{
#ifdef __ID3D12InfoQueue1_INTERFACE_DEFINED__
	HRESULT hr = device->QueryInterface(IID_PPV_ARGS(&m_debugInfoQueue));
	std::string errorMessage = "";
	if (hr == S_OK) {
		m_debugInfoQueue->RegisterMessageCallback(
			D3DDebugLogger::MessageCallback, D3D12_MESSAGE_CALLBACK_FLAG_NONE, nullptr,
			&m_callBackCookie
		);
	}
	else if (hr == E_NOINTERFACE) {
		errorMessage = std::format("ID3D12InfoQueue1 isn't supported. Upgrade to Windows 11. HRESULT: 0x{0:X}\n", static_cast<unsigned>(hr)).c_str();
	}
	else {
		errorMessage = std::format("Something went wrong while initializing debugInfoQueue. HRESULT:0x{0:X}\n", static_cast<unsigned>(hr)).c_str();
}
	if (hr != S_OK) {
		std::ofstream{ "ErrorLog.txt", std::ios_base::app | std::ios_base::out } << errorMessage;
		OutputDebugStringA(errorMessage.c_str());
	}
#else
	std::string errorInterfaceNotFoundMessage = "Failed to initalize D3D12DebugLogger: ID3D12InfoQueue1 interface is not defined. Upgrade to Windows 11 or use DX12 Agility SDK from NuGet.\n";
	OutputDebugStringA(errorInterfaceNotFoundMessage.c_str());
	std::ofstream{ "ErrorLog.txt", std::ios_base::app | std::ios_base::out } << errorInterfaceNotFoundMessage << std::endl;
#endif
}

D3DDebugLogger::~D3DDebugLogger() noexcept {
	UnregisterCallBack();
}

void D3DDebugLogger::UnregisterCallBack() const noexcept {
#ifdef __ID3D12InfoQueue1_INTERFACE_DEFINED__
	if (m_debugInfoQueue) {
		m_debugInfoQueue->UnregisterMessageCallback(m_callBackCookie);
	}
#endif
}

void D3DDebugLogger::MessageCallback(
	D3D12_MESSAGE_CATEGORY category, D3D12_MESSAGE_SEVERITY severity,
	D3D12_MESSAGE_ID id, LPCSTR pDescription, [[maybe_unused]] void* pContext
) {
	std::ofstream log("ErrorLog.txt", std::ios_base::app | std::ios_base::out);
	log << "Category : " << messageCategories[category] << "    "
		<< "Severity : " << messageSeverity[severity] << "    "
		<< "ID : " << id << "\n"
		<< "Description : " << pDescription << "    "
		<< std::endl;
}
