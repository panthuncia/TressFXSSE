#pragma once
#include "GFSDK_SSAO.h"
class HBAOPlus
{
public:
	HBAOPlus();
	~HBAOPlus();
	void SetInputDepths(ID3D11ShaderResourceView* pDepthStencilTextureSRV, DirectX::XMMATRIX projectionMatrix, float sceneScale);
	void SetAOParameters(float radius, float bias, float powerExponent, bool blur, GFSDK_SSAO_BlurRadius blurRadius, float sharpness);
	void SetRenderTarget(ID3D11RenderTargetView* pOutputColorRTV, GFSDK_SSAO_BlendMode blendMode);
	void RenderAO();

private:
	GFSDK_SSAO_Status         status;
	GFSDK_SSAO_Context_D3D11* pAOContext;
	GFSDK_SSAO_Parameters Params;
	GFSDK_SSAO_InputData_D3D11 Input;
	GFSDK_SSAO_Output_D3D11 Output;
};
