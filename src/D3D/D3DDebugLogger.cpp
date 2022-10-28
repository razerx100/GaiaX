#include <D3DDebugLogger.hpp>
#include <fstream>
#include <array>

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

D3DDebugLogger::D3DDebugLogger(ID3D12Device* device) : m_callBackCookie{0u} {
	device->QueryInterface(IID_PPV_ARGS(&m_debugInfoQueue));

	m_debugInfoQueue->RegisterMessageCallback(
		D3DDebugLogger::MessageCallback, D3D12_MESSAGE_CALLBACK_FLAG_NONE, nullptr,
		&m_callBackCookie
	);
}

D3DDebugLogger::~D3DDebugLogger() noexcept {
	m_debugInfoQueue->UnregisterMessageCallback(m_callBackCookie);
}

void D3DDebugLogger::MessageCallback(
	D3D12_MESSAGE_CATEGORY category, D3D12_MESSAGE_SEVERITY severity,
	D3D12_MESSAGE_ID id, LPCSTR pDescription, void* pContext
) {
	std::ofstream log("ErrorLog.txt", std::ios_base::app | std::ios_base::out);
	log << "Category : " << messageCategories[category] << "    "
		<< "Severity : " << messageSeverity[severity] << "    "
		<< "ID : " << id << "    "
		<< "Description : " << pDescription << "    "
		<< std::endl;
}
