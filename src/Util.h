#pragma once

#include "LOCAL_RE/BSGraphicsTypes.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
namespace Util
{
	// If we don't downscale positions we will run in to precision issues on the GPU when
	// near the edges of the map. World positions and camera must all be scaled by the same amount!
	// I'm a bit surprised a game of this scale wouldn't use some kind of origin re-basing.
	//also this is important for hair physics
	//constexpr auto RenderScale = 0.142875f;
	//constexpr auto              RenderScale = 0.0142875f;
	constexpr auto              RenderScale = 1.0f;
	// Convert a world position to our render scale
	__forceinline glm::vec3 ToRenderScale(const glm::vec3& position) noexcept
	{
		return position * RenderScale;
	}

	// And convert back
	__forceinline glm::vec3 FromRenderScale(const glm::vec3& position) noexcept
	{
		return position / RenderScale;
	}
	ID3D11ShaderResourceView*   GetSRVFromRTV(ID3D11RenderTargetView* a_rtv);
	ID3D11RenderTargetView*     GetRTVFromSRV(ID3D11ShaderResourceView* a_srv);
	std::string                 GetNameFromSRV(ID3D11ShaderResourceView* a_srv);
	std::string                 GetNameFromRTV(ID3D11RenderTargetView* a_rtv);
	RE::NiPointer<RE::NiCamera> GetPlayerNiCamera();
	glm::mat4                   GetPlayerProjectionMatrix(const RE::NiFrustum& frustum, UINT resolution_x, UINT resolution_y) noexcept;
	glm::mat4                   BuildViewMatrix(const glm::vec3& position, const glm::dvec3& rotation) noexcept;
	void                        PrintXMMatrix(DirectX::XMMATRIX mat);
	[[nodiscard]] inline RE::NiPoint3 HkVectorToNiPoint(const RE::hkVector4& vec) { return { vec.quad.m128_f32[0], vec.quad.m128_f32[1], vec.quad.m128_f32[2] }; }
	[[nodiscard]] RE::NiTransform     HkTransformToNiTransform(const RE::hkTransform& a_hkTransform);
	void                              HkMatrixToNiMatrix(const RE::hkMatrix3& a_hkMat, RE::NiMatrix3& a_niMat);
	std::string                       ptr_to_string(void* ptr);
	void                              StoreTransform3x4NoScale(DirectX::XMFLOAT3X4& Dest, const RE::NiTransform& Source);
	std::string                       GetFormEditorID(const RE::TESForm* a_form);
	ID3D11BlendState*                 CreateBlendState(ID3D11Device* pDevice, BOOL AlphaToCoverageEnable, BOOL IndependentBlendEnable, BOOL BlendEnable, D3D11_BLEND SrcBlend, D3D11_BLEND_OP BlendOp, D3D11_BLEND_OP BlendOpAlpha, D3D11_BLEND DestBlend, D3D11_BLEND DestBlendAlpha, UINT8 RenderTargetWriteMask, D3D11_BLEND SrcBlendAlpha);
	ID3D11DepthStencilState*          CreateDepthStencilState(ID3D11Device* pDevice, BOOL DepthEnable, D3D11_DEPTH_WRITE_MASK DepthWriteMask, D3D11_COMPARISON_FUNC DepthFunc, BOOL StencilEnable, UINT8 StencilReadMask, UINT8 StencilWriteMask, D3D11_STENCIL_OP StencilFailOp, D3D11_STENCIL_OP StencilDepthFailOp, D3D11_STENCIL_OP StencilPassOp, D3D11_COMPARISON_FUNC StencilFunc);
	ID3D11RasterizerState*            CreateRasterizerState(ID3D11Device* pDevice, D3D11_FILL_MODE FillMode, D3D11_CULL_MODE CullMode, BOOL FrontCounterClockwise, INT DepthBias, FLOAT DepthBiasClamp, FLOAT SlopeScaledDepthBias, BOOL DepthClipEnable, BOOL MultisampleEnable, BOOL AntialiasedLineEnable, BOOL ScissorEnable);
	void                              PrintAllD3D11DebugMessages(ID3D11Device* d3dDevice);
	void                              printHResult(HRESULT hr);
	void                              OverrideCommunityShadersScreenSpaceShadowsDepthTexture(ID3D11ShaderResourceView* pDepthSRV);
}
