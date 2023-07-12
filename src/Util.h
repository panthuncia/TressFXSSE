#pragma once

#include "LOCAL_RE/BSGraphicsTypes.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
namespace Util
{
	// If we don't downscale positions we will run in to precision issues on the GPU when
	// near the edges of the map. World positions and camera must all be scaled by the same amount!
	// I'm a bit surprised a game of this scale wouldn't use some kind of origin re-basing.
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
	glm::mat4                   BuildViewMatrix(const glm::vec3& position, const glm::dvec2& rotation) noexcept;
	void                        PrintXMMatrix(DirectX::XMMATRIX mat);
	[[nodiscard]] inline RE::NiPoint3 HkVectorToNiPoint(const RE::hkVector4& vec) { return { vec.quad.m128_f32[0], vec.quad.m128_f32[1], vec.quad.m128_f32[2] }; }
	[[nodiscard]] RE::NiTransform     HkTransformToNiTransform(const RE::hkTransform& a_hkTransform);
	void                              HkMatrixToNiMatrix(const RE::hkMatrix3& a_hkMat, RE::NiMatrix3& a_niMat);
}
