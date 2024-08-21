#include <DeviceManager.hpp>
#include <D3DResource.hpp>
#include <gtest/gtest.h>
#include <memory>

class AllocatorTest : public ::testing::Test
{
protected:
	static void SetUpTestSuite();
	static void TearDownTestSuite();

protected:
	inline static std::unique_ptr<DeviceManager> s_deviceManager;
};

void AllocatorTest::SetUpTestSuite()
{
	s_deviceManager = std::make_unique<DeviceManager>();
	s_deviceManager->Create(D3D_FEATURE_LEVEL_12_0);
}

void AllocatorTest::TearDownTestSuite()
{
	s_deviceManager.reset();
}

TEST_F(AllocatorTest, D3DAllocatorTest)
{
	ID3D12Device* device = s_deviceManager->GetDevice();

	D3DHeap heap{ device, D3D12_HEAP_TYPE_DEFAULT, 111_MB };

	D3DAllocator allocator{ std::move(heap), 0u };

	EXPECT_EQ(allocator.AvailableSize(), 111_MB) << "The available size isn't 111MB.";

	auto offset = allocator.Allocate({ .SizeInBytes = 5_MB, .Alignment = 64_KB });

	if (offset)
		EXPECT_EQ(offset.value(), 7_MB) << "The offset isn't 7MB.";

	EXPECT_EQ(allocator.AvailableSize(), 103_MB) << "The available size isn't 103MB.";

	allocator.Deallocate(7_MB, 5_MB, 64_KB);

	EXPECT_EQ(allocator.AvailableSize(), 111_MB) << "The available size isn't 111MB.";
}

TEST_F(AllocatorTest, MemoryManagerTest)
{
	ID3D12Device* device   = s_deviceManager->GetDevice();
	IDXGIAdapter3* adapter = s_deviceManager->GetAdapter();

	MemoryManager memoryManager{ adapter, device, 2_MB, 200_KB };

	{
		Buffer buffer{ device, &memoryManager, D3D12_HEAP_TYPE_DEFAULT };
		buffer.Create(1_KB, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		EXPECT_NE(buffer.Get(), nullptr) << "Buffer wasn't initialised";
		EXPECT_EQ(buffer.CPUHandle(), nullptr) << "CPU Pointer isn't null.";
		EXPECT_EQ(buffer.BufferSize(), 1_KB) << "BufferSize doesn't match.";
	}
}
