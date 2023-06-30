#pragma once

#include "LOCAL_RE/BSGraphicsTypes.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
namespace Util
{
	// If we don't downscale positions we will run in to precision issues on the GPU when
	// near the edges of the map. World positions and camera must all be scaled by the same amount!
	// I'm a bit surprised a game of this scale wouldn't use some kind of origin re-basing.
	constexpr auto              RenderScale = 0.0142875f;
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
	glm::mat4                   BuildViewMatrix(const glm::vec3& position, const glm::vec2& rotation) noexcept;
}
