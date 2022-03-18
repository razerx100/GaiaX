#ifndef __I_COMMAND_LIST_MANAGER_HPP__
#define __I_COMMAND_LIST_MANAGER_HPP__
#include <D3DHeaders.hpp>
#include <cstdint>

class ICommandListManager {
public:
	virtual ~ICommandListManager() = default;

	virtual void Reset(size_t allocIndex) = 0;
	virtual void Close() const = 0;

	virtual ID3D12GraphicsCommandList* GetCommandListRef() const noexcept = 0;
};

ICommandListManager* CreateCommandListInstance(
	ID3D12Device5* device,
	D3D12_COMMAND_LIST_TYPE type,
	size_t bufferCount
);

#endif