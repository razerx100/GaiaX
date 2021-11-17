#ifndef __I_COMMAND_LIST_MANAGER_HPP__
#define __I_COMMAND_LIST_MANAGER_HPP__
#include <D3DHeaders.hpp>
#include <cstdint>

class ICommandListManager {
public:
	virtual ~ICommandListManager() = default;

	virtual void Reset(std::uint32_t allocIndex) const = 0;
	virtual void Close() const = 0;

	virtual ID3D12GraphicsCommandList* GetCommandListRef() const noexcept = 0;
};

ICommandListManager* GetGraphicsListInstance() noexcept;
void InitGraphicsListInstance(
	ID3D12Device5* device,
	std::uint8_t bufferCount
);
void CleanUpGraphicsListInstance() noexcept;
#endif