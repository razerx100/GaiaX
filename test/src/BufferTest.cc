#include <DeviceManager.hpp>
#include <D3DHeap.hpp>
#include <D3DHelperFunctions.hpp>
#include <gtest/gtest.h>
#include <memory>

class BufferTest : public ::testing::Test
{
protected:
	static void SetUpTestSuite();
	static void TearDownTestSuite();

protected:
	inline static std::unique_ptr<DeviceManager> s_deviceManager;
};

void BufferTest::SetUpTestSuite()
{
	s_deviceManager = std::make_unique<DeviceManager>();
	s_deviceManager->Create(D3D_FEATURE_LEVEL_12_0);
}

void BufferTest::TearDownTestSuite()
{
	s_deviceManager.reset();
}

TEST_F(BufferTest, D3DHeapTest)
{
	ID3D12Device* device = s_deviceManager->GetDevice();

	D3DHeap heap{ device, D3D12_HEAP_TYPE_DEFAULT, 111_MB };

	EXPECT_EQ(heap.Size(), 111_MB) << "Size isn't 111MB";
}
