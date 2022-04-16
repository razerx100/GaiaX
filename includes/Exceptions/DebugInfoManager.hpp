#ifndef DXGI_INFO_MANAGER_HPP_
#define DXGI_INFO_MANAGER_HPP_
#include <D3DHeaders.hpp>
#include <vector>
#include <string>
#include <dxgidebug.h>

using Microsoft::WRL::ComPtr;

class DebugInfoManager {
public:
	DebugInfoManager();

	DebugInfoManager(const DebugInfoManager&) = delete;
	DebugInfoManager& operator=(const DebugInfoManager&) = delete;

	void Set() noexcept;

	[[nodiscard]]
	std::vector<std::string> GetMessages() const;

private:
	std::uint64_t m_next;
	ComPtr<IDXGIInfoQueue> m_pDxgiInfoQueue;
};

DebugInfoManager* CreateDebugInfoManagerInstance();

#endif