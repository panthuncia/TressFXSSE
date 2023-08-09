#include "HBAOPlus.h"

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

	Output.Blend.Mode = GFSDK_SSAO_OVERWRITE_RGB;

	m_pDevice = pDevice;
}
void HBAOPlus::SetInputDepths(ID3D11ShaderResourceView* pDepthStencilTextureSRV, DirectX::XMMATRIX projectionMatrix, float sceneScale)
{
	DirectX::XMFLOAT4X4 projectionFloats;
	DirectX::XMStoreFloat4x4(&projectionFloats, projectionMatrix);
	float matrix[16] = { projectionFloats._11, projectionFloats._12, projectionFloats._13, projectionFloats._14, projectionFloats._21, projectionFloats._22, projectionFloats._23, projectionFloats._24, projectionFloats._31, projectionFloats._32, projectionFloats._33, projectionFloats._34, projectionFloats._41, projectionFloats._42, projectionFloats._43, projectionFloats._44 };

	Input.DepthData.DepthTextureType = GFSDK_SSAO_HARDWARE_DEPTHS;
	Input.DepthData.pFullResDepthTextureSRV = pDepthStencilTextureSRV;
	Input.DepthData.ProjectionMatrix.Data = GFSDK_SSAO_Float4x4(matrix);
	Input.DepthData.ProjectionMatrix.Layout = GFSDK_SSAO_ROW_MAJOR_ORDER;
	Input.DepthData.MetersToViewSpaceUnits = sceneScale;
}

void HBAOPlus::SetAOParameters(float radius, float bias, float powerExponent, bool blur, GFSDK_SSAO_BlurRadius blurRadius, float sharpness) {
	Params.Radius = radius;
	Params.Bias = bias;
	Params.PowerExponent = powerExponent;
	Params.Blur.Enable = blur;
	Params.Blur.Radius = blurRadius;
	Params.Blur.Sharpness = sharpness;
}

void HBAOPlus::SetRenderTarget(ID3D11RenderTargetView* pOutputColorRTV, GFSDK_SSAO_BlendMode blendMode) {
	Output.pRenderTargetView = pOutputColorRTV;
	Output.Blend.Mode = blendMode;
}

void HBAOPlus::RenderAO() {
	auto manager = RE::BSGraphics::Renderer::GetSingleton();
	auto pContext = manager->GetRuntimeData().context;
	status = pAOContext->RenderAO(pContext, Input, Params, Output);
	assert(status == GFSDK_SSAO_OK);
}
