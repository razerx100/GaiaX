#include <DebugInfoManager.hpp>
#include <D3DExceptions.hpp>
#include <D3DThrowMacros.hpp>

DebugInfoManager::DebugInfoManager()
	: m_next(0u), m_pDxgiInfoQueue(nullptr) {

	typedef HRESULT(WINAPI* DXGIGetDebugInterface)(REFIID, void**);

	HMODULE hmodDxgiDebug = LoadLibraryExA(
		"dxgidebug.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32
	);

	if (!hmodDxgiDebug)
		D3D_GENERIC_THROW("Couldn't load dxgidebug.dll.");

	auto dxgiGetDebugInterface =
		reinterpret_cast<DXGIGetDebugInterface>(
			GetProcAddress(hmodDxgiDebug, "DXGIGetDebugInterface")
			);

	if (!dxgiGetDebugInterface)
		D3D_GENERIC_THROW("Couldn't load DXGIGetDebugInterface.");

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

	for (std::uint64_t index = m_next; index < end; ++index) {
		SIZE_T messageLength = 0u;
		m_pDxgiInfoQueue->GetMessageA(
			DXGI_DEBUG_ALL, index, nullptr, &messageLength
		);

		byte* bytes = new byte[messageLength];
		auto pMessage =
			reinterpret_cast<DXGI_INFO_QUEUE_MESSAGE*>(bytes);

		m_pDxgiInfoQueue->GetMessageA(
			DXGI_DEBUG_ALL, index, pMessage, &messageLength
		);

		if (pMessage)
			messages.emplace_back(pMessage->pDescription);

		delete[] bytes;
	}

	return messages;
}

DebugInfoManager* CreateDebugInfoManagerInstance() {
	return new DebugInfoManager();
}
