#include "Util.h"
#define _USE_MATH_DEFINES
#include <math.h>
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
	RE::NiPointer<RE::NiCamera> GetPlayerNiCamera()
	{
		RE::PlayerCamera* camera = RE::PlayerCamera::GetSingleton();
		// Do other things parent stuff to the camera node? Better safe than sorry I guess
		if (camera->cameraRoot->GetChildren().size() == 0)
			return nullptr;
		for (auto& entry : camera->cameraRoot->GetChildren()) {
			auto asCamera = skyrim_cast<RE::NiCamera*>(entry.get());
			if (asCamera)
				return RE::NiPointer<RE::NiCamera>(asCamera);
		}
		return nullptr;
	}
	float GetFOV() noexcept
	{
#ifdef SKYRIM_SUPPORT_AE
		static const auto fov = REL::Relocation<float*>(g_Offsets->FOV);
		const auto        deg = *fov + *REL::Relocation<float*>(g_Offsets->FOVOffset);
		return glm::radians(deg);
#else
		// The game doesn't tell us about dyanmic changes to FOV in an easy way (that I can see, but I'm also blind)
		// It does store the FOV indirectly in this global - we can just run the equations it uses in reverse.
		uintptr_t         SE_FOV = 513786;
		uintptr_t FOV = REL::ID(SE_FOV).address();
		static const auto fac = REL::Relocation<float*>(FOV);
		const auto        base = *fac;
		const auto        x = base / 1.30322540f;
		const auto        y = atan(x);
		const auto        fov = y / 0.01745328f / 0.5f;
		return fov * (float)M_PI / 180;
#endif
	}
	DirectX::XMFLOAT4X4 Perspective(float fov, float aspect, const RE::NiFrustum& frustum) noexcept
	{
		const auto range = frustum.fFar / (frustum.fNear - frustum.fFar);
		const auto height = 1.0f / tan(fov * 0.5f);
		DirectX::XMFLOAT4X4 proj;
		proj._11 = height;
		proj._12 = 0.0f;
		proj._13 = 0.0f;
		proj._14 = 0.0f;

		proj._21 = 0.0f;
		proj._22 = height * aspect;
		proj._23 = 0.0f;
		proj._24 = 0.0f;

		proj._31 = 0.0f;
		proj._32 = 0.0f;
		proj._33 = range * -1.0f;
		proj._34 = 1.0f;

		proj._41 = 0.0f;
		proj._42 = 0.0f;
		proj._43 = range * frustum.fNear;
		proj._44 = 0.0f;
		// exact match, save for 2,0 2,1 - looks like XMMatrixPerspectiveOffCenterLH with a slightly
		// different frustum or something. whatever, close enough.
		return proj;
	}
	DirectX::XMFLOAT4X4 GetPlayerProjectionMatrix(const RE::NiFrustum& frustum, UINT resolution_x, UINT resolution_y) noexcept
	{
		const float aspect = (float)resolution_x / (float)resolution_y;
		auto f = frustum;
		//f.fNear *= RenderScale;
		//f.fFar *= RenderScale;

		float fov = GetFOV();
		return Perspective(fov, aspect, f);
	}
}
