#include <DebugInfoManager.hpp>
#include <D3DExceptions.hpp>

DebugInfoManager::DebugInfoManager()
	: m_next(0u), m_pDxgiInfoQueue(nullptr) {

	typedef HRESULT(WINAPI* DXGIGetDebugInterface)(REFIID, void**);

	HMODULE hmodDxgiDebug = LoadLibraryExA(
		"dxgidebug.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32
	);

	if (!hmodDxgiDebug)
		throw GenericException(
			__LINE__, __FILE__,
			"Couldn't load dxgidebug.dll."
		);

	DXGIGetDebugInterface dxgiGetDebugInterface =
		reinterpret_cast<DXGIGetDebugInterface>(
			GetProcAddress(hmodDxgiDebug, "DXGIGetDebugInterface")
			);

	if(!dxgiGetDebugInterface)
		throw GenericException(
			__LINE__, __FILE__,
			"Couldn't load DXGIGetDebugInterface."
		);

	dxgiGetDebugInterface(__uuidof(IDXGIInfoQueue), &m_pDxgiInfoQueue);

	FreeLibrary(hmodDxgiDebug);
}

void DebugInfoManager::Set() noexcept {
	// Set the index (next) so that the next call to GetMessages()
	// will only get errors generated after this call
	m_next = m_pDxgiInfoQueue->GetNumStoredMessages(DXGI_DEBUG_ALL);
}

std::vector<std::string> DebugInfoManager::GetMessages() const {
	std::vector<std::string> messages;
	const std::uint64_t end = m_pDxgiInfoQueue->GetNumStoredMessages(DXGI_DEBUG_ALL);

	for (std::uint64_t i = m_next; i < end; i++) {
		SIZE_T messageLength = 0;
		m_pDxgiInfoQueue->GetMessageA(
			DXGI_DEBUG_ALL, i, nullptr, &messageLength
		);

		byte* bytes = new byte[messageLength];
		DXGI_INFO_QUEUE_MESSAGE* pMessage =
			reinterpret_cast<DXGI_INFO_QUEUE_MESSAGE*>(bytes);

		m_pDxgiInfoQueue->GetMessageA(
			DXGI_DEBUG_ALL, i, pMessage, &messageLength
		);

		if (pMessage)
			messages.emplace_back(pMessage->pDescription);

		delete[] bytes;
	}

	return messages;
}

#ifdef _DEBUG
static DebugInfoManager* s_pDebugInfoManager = nullptr;

void InitDebugInfoManagerInstance() {
	if (!s_pDebugInfoManager)
		s_pDebugInfoManager = new DebugInfoManager();
}

DebugInfoManager* GetDebugInfoManagerInstance() noexcept {
	return s_pDebugInfoManager;
}

void CleanUpDebugInfoManagerInstance() noexcept {
	if (s_pDebugInfoManager)
		delete s_pDebugInfoManager;
}
#endif
