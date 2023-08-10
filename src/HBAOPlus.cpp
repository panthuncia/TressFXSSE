#include "HBAOPlus.h"
#include "Util.h"
#include "Menu.h"
HBAOPlus::HBAOPlus() {
}
HBAOPlus::~HBAOPlus() {

}
void HBAOPlus::Initialize(ID3D11Device* pDevice) {
	//init HBAOPlus
	GFSDK_SSAO_CustomHeap  CustomHeap;
	CustomHeap.new_ = ::   operator new;
	CustomHeap.delete_ = ::operator delete;
	status = GFSDK_SSAO_CreateContext_D3D11(pDevice, &pAOContext, &CustomHeap);
	assert(status == GFSDK_SSAO_OK);  // HBAO+ requires feature level 11_0 or above

	Params.Radius = 2.f;
	Params.Bias = 0.1f;
	Params.PowerExponent = 2.f;
	Params.Blur.Enable = true;
	Params.Blur.Radius = GFSDK_SSAO_BLUR_RADIUS_4;
	Params.Blur.Sharpness = 16.f;

	Output.Blend.Mode = GFSDK_SSAO_MULTIPLY_RGB;

	m_pDevice = pDevice;
}

void HBAOPlus::CopyDSVTexture(ID3D11Resource* pResource) {
	auto pManager = RE::BSGraphics::Renderer::GetSingleton();
	if (m_pDepthSRV == nullptr) {
		auto pDevice = pManager->GetRuntimeData().forwarder;
		m_pDepthTexture = pResource;
		D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc;
		viewDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
		viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		viewDesc.Texture2D.MipLevels = 1;
		viewDesc.Texture2D.MostDetailedMip = 0;
		logger::info("Creating HBAO SRV");
		HRESULT hr2 = pDevice->CreateShaderResourceView(m_pDepthTexture, &viewDesc, &m_pDepthSRV);
		Util::printHResult(hr2);
		if (m_pDepthSRV == nullptr) {
			//Util::PrintAllD3D11DebugMessages(pDevice);
			logger::info("HBAO SRV creation failed!");
			return;
		}
		Input.DepthData.pFullResDepthTextureSRV = m_pDepthSRV;
	}
	//pManager->context->CopyResource(m_pDepthTexture, pResource);
	//logger::info("Copied resource");
}

void HBAOPlus::SetDepthSRV(ID3D11ShaderResourceView* pSRV){
	m_pDepthSRV = pSRV;
}

void HBAOPlus::SetInput(DirectX::XMMATRIX projectionMatrix, float sceneScale)
{
	DirectX::XMFLOAT4X4 projectionFloats;
	DirectX::XMStoreFloat4x4(&projectionFloats, projectionMatrix);
	float matrix[16] = { projectionFloats._11, projectionFloats._12, projectionFloats._13, projectionFloats._14, projectionFloats._21, projectionFloats._22, projectionFloats._23, projectionFloats._24, projectionFloats._31, projectionFloats._32, projectionFloats._33, projectionFloats._34, projectionFloats._41, projectionFloats._42, projectionFloats._43, projectionFloats._44 };

	Input.DepthData.DepthTextureType = GFSDK_SSAO_HARDWARE_DEPTHS;
	//Input.DepthData.pFullResDepthTextureSRV = pDepthStencilTextureSRV;
	Input.DepthData.ProjectionMatrix.Data = GFSDK_SSAO_Float4x4(matrix);
	Input.DepthData.ProjectionMatrix.Layout = GFSDK_SSAO_COLUMN_MAJOR_ORDER;
	Input.DepthData.MetersToViewSpaceUnits = sceneScale;
	//GFSDK_SSAO_InputViewport vp;
	//Input.DepthData.Viewport = 
}

void HBAOPlus::SetAOParameters() {
	auto menu = Menu::GetSingleton();
	Params.BackgroundAO.Enable = false;
	Params.ForegroundAO.Enable = true;
	Params.LargeScaleAO = menu->aoLargeScaleAOSlider;
	Params.SmallScaleAO = menu->aoSmallScaleAOSlider;
	Params.Bias = menu->aoBiasSlider;
	Params.Blur.Enable = menu->aoBlurEnableCheckbox;
	Params.Blur.Radius = GFSDK_SSAO_BLUR_RADIUS_2;
	Params.Blur.Sharpness = menu->aoBlurSharpnessSlider;
	Params.PowerExponent = menu->aoPowerExponentSlider;
	Params.EnableDualLayerAO = false;
	Params.Radius = menu->aoRadiusSlider;
	//Params.DepthStorage;
	Params.DepthThreshold.Enable = menu->aoDepthThresholdEnableCheckbox;
	Params.DepthThreshold.MaxViewDepth = menu->aoDepthThresholdMaxViewDepthSlider;
	Params.DepthThreshold.Sharpness = menu->aoDepthThresholdSharpnessSlider;
	Params.StepCount = GFSDK_SSAO_STEP_COUNT_8;
}

void HBAOPlus::SetRenderTarget(ID3D11RenderTargetView* pOutputColorRTV, GFSDK_SSAO_BlendMode blendMode) {
	Output.pRenderTargetView = pOutputColorRTV;
	Output.Blend.Mode = blendMode;
	gotRTV = true;
	pRTV = pOutputColorRTV;
}

void HBAOPlus::RenderAO() {
	auto manager = RE::BSGraphics::Renderer::GetSingleton();
	auto pContext = manager->GetRuntimeData().context;
	status = pAOContext->RenderAO(pContext, Input, Params, Output);
	assert(status == GFSDK_SSAO_OK);
}
