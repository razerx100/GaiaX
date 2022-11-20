#ifndef D3D_DEBUG_LOGGER_HPP_
#define D3D_DEBUG_LOGGER_HPP_
#include <D3DHeaders.hpp>

class D3DDebugLogger {
public:
	D3DDebugLogger(ID3D12Device* device);
	~D3DDebugLogger() noexcept;

	void UnregisterCallBack() const noexcept;

	static void MessageCallback(
		D3D12_MESSAGE_CATEGORY category, D3D12_MESSAGE_SEVERITY severity,
		D3D12_MESSAGE_ID id, LPCSTR pDescription, void* pContext
	);

private:
	DWORD m_callBackCookie;
	ComPtr<ID3D12InfoQueue1> m_debugInfoQueue;
};
#endif