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
	ID3D11RasterizerState*   CreateRasterizerState(ID3D11Device* pDevice, D3D11_FILL_MODE FillMode, D3D11_CULL_MODE CullMode, BOOL FrontCounterClockwise, INT DepthBias, FLOAT DepthBiasClamp, FLOAT SlopeScaledDepthBias, BOOL DepthClipEnable, BOOL MultisampleEnable, BOOL AntialiasedLineEnable, BOOL ScissorEnable) {
		D3D11_RASTERIZER_DESC rasterizerDesc;
		rasterizerDesc.FillMode = FillMode;
		rasterizerDesc.CullMode = CullMode;
		rasterizerDesc.FrontCounterClockwise = FrontCounterClockwise;
		rasterizerDesc.DepthBias = DepthBias;
		rasterizerDesc.DepthBiasClamp = DepthBiasClamp;
		rasterizerDesc.SlopeScaledDepthBias = SlopeScaledDepthBias;
		rasterizerDesc.DepthClipEnable = DepthClipEnable;
		rasterizerDesc.MultisampleEnable = MultisampleEnable;
		rasterizerDesc.AntialiasedLineEnable = AntialiasedLineEnable;
		rasterizerDesc.ScissorEnable = ScissorEnable;
		ID3D11RasterizerState* pRasterizerState;
		pDevice->CreateRasterizerState(&rasterizerDesc, &pRasterizerState);
		return pRasterizerState;
	}
	ID3D11DepthStencilState* CreateDepthStencilState(ID3D11Device* pDevice, BOOL DepthEnable, D3D11_DEPTH_WRITE_MASK DepthWriteMask, D3D11_COMPARISON_FUNC DepthFunc, BOOL StencilEnable, UINT8 StencilReadMask, UINT8 StencilWriteMask, D3D11_STENCIL_OP StencilFailOp, D3D11_STENCIL_OP StencilDepthFailOp, D3D11_STENCIL_OP StencilPassOp, D3D11_COMPARISON_FUNC StencilFunc) {
		D3D11_DEPTH_STENCIL_DESC depthStencilDesc;
		depthStencilDesc.DepthEnable = DepthEnable;
		depthStencilDesc.DepthWriteMask = DepthWriteMask;
		depthStencilDesc.DepthFunc = DepthFunc;
		depthStencilDesc.StencilEnable = StencilEnable;
		depthStencilDesc.StencilReadMask = StencilReadMask;
		depthStencilDesc.StencilWriteMask = StencilWriteMask;
		D3D11_DEPTH_STENCILOP_DESC depthStencilopDesc;
		depthStencilopDesc.StencilFailOp = StencilFailOp;
		depthStencilopDesc.StencilDepthFailOp = StencilDepthFailOp;
		depthStencilopDesc.StencilPassOp = StencilPassOp;
		depthStencilopDesc.StencilFunc = StencilFunc;
		depthStencilDesc.FrontFace = depthStencilopDesc;
		depthStencilDesc.BackFace = depthStencilopDesc;
		ID3D11DepthStencilState* pDepthStencilState;
		pDevice->CreateDepthStencilState(&depthStencilDesc, &pDepthStencilState);
		return pDepthStencilState;
	}
	ID3D11BlendState* CreateBlendState(ID3D11Device* pDevice, BOOL AlphaToCoverageEnable, BOOL IndependentBlendEnable, BOOL BlendEnable, D3D11_BLEND SrcBlend, D3D11_BLEND_OP BlendOp, D3D11_BLEND_OP BlendOpAlpha, D3D11_BLEND DestBlend, D3D11_BLEND DestBlendAlpha, UINT8 RenderTargetWriteMask, D3D11_BLEND SrcBlendAlpha) {
		
		D3D11_BLEND_DESC blendDesc;
		blendDesc.AlphaToCoverageEnable = AlphaToCoverageEnable;
		blendDesc.IndependentBlendEnable = IndependentBlendEnable;
		D3D11_RENDER_TARGET_BLEND_DESC targetBlendDesc;
		targetBlendDesc.BlendEnable = BlendEnable;
		targetBlendDesc.SrcBlend = SrcBlend;
		targetBlendDesc.BlendOp = BlendOp;
		targetBlendDesc.BlendOpAlpha = BlendOpAlpha;
		targetBlendDesc.DestBlend = DestBlend;
		targetBlendDesc.DestBlendAlpha = DestBlendAlpha;
		targetBlendDesc.RenderTargetWriteMask = RenderTargetWriteMask;
		targetBlendDesc.SrcBlendAlpha = SrcBlendAlpha;
		blendDesc.RenderTarget[0] = targetBlendDesc;
		blendDesc.RenderTarget[1] = targetBlendDesc;
		blendDesc.RenderTarget[2] = targetBlendDesc;
		blendDesc.RenderTarget[3] = targetBlendDesc;
		blendDesc.RenderTarget[4] = targetBlendDesc;
		blendDesc.RenderTarget[5] = targetBlendDesc;
		blendDesc.RenderTarget[6] = targetBlendDesc;
		blendDesc.RenderTarget[7] = targetBlendDesc;
		ID3D11BlendState* blendState;
		pDevice->CreateBlendState(&blendDesc, &blendState);
		return blendState;
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
		if (camera == nullptr) {
			return RE::NiPointer<RE::NiCamera>(nullptr);
		}
		// Do other things parent stuff to the camera node? Better safe than sorry I guess
		if (camera->cameraRoot == nullptr || camera->cameraRoot->GetChildren().size() == 0)
			return RE::NiPointer<RE::NiCamera>(nullptr);
		for (auto& entry : camera->cameraRoot->GetChildren()) {
			auto asCamera = skyrim_cast<RE::NiCamera*>(entry.get());
			if (asCamera)
				return RE::NiPointer<RE::NiCamera>(asCamera);
		}
		return RE::NiPointer<RE::NiCamera>(nullptr);
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
	void PrintAllD3D11DebugMessages(ID3D11Device* d3dDevice)
	{
		Microsoft::WRL::ComPtr<ID3D11Device> m_d3dDevice;
		*(m_d3dDevice.GetAddressOf()) = d3dDevice;
		Microsoft::WRL::ComPtr<ID3D11Debug>     d3dDebug;
		Microsoft::WRL::ComPtr<ID3D11InfoQueue> d3dInfoQueue;
		HRESULT                                 hr = m_d3dDevice.As(&d3dDebug);
		if (SUCCEEDED(hr)) {
			hr = d3dDebug.As(&d3dInfoQueue);
			if (SUCCEEDED(hr)) {
#ifdef _DEBUG
				d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, true);
				d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, true);
#endif
				D3D11_MESSAGE_ID hide[] = {
					D3D11_MESSAGE_ID_SETPRIVATEDATA_CHANGINGPARAMS,
					// TODO: Add more message IDs here as needed
				};
				D3D11_INFO_QUEUE_FILTER filter = {};
				filter.DenyList.NumIDs = _countof(hide);
				filter.DenyList.pIDList = hide;
				d3dInfoQueue->AddStorageFilterEntries(&filter);
			}
		}
		//HANDLE_HRESULT is just a macro of mine to check for S_OK return value
		d3dInfoQueue->PushEmptyStorageFilter();
		UINT64 message_count = d3dInfoQueue->GetNumStoredMessages();

		for (UINT64 i = 0; i < message_count; i++) {
			SIZE_T message_size = 0;
			d3dInfoQueue->GetMessage(i, nullptr, &message_size);  //get the size of the message

			D3D11_MESSAGE* message = (D3D11_MESSAGE*)malloc(message_size);  //allocate enough space
			d3dInfoQueue->GetMessage(i, message, &message_size);            //get the actual message

			//do whatever you want to do with it
			logger::info("{}", message->pDescription);

			free(message);
		}

		d3dInfoQueue->ClearStoredMessages();
	}
}
