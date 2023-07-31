#include "Util.h"
#define _USE_MATH_DEFINES
#include <math.h>
#include <DirectXMath.h>
#include "Offsets.h"
typedef const char* (*_GetFormEditorID)(std::uint32_t a_formID);
namespace Util
{
	std::string GetFormEditorID(const RE::TESForm* a_form)
	{
		static auto tweaks = GetModuleHandle(L"po3_Tweaks");
		static auto func = reinterpret_cast<_GetFormEditorID>(GetProcAddress(tweaks, "GetFormEditorID"));
		if (func) {
			return func(a_form->formID);
		}
		return "";
	}
	void StoreTransform3x4NoScale(DirectX::XMFLOAT3X4& Dest, const RE::NiTransform& Source)
	{
		//
		// Shove a Matrix3+Point3 directly into a float[3][4] with no modifications
		//
		// Dest[0][#] = Source.m_Rotate.m_pEntry[0][#];
		// Dest[0][3] = Source.m_Translate.x;
		// Dest[1][#] = Source.m_Rotate.m_pEntry[1][#];
		// Dest[1][3] = Source.m_Translate.x;
		// Dest[2][#] = Source.m_Rotate.m_pEntry[2][#];
		// Dest[2][3] = Source.m_Translate.x;
		//
		static_assert(sizeof(RE::NiTransform::rotate) == 3 * 3 * sizeof(float));  // NiMatrix3
		static_assert(sizeof(RE::NiTransform::translate) == 3 * sizeof(float));   // NiPoint3
		static_assert(offsetof(RE::NiTransform, translate) > offsetof(RE::NiTransform, rotate));

		_mm_store_ps(Dest.m[0], _mm_loadu_ps(Source.rotate.entry[0]));
		_mm_store_ps(Dest.m[1], _mm_loadu_ps(Source.rotate.entry[1]));
		_mm_store_ps(Dest.m[2], _mm_loadu_ps(Source.rotate.entry[2]));
		Dest.m[0][3] = Source.translate.x;
		Dest.m[1][3] = Source.translate.y;
		Dest.m[2][3] = Source.translate.z;
	}
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
		const auto        y = glm::atan(x);
		const auto        fov = y / 0.01745328f / 0.5f;
		return glm::radians(fov);
#endif
	}
	glm::mat4 Perspective(float fov, float aspect, const RE::NiFrustum& frustum) noexcept
	{
		const auto range = frustum.fFar / (frustum.fNear - frustum.fFar);
		const auto height = 1.0f / glm::tan(fov * 0.5f);

		glm::mat4 proj;
		proj[0][0] = height;
		proj[0][1] = 0.0f;
		proj[0][2] = 0.0f;
		proj[0][3] = 0.0f;

		proj[1][0] = 0.0f;
		proj[1][1] = height * aspect;
		proj[1][2] = 0.0f;
		proj[1][3] = 0.0f;

		proj[2][0] = 0.0f;
		proj[2][1] = 0.0f;
		proj[2][2] = range * -1.0f;
		proj[2][3] = 1.0f;

		proj[3][0] = 0.0f;
		proj[3][1] = 0.0f;
		proj[3][2] = range * frustum.fNear;
		proj[3][3] = 0.0f;

		// exact match, save for 2,0 2,1 - looks like XMMatrixPerspectiveOffCenterLH with a slightly
		// different frustum or something. whatever, close enough.
		//return proj;
		return glm::transpose(proj);
	}
	glm::mat4 GetPlayerProjectionMatrix(const RE::NiFrustum& frustum, UINT resolution_x, UINT resolution_y) noexcept
	{
		const float aspect = (float)resolution_x / (float)resolution_y;
		auto f = frustum;
		f.fNear *= RenderScale;
		f.fFar *= RenderScale;

		float fov = GetFOV();
		logger::info("FOV: {}", fov);
		//return glm::perspective(fov, aspect, f.fNear, f.fFar);
		return Perspective(fov, aspect, f);
	}
	glm::mat4 LookAt(const glm::dvec3& pos, const glm::dvec3& at, const glm::dvec3& up) noexcept
	{
		const auto forward = glm::normalize(at - pos);
		const auto side = glm::normalize(glm::cross(up, forward));
		const auto u = glm::cross(forward, side);

		const auto negEyePos = pos * -1.0;
		const auto sDotEye = glm::dot(side, negEyePos);
		const auto uDotEye = glm::dot(u, negEyePos);
		const auto fDotEye = glm::dot(forward, negEyePos);

		glm::dmat4 result;
		result[0][0] = side.x * -1.0;
		result[0][1] = side.y * -1.0;
		result[0][2] = side.z * -1.0;
		result[0][3] = sDotEye * -1.0;

		result[1][0] = u.x;
		result[1][1] = u.y;
		result[1][2] = u.z;
		result[1][3] = uDotEye;

		result[2][0] = forward.x;
		result[2][1] = forward.y;
		result[2][2] = forward.z;
		result[2][3] = fDotEye;

		result[3][0] = 0.0;
		result[3][1] = 0.0;
		result[3][2] = 0.0;
		result[3][3] = 1.0;
		return result;
		//return glm::transpose(result);
	}
	// Return the forward view vector
	glm::dvec3 GetViewVector(const glm::dvec3& forwardRefer, double pitch, double roll, double yaw) noexcept
	{
		auto aproxNormal = glm::dvec4(forwardRefer.x, forwardRefer.y, forwardRefer.z, 1.0);

		auto m = glm::identity<glm::dmat4>();
		m = glm::rotate(m, -pitch, glm::dvec3(1.0l, 0.0l, 0.0l));
		aproxNormal = m * aproxNormal;

		m = glm::identity<glm::dmat4>();
		m = glm::rotate(m, -roll, glm::dvec3(0.0l, 1.0l, 0.0l));
		aproxNormal = m * aproxNormal;

		m = glm::identity<glm::dmat4>();
		m = glm::rotate(m, -yaw, glm::dvec3(0.0l, 0.0l, 1.0l));
		aproxNormal = m * aproxNormal;

		return static_cast<glm::vec3>(aproxNormal);
	}
	glm::mat4 BuildViewMatrix(const glm::vec3& position, const glm::dvec3& rotation) noexcept
	{
		const glm::dvec3 pos = ToRenderScale(position);
		constexpr auto limit = M_PI_2 * 0.9999;
		limit;
		const glm::dvec3 dir = GetViewVector(
            { 0.0, 1.0, 0.0 },
			glm::clamp(rotation.x, -limit, limit),
			rotation.y, 
            rotation.z);
		logger::info("dir: {}, {}, {}", dir.x, dir.y, dir.z);
		return LookAt(pos, pos + dir, { 0.0l, 0.0l, 1.0l });
	}
	void PrintXMMatrix(DirectX::XMMATRIX mat)
	{
		DirectX::XMFLOAT4X4 floats;
		DirectX::XMStoreFloat4x4(&floats, mat);
		logger::info("{}, {}, {}, {}", floats._11, floats._12, floats._13, floats._14);
		logger::info("{}, {}, {}, {}", floats._21, floats._22, floats._23, floats._24);
		logger::info("{}, {}, {}, {}", floats._31, floats._32, floats._33, floats._34);
		logger::info("{}, {}, {}, {}", floats._41, floats._42, floats._43, floats._44);
	}
	void HkMatrixToNiMatrix(const RE::hkMatrix3& a_hkMat, RE::NiMatrix3& a_niMat)
	{
		a_niMat.entry[0][0] = a_hkMat.col0.quad.m128_f32[0];
		a_niMat.entry[1][0] = a_hkMat.col0.quad.m128_f32[1];
		a_niMat.entry[2][0] = a_hkMat.col0.quad.m128_f32[2];

		a_niMat.entry[0][1] = a_hkMat.col1.quad.m128_f32[0];
		a_niMat.entry[1][1] = a_hkMat.col1.quad.m128_f32[1];
		a_niMat.entry[2][1] = a_hkMat.col1.quad.m128_f32[2];

		a_niMat.entry[0][2] = a_hkMat.col2.quad.m128_f32[0];
		a_niMat.entry[1][2] = a_hkMat.col2.quad.m128_f32[1];
		a_niMat.entry[2][2] = a_hkMat.col2.quad.m128_f32[2];
	}
	RE::NiTransform HkTransformToNiTransform(const RE::hkTransform& a_hkTransform)
	{
		RE::NiTransform result;
		HkMatrixToNiMatrix(a_hkTransform.rotation, result.rotate);
		result.translate = HkVectorToNiPoint(a_hkTransform.translation * *g_worldScaleInverse);
		result.scale = 1.f;

		return result;
	}
	std::string ptr_to_string(void* ptr)
	{
		const void*       address = static_cast<const void*>(ptr);
		std::stringstream ss;
		ss << address;
		std::string name = ss.str();
		return name;
	}
}
