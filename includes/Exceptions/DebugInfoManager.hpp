#ifndef __DXGI_INFO_MANAGER_HPP__
#define __DXGI_INFO_MANAGER_HPP__
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
	std::vector<std::string> GetMessages() const;

private:
	std::uint64_t m_next;
	ComPtr<IDXGIInfoQueue> m_pDxgiInfoQueue;
};

#ifdef _DEBUG
	DebugInfoManager* GetDebugInfoManagerInstance() noexcept;
	void InitDebugInfoManagerInstance();
	void CleanUpDebugInfoManagerInstance() noexcept;
#endif
#endif