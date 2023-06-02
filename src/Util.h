#pragma once

#include "LOCAL_RE/BSGraphicsTypes.h"

namespace Util
{
	// If we don't downscale positions we will run in to precision issues on the GPU when
	// near the edges of the map. World positions and camera must all be scaled by the same amount!
	// I'm a bit surprised a game of this scale wouldn't use some kind of origin re-basing.
	constexpr auto              RenderScale = 0.0142875f;
	ID3D11ShaderResourceView*   GetSRVFromRTV(ID3D11RenderTargetView* a_rtv);
	ID3D11RenderTargetView*     GetRTVFromSRV(ID3D11ShaderResourceView* a_srv);
	std::string                 GetNameFromSRV(ID3D11ShaderResourceView* a_srv);
	std::string                 GetNameFromRTV(ID3D11RenderTargetView* a_rtv);
	RE::NiPointer<RE::NiCamera> GetPlayerNiCamera();
	DirectX::XMFLOAT4X4         GetPlayerProjectionMatrix(const RE::NiFrustum& frustum, UINT resolution_x, UINT resolution_y) noexcept;
}
