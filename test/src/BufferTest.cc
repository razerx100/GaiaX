#include <DeviceManager.hpp>
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
