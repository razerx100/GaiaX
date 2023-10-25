#include <D3DDebugLogger.hpp>
#include <fstream>
#include <array>

#ifndef __ID3D12InfoQueue1_INTERFACE_DEFINED__
D3DDebugLogger::D3DDebugLogger(const Args& arguments) {
	OutputDebugStringA("Failed to initalize D3D12DebugLogger: ID3D12InfoQueue1 interface is not defined. Upgrade to Windows 11 or use DX12 Agility SDK from NuGet.\n");
	std::ofstream{ "ErrorLog.txt", std::ios_base::app | std::ios_base::out } << "Failed to initalize D3D12DebugLogger: ID3D12InfoQueue1 interface is not defined. Upgrade to Windows 11 or use DX12 Agility SDK from NuGet." << std::endl;
}
D3DDebugLogger::~D3DDebugLogger() noexcept {
}

#else
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
D3DDebugLogger::D3DDebugLogger(const Args& arguments) : m_callBackCookie{ 0u } {
	HRESULT hr = arguments.device.value()->QueryInterface(IID_PPV_ARGS(&m_debugInfoQueue));
	if (hr == S_OK) {
		m_debugInfoQueue->RegisterMessageCallback(
			D3DDebugLogger::MessageCallback, D3D12_MESSAGE_CALLBACK_FLAG_NONE, nullptr,
			&m_callBackCookie
		);
	}
	else if (hr == E_NOINTERFACE) {
		std::ofstream{ "ErrorLog.txt", std::ios_base::app | std::ios_base::out } << std::format("ID3D12InfoQueue1 isn't supported. Upgrade to Windows 11. HRESULT: 0x{0:X}", static_cast<unsigned>(hr)).c_str() << std::endl;
	}
	else {
		std::ofstream{ "ErrorLog.txt", std::ios_base::app | std::ios_base::out } << std::format("Something went wrong while initializing debugInfoQueue. HRESULT:0x{0:X}", static_cast<unsigned>(hr)).c_str() << std::endl;
	}
}

D3DDebugLogger::~D3DDebugLogger() noexcept {
	UnregisterCallBack();
}

void D3DDebugLogger::UnregisterCallBack() const noexcept {
	if (m_debugInfoQueue) {
		m_debugInfoQueue->UnregisterMessageCallback(m_callBackCookie);
	}
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
#endif