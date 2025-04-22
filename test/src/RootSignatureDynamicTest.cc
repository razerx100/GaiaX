#include <D3DDeviceManager.hpp>
#include <D3DRootSignature.hpp>
#include <gtest/gtest.h>
#include <memory>

class DynamicRootSignatureTest : public ::testing::Test
{
protected:
	static void SetUpTestSuite();
	static void TearDownTestSuite();

protected:
	inline static std::unique_ptr<DeviceManager> s_deviceManager;
};

void DynamicRootSignatureTest::SetUpTestSuite()
{
	s_deviceManager = std::make_unique<DeviceManager>();

	s_deviceManager->GetDebugLogger().AddCallbackType(DebugCallbackType::StandardError);
	s_deviceManager->Create(D3D_FEATURE_LEVEL_12_0);
}

void DynamicRootSignatureTest::TearDownTestSuite()
{
	s_deviceManager.reset();
}

TEST_F(DynamicRootSignatureTest, D3DRootSignatureDynamicTest)
{
	ID3D12Device* device = s_deviceManager->GetDevice();

	D3DRootSignatureDynamic dynamicRS{};

	dynamicRS.AddConstants(4u, D3D12_SHADER_VISIBILITY_VERTEX, 0u);
	dynamicRS.AddConstants(4u, D3D12_SHADER_VISIBILITY_PIXEL, 0u, 1u);

	dynamicRS.AddRootCBV(D3D12_SHADER_VISIBILITY_PIXEL, 0u, 0u);
	dynamicRS.AddRootSRV(D3D12_SHADER_VISIBILITY_PIXEL, 1u, 0u);
	dynamicRS.AddRootUAV(D3D12_SHADER_VISIBILITY_PIXEL, 2u, 0u);

	dynamicRS.AddDescriptorTable(
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 10u, D3D12_SHADER_VISIBILITY_PIXEL, true, 3u
	);
	dynamicRS.AddDescriptorTable(
		D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 10u, D3D12_SHADER_VISIBILITY_PIXEL, true, 4u
	);

	dynamicRS.CompileSignature(RSCompileFlagBuilder{}.VertexShader(), BindlessLevel::None);

	D3DRootSignature rs{};
	rs.CreateSignature(device, dynamicRS);

	EXPECT_NE(rs.Get(), nullptr) << "Root signature creation failed.";
}
