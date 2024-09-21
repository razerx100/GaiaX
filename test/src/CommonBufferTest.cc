#include <DeviceManager.hpp>
#include <gtest/gtest.h>
#include <memory>

#include <CommonBuffers.hpp>

namespace Constants
{
	constexpr size_t bufferCount = 2u;
}

class CommonBufferTest : public ::testing::Test
{
protected:
	static void SetUpTestSuite();
	static void TearDownTestSuite();

protected:
	inline static std::unique_ptr<DeviceManager> s_deviceManager;
};

void CommonBufferTest::SetUpTestSuite()
{
	s_deviceManager = std::make_unique<DeviceManager>();

	s_deviceManager->GetDebugLogger().AddCallbackType(DebugCallbackType::StandardError);
	s_deviceManager->Create(D3D_FEATURE_LEVEL_12_0);
}

void CommonBufferTest::TearDownTestSuite()
{
	s_deviceManager.reset();
}

TEST_F(CommonBufferTest, SharedBufferTest)
{
	ID3D12Device5* device  = s_deviceManager->GetDevice();
	IDXGIAdapter3* adapter = s_deviceManager->GetAdapter();

	MemoryManager memoryManager{ adapter, device, 20_MB, 200_KB };

	SharedBufferGPU sharedBuffer{ device, &memoryManager, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS };

	TemporaryDataBufferGPU tempDataBuffer{};

	{
		auto allocInfo = sharedBuffer.AllocateAndGetSharedData(12u, tempDataBuffer);
		UINT64 offset  = allocInfo.offset;
		UINT64 size    = sharedBuffer.Size();

		EXPECT_EQ(offset, 0u) << "Offset isn't 0.";
		EXPECT_EQ(size, 12u) << "Size isn't 12Bytes.";
	}

	{
		auto allocInfo = sharedBuffer.AllocateAndGetSharedData(12u, tempDataBuffer);
		UINT64 offset  = allocInfo.offset;
		UINT64 size    = sharedBuffer.Size();

		EXPECT_EQ(offset, 12u) << "Offset isn't 12.";
		EXPECT_EQ(size, 24u) << "Size isn't 24Bytes.";

		sharedBuffer.RelinquishMemory(allocInfo);
	}

	{
		auto allocInfo = sharedBuffer.AllocateAndGetSharedData(12u, tempDataBuffer);
		UINT64 offset  = allocInfo.offset;
		UINT64 size    = sharedBuffer.Size();

		EXPECT_EQ(offset, 12u) << "Offset isn't 12.";
		EXPECT_EQ(size, 24u) << "Size isn't 24Bytes.";
	}

	{
		auto allocInfo = sharedBuffer.AllocateAndGetSharedData(12u, tempDataBuffer);
		UINT64 offset  = allocInfo.offset;
		UINT64 size    = sharedBuffer.Size();

		EXPECT_EQ(offset, 24u) << "Offset isn't 12.";
		EXPECT_EQ(size, 36u) << "Size isn't 36Bytes.";
	}

	const size_t midOffset = 36u;

	{
		auto allocInfo = sharedBuffer.AllocateAndGetSharedData(20_KB, tempDataBuffer);
		UINT64 offset  = allocInfo.offset;
		UINT64 size    = sharedBuffer.Size();

		EXPECT_EQ(offset, 0u + midOffset) << "Offset isn't 0.";
		EXPECT_EQ(size, 20_KB + midOffset) << "Size isn't 20KB.";
	}

	{
		D3DCommandQueue copyQueue{};
		copyQueue.Create(device, D3D12_COMMAND_LIST_TYPE_COPY, 1u);

		const size_t bufferIndex = 0u;

		const D3DCommandList& copyList = copyQueue.GetCommandList(bufferIndex);
		{
			CommandListScope scope{ copyList };

			sharedBuffer.CopyOldBuffer(scope);
			tempDataBuffer.SetUsed(bufferIndex);
		}

		D3DFence fence{};
		fence.Create(device);

		QueueSubmitBuilder<0u, 1u> submitBuilder{};
		submitBuilder.SignalFence(fence).CommandList(copyList);

		copyQueue.SubmitCommandLists(submitBuilder);

		fence.Wait(1u);

		tempDataBuffer.Clear(bufferIndex);
	}

	auto twentyKBAllocInfo = sharedBuffer.AllocateAndGetSharedData(30_KB, tempDataBuffer);
	{
		UINT64 offset = twentyKBAllocInfo.offset;
		UINT64 size   = sharedBuffer.Size();

		EXPECT_EQ(offset, 20_KB + midOffset) << "Offset isn't 20KB.";
		EXPECT_EQ(size, 20_KB + 30_KB + midOffset) << "Size isn't 50KB.";
	}

	{
		auto allocInfo = sharedBuffer.AllocateAndGetSharedData(50_KB, tempDataBuffer);
		UINT64 offset  = allocInfo.offset;
		UINT64 size    = sharedBuffer.Size();

		EXPECT_EQ(offset, 50_KB + midOffset) << "Offset isn't 50KB.";
		EXPECT_EQ(size, 100_KB + midOffset) << "Size isn't 100KB.";
	}

	sharedBuffer.RelinquishMemory(twentyKBAllocInfo);

	{
		auto allocInfo = sharedBuffer.AllocateAndGetSharedData(20_KB, tempDataBuffer);
		UINT64 offset  = allocInfo.offset;
		UINT64 size    = sharedBuffer.Size();

		EXPECT_EQ(offset, 20_KB + midOffset) << "Offset isn't 20KB.";
		EXPECT_EQ(size, 100_KB + midOffset) << "Size isn't 100KB.";
	}

	{
		auto allocInfo = sharedBuffer.AllocateAndGetSharedData(10_KB, tempDataBuffer);
		UINT64 offset  = allocInfo.offset;
		UINT64 size    = sharedBuffer.Size();

		EXPECT_EQ(offset, 40_KB + midOffset) << "Offset isn't 40KB.";
		EXPECT_EQ(size, 100_KB + midOffset) << "Size isn't 100KB.";
	}
}

class MaterialDummy : public Material
{
public:
	MaterialData Get() const noexcept override { return {}; }
};

TEST_F(CommonBufferTest, MaterialBufferTest)
{
	ID3D12Device5* device  = s_deviceManager->GetDevice();
	IDXGIAdapter3* adapter = s_deviceManager->GetAdapter();

	MemoryManager memoryManager{ adapter, device, 20_MB, 200_KB };

	MaterialBuffers materialBuffers{ device, &memoryManager };

	size_t index = materialBuffers.Add(std::make_shared<MaterialDummy>());
	materialBuffers.Update(index);
}
