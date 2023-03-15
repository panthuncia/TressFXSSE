#pragma once

#include "LOCAL_RE/BSGraphicsTypes.h"

namespace Util
{
	ID3D11ShaderResourceView* GetSRVFromRTV(ID3D11RenderTargetView* a_rtv);
	ID3D11RenderTargetView*   GetRTVFromSRV(ID3D11ShaderResourceView* a_srv);
	std::string               GetNameFromSRV(ID3D11ShaderResourceView* a_srv);
	std::string               GetNameFromRTV(ID3D11RenderTargetView* a_rtv);
}
