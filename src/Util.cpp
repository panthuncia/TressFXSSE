#include "Util.h"

namespace Util
{
	ID3D11ShaderResourceView* GetSRVFromRTV(ID3D11RenderTargetView* a_rtv)
	{
		if (a_rtv) {
			if (auto r = BSGraphics::Renderer::QInstance()) {
				for (int i = 0; i < RenderTargets::RENDER_TARGET_COUNT; i++) {
					auto rt = r->pRenderTargets[i];
					if (a_rtv == rt.RTV) {
						return rt.SRV;
					}
				}
			}
		}
		return nullptr;
	}

	ID3D11RenderTargetView* GetRTVFromSRV(ID3D11ShaderResourceView* a_srv)
	{
		if (a_srv) {
			if (auto r = BSGraphics::Renderer::QInstance()) {
				for (int i = 0; i < RenderTargets::RENDER_TARGET_COUNT; i++) {
					auto rt = r->pRenderTargets[i];
					if (a_srv == rt.SRV || a_srv == rt.SRVCopy) {
						return rt.RTV;
					}
				}
			}
		}
		return nullptr;
	}

	std::string GetNameFromSRV(ID3D11ShaderResourceView* a_srv)
	{
		if (a_srv) {
			if (auto r = BSGraphics::Renderer::QInstance()) {
				for (int i = 0; i < RenderTargets::RENDER_TARGET_COUNT; i++) {
					auto rt = r->pRenderTargets[i];
					if (a_srv == rt.SRV || a_srv == rt.SRVCopy) {
						return RTNames[i];
					}
				}
			}
		}
		return "NONE";
	}

	std::string GetNameFromRTV(ID3D11RenderTargetView* a_rtv)
	{
		if (a_rtv) {
			if (auto r = BSGraphics::Renderer::QInstance()) {
				for (int i = 0; i < RenderTargets::RENDER_TARGET_COUNT; i++) {
					auto rt = r->pRenderTargets[i];
					if (a_rtv == rt.RTV) {
						return RTNames[i];
					}
				}
			}
		}
		return "NONE";
	}
}
