#ifndef __DXGI_INFO_MANAGER_HPP__
#define __DXGI_INFO_MANAGER_HPP__
#include <D3DHeaders.hpp>
#include <vector>
#include <string>

using Microsoft::WRL::ComPtr;

class DebugInfoManager {
public:
	DebugInfoManager();

	DebugInfoManager(const DebugInfoManager&) = delete;
	DebugInfoManager& operator=(const DebugInfoManager&) = delete;

	void Set() noexcept;
	std::vector<std::string> GetMessages() const;

	ComPtr<ID3D12InfoQueue> m_pInfoQueue;
private:
	std::uint64_t m_next;
};

#ifdef _DEBUG
	DebugInfoManager* GetDebugInfoManagerInstance() noexcept;
	void InitDebugInfoManagerInstance(ID3D12Device5* device);
	void CleanUpDebugInfoManagerInstance() noexcept;
#endif
#endif