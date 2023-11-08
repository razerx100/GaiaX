#ifndef D3D_DEBUG_LOGGER_HPP_
#define D3D_DEBUG_LOGGER_HPP_
#include <D3DHeaders.hpp>
#include <optional>


class D3DDebugLogger {
public:
	struct Args {
		std::optional<ID3D12Device*> device;
	};

public:
	D3DDebugLogger(const Args& arguments);
	~D3DDebugLogger() noexcept;

	void UnregisterCallBack() const noexcept;

	static void MessageCallback(
		D3D12_MESSAGE_CATEGORY category, D3D12_MESSAGE_SEVERITY severity,
		D3D12_MESSAGE_ID id, LPCSTR pDescription, void* pContext
	);

private:
	DWORD m_callBackCookie;
#ifdef __ID3D12InfoQueue1_INTERFACE_DEFINED__
	ComPtr<ID3D12InfoQueue1> m_debugInfoQueue;
#else
	ComPtr<ID3D12InfoQueue> m_debugInfoQueue;
#endif
};
#endif
