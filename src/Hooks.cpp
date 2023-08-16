#include "Hooks.h"
#include "HookEvents.h"
#include "ActorManager.h"
#include <sstream> //for std::stringstream 
#include <string>  //for std::string
#include <d3d11.h>
#include <chrono>
#include "Util.h"
#include "Menu.h"
#include "HBAOPlus.h"
#include "SkyrimTressFX.h"
decltype(&RE::BSFaceGenNiNode::FixSkinInstances) ptrFixSkinInstances;

struct BSFaceGenNiNode_FixSkinInstances
{
	static void thunk(RE::BSFaceGenNiNode* self, RE::NiNode* a_skeleton, bool a_arg2)
	{
		const char* name = "";
		uint32_t formId = 0x0;

		if (a_skeleton->GetUserData() && a_skeleton->GetUserData()->GetBaseObject())
		{
			//logger::info("2");
			auto bname = skyrim_cast<RE::TESNPC*>(a_skeleton->GetUserData()->GetBaseObject());
			if (bname)
				name = bname->GetName();
			//logger::info("3");
			auto bnpc = skyrim_cast<RE::TESNPC*>(a_skeleton->GetUserData()->GetBaseObject());
			if (bnpc && bnpc->faceNPC)
				formId = bnpc->faceNPC->formID;
			/*logger::info("Num headparts: {}", bnpc->numHeadParts);
			for (int i = 0; i < bnpc->numHeadParts; i++) {
				logger::info("id: {}", bnpc->headParts[i]->formEditorID);
				for (int j = 0; j < RE::BGSHeadPart::MorphIndices::kTotal; j++) {
					logger::info("morph: {}", bnpc->headParts[i]->morphs[j].model);
				}
			}*/
		}
		//logger::info("4");
		logger::info("SkinAllGeometry {} {}, {}, (formid {} base form {} head template form {})",
			a_skeleton->name, a_skeleton->GetChildren().capacity(), name,
			a_skeleton->GetUserData() ? a_skeleton->GetUserData()->formID : 0x0,
			a_skeleton->GetUserData() ? a_skeleton->GetUserData()->GetBaseObject()->formID : 0x0, formId);
		//logger::info("5");
		hdt::SkinAllHeadGeometryEvent e;
		e.skeleton = a_skeleton;
		//e.headNode = this;
		//CALL_MEMBER_FN(this, SkinAllGeometry)(a_skeleton, a_unk);
		func(self, a_skeleton, a_arg2);
		e.hasSkinned = true;
		hdt::g_skinAllHeadGeometryEventDispatcher.dispatch(e);
	}
	static inline REL::Relocation<decltype(thunk)> func;
};
struct Main_Update
{
	static void thunk()
	{
		//call main game loop
		SkyrimTressFX* tfx = SkyrimTressFX::GetSingleton();
		if (tfx->m_doReload) {
			tfx->ReloadAllHairs();
		}
		func();
	}
	static inline REL::Relocation<decltype(thunk)> func;
};
//uint8_t skipFrame = 10;
struct Hooks
{
	struct Main_DrawWorld_MainDraw
	{
		static INT64 thunk(INT64 a1, int a2, int a3, int a4, int a5)
		{
			auto val = func(a1, a2, a3, a4, a5);
			//draw hair
			auto camera = RE::PlayerCamera::GetSingleton();
			auto tfx = SkyrimTressFX::GetSingleton();
			//if this is the first frame, we need to grab the color texture
			if (!tfx->m_gameLoaded) {
				tfx->CreateRenderResources();
			}
			if (tfx->m_currentState!=state::done_drawing && camera != nullptr && camera->currentState != nullptr && (camera->currentState->id == RE::CameraState::kThirdPerson || camera->currentState->id == RE::CameraState::kFree || 
				camera->currentState->id == RE::CameraState::kDragon || camera->currentState->id == RE::CameraState::kFurniture || camera->currentState->id == RE::CameraState::kMount)) {
				if (Menu::GetSingleton()->drawHairCheckbox) {
					//tfx->Update();
					//tfx->Draw();
					tfx->m_currentState = state::done_drawing;
				}
				//MarkerRender::GetSingleton()->DrawWorldAxes(PPLLObject::GetSingleton()->m_cameraWorld, PPLLObject::GetSingleton()->m_viewXMMatrix, PPLLObject::GetSingleton()->m_projXMMatrix);
				/*auto hbao = HBAOPlus::GetSingleton();
				if (!hbao->gotRTV) {
					ID3D11RenderTargetView* pRTV;
					RE::BSGraphics::Renderer::GetSingleton()->GetRuntimeData().context->OMGetRenderTargets(1, &pRTV, nullptr);
					logger::info("HBAO RTV address: {}", Util::ptr_to_string(pRTV));
					hbao->SetRenderTarget(pRTV, GFSDK_SSAO_MULTIPLY_RGB);
				}
				if (Menu::GetSingleton()->HBAOCheckbox) {
					hbao->SetInput(tfx->m_activeScene.scene.get()->m_projMatrix, (float)69.99104);
					if (Menu::GetSingleton()->clearBeforeHBAOCheckbox) {
						logger::info("Clearing RTV");
						float clearColor[4] = { 1.0, 1.0, 1.0, 1.0 };
						RE::BSGraphics::Renderer::GetSingleton()->GetRuntimeData().context->ClearRenderTargetView(hbao->pRTV, clearColor);
					}
					logger::info("Rendering AO");
					hbao->SetAOParameters();
					hbao->RenderAO();
				}*/
			} else {
				//logger::info("Skipping frame");
			}
			return val;
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	struct Main_PostDrawWorld_MainDraw
	{
		static void thunk(INT64 BSGraphics_Renderer, int unk)
		{
			func(BSGraphics_Renderer, unk);
			//draw hair
			auto camera = RE::PlayerCamera::GetSingleton();
			auto tfx = SkyrimTressFX::GetSingleton();
			if (camera != nullptr && camera->currentState != nullptr && (camera->currentState->id == RE::CameraState::kThirdPerson || camera->currentState->id == RE::CameraState::kFree || camera->currentState->id == RE::CameraState::kDragon || camera->currentState->id == RE::CameraState::kFurniture || camera->currentState->id == RE::CameraState::kMount)) {
				MarkerRender::GetSingleton()->DrawAllMarkers(tfx->m_activeScene.scene.get()->m_viewMatrix, tfx->m_activeScene.scene.get()->m_projMatrix);
				MarkerRender::GetSingleton()->ClearMarkers();
			}
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};
};
void DrawShadows() {
	//Menu::GetSingleton()->DrawVector3(a1->light.get()->world.translate, "shadowLight pos:")
	auto camera = RE::PlayerCamera::GetSingleton();
	auto tfx = SkyrimTressFX::GetSingleton();
	if (camera != nullptr && camera->currentState != nullptr && (camera->currentState->id == RE::CameraState::kThirdPerson || camera->currentState->id == RE::CameraState::kFree || camera->currentState->id == RE::CameraState::kDragon || camera->currentState->id == RE::CameraState::kFurniture || camera->currentState->id == RE::CameraState::kMount)) {
		//RE::ThirdPersonState* tps = reinterpret_cast<RE::ThirdPersonState*>(camera->currentState.get());
		/*if (skipFrame) {
				skipFrame--;
				logger::info("skipping frame");
				} else*/
			//get current view projection matrix
			auto shadowState = RE::BSGraphics::RendererShadowState::GetSingleton();
			auto gameViewMatrix = shadowState->GetRuntimeData().cameraData.getEye().viewMat;
			auto gameProjMatrix = shadowState->GetRuntimeData().cameraData.getEye().projMat;
			auto projMatrix = DirectX::XMMatrixTranspose(gameProjMatrix);
			//Menu::GetSingleton()->DrawMatrix(shadowState->GetRuntimeData().cameraData.getEye().viewMat, "game shadow viewMat");
			//Menu::GetSingleton()->DrawMatrix(shadowState->GetRuntimeData().cameraData.getEye().projMat, "game shadow projMat");
			DirectX::XMFLOAT4X4 gameVFloats;
			DirectX::XMStoreFloat4x4(&gameVFloats, gameViewMatrix);
			//DirectX::XMFLOAT4X4 gameVFloats;
			//DirectX::XMStoreFloat4x4(&gameVFloats, gameViewMatrix);
			//transform into our render scale
			//auto worldShadowLightPos = a1->light.get()->world.translate;
			//auto worldShadowLightPosScaled = -Util::ToRenderScale(glm::vec3(worldShadowLightPos.x, worldShadowLightPos.y, worldShadowLightPos.z));
			//Menu::GetSingleton()->DrawVector3(worldShadowLightPos, "shadow light pos");

			////draw marker at light
			//auto shadowLightRot = a1->light.get()->world.rotate;
			//auto translation = DirectX::XMMatrixTranspose(DirectX::XMMatrixTranslation(worldShadowLightPosScaled.x, worldShadowLightPosScaled.y, worldShadowLightPosScaled.z));
			//auto rotation = DirectX::XMMATRIX(shadowLightRot.entry[0][0], shadowLightRot.entry[0][1], shadowLightRot.entry[0][2], 0, shadowLightRot.entry[1][0], shadowLightRot.entry[1][1], shadowLightRot.entry[1][2], 0, shadowLightRot.entry[2][0], shadowLightRot.entry[2][1], shadowLightRot.entry[2][2], 0, 0, 0, 0, 1);
			//auto transform = translation * rotation;
			//ppll->m_markerPositions.push_back(transform);

			//get viewport
			auto              currentViewport = tfx->m_activeScene.scene.get()->m_currentViewport;
			DirectX::XMVECTOR viewportVector = DirectX::XMVectorSet(currentViewport.TopLeftX, currentViewport.TopLeftY, currentViewport.Width, currentViewport.Height);
			//ppll->m_pStrandEffect->GetVariableByName("g_vViewport")->AsVector()->SetFloatVector(reinterpret_cast<float*>(&viewportVector));
			//logger::info("drawing hair shadows");
			//tfx->DrawShadows();
			//MarkerRender::GetSingleton()->DrawWorldAxes(PPLLObject::GetSingleton()->m_cameraWorld, PPLLObject::GetSingleton()->m_viewXMMatrix, PPLLObject::GetSingleton()->m_projXMMatrix);
	}
}
struct BSShadowLight_DrawShadows
{
	static void thunk(RE::BSShadowLight* a1, INT64* a2, DWORD* a3, int a4)
	{
		func(a1, a2, a3, a4);
		auto tfx = SkyrimTressFX::GetSingleton();
		tfx->m_currentState = state::draw_shadows;
		/*if (Menu::GetSingleton()->drawShadowsCheckbox) {
			DrawShadows();
		}*/
	}
	static inline REL::Relocation<decltype(thunk)> func;
};
ID3D11Texture2D* pCurrentDepthStencilResourceNoHair = nullptr;
ID3D11Resource*  pOriginalDepthTexture = nullptr;
ID3D11Texture2D*        pCurrentDepthStencilResourceWithHair = nullptr;
ID3D11ShaderResourceView* pHairDepthSRV;
extern bool             catchNextResourceCopy;
extern ID3D11Resource*  overrideResource;
struct Sub_DrawShadows
{
	static INT64 thunk(INT64 a1, UINT64* a2)
	{
		//auto                    pManager = RE::BSGraphics::Renderer::GetSingleton();
		//auto                    pContext = pManager->GetRuntimeData().context;
		//ID3D11DepthStencilView* pOriginalDSV;
		//pContext->OMGetRenderTargetsAndUnorderedAccessViews(0, nullptr, &pOriginalDSV,0, 0, nullptr);
		//pOriginalDSV->GetResource(&pOriginalDepthTexture);
		////create texture for copy
		//if (pCurrentDepthStencilResourceNoHair == nullptr) {
		//	auto pDevice = pManager->GetRuntimeData().forwarder;
		//	auto pSwapChain = pManager->GetRuntimeData().renderWindows->swapChain;
		//	DXGI_SWAP_CHAIN_DESC swapDesc;
		//	pSwapChain->GetDesc(&swapDesc);
		//	D3D11_TEXTURE2D_DESC desc;
		//	desc.Width = swapDesc.BufferDesc.Width;
		//	desc.Height = swapDesc.BufferDesc.Height;
		//	desc.MipLevels = 1;
		//	desc.ArraySize = 1;
		//	desc.Format = DXGI_FORMAT_R24G8_TYPELESS;
		//	desc.SampleDesc.Count = 1;
		//	desc.SampleDesc.Quality = 0;
		//	desc.Usage = D3D11_USAGE_DEFAULT;
		//	desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		//	desc.CPUAccessFlags = 0;
		//	desc.MiscFlags = 0;
		//	pDevice->CreateTexture2D(&desc, NULL, &pCurrentDepthStencilResourceNoHair);
		//}
		//if (pCurrentDepthStencilResourceWithHair == nullptr) {
		//	auto                 pDevice = pManager->GetRuntimeData().forwarder;
		//	auto                 pSwapChain = pManager->GetRuntimeData().renderWindows->swapChain;
		//	DXGI_SWAP_CHAIN_DESC swapDesc;
		//	pSwapChain->GetDesc(&swapDesc);
		//	D3D11_TEXTURE2D_DESC desc;
		//	desc.Width = swapDesc.BufferDesc.Width;
		//	desc.Height = swapDesc.BufferDesc.Height;
		//	desc.MipLevels = 1;
		//	desc.ArraySize = 1;
		//	desc.Format = DXGI_FORMAT_R24G8_TYPELESS;
		//	desc.SampleDesc.Count = 1;
		//	desc.SampleDesc.Quality = 0;
		//	desc.Usage = D3D11_USAGE_DEFAULT;
		//	desc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
		//	desc.CPUAccessFlags = 0;
		//	desc.MiscFlags = 0;
		//	pDevice->CreateTexture2D(&desc, NULL, &pCurrentDepthStencilResourceWithHair);
		//}
		//if (pHairDepthSRV == nullptr) {
		//	auto pDevice = pManager->GetRuntimeData().forwarder;
		//	D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc;
		//	viewDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
		//	viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		//	viewDesc.Texture2D.MipLevels = 1;
		//	viewDesc.Texture2D.MostDetailedMip = 0;
		//	logger::info("Creating Community Shaders depth SRV");
		//	HRESULT hr2 = pDevice->CreateShaderResourceView(pCurrentDepthStencilResourceWithHair, &viewDesc, &pHairDepthSRV);
		//	Util::printHResult(hr2);
		//	if (pHairDepthSRV == nullptr) {
		//		//Util::PrintAllD3D11DebugMessages(pDevice);
		//		logger::info("Depth SRV creation failed!");
		//	}
		//}
		//logger::info("Copying depth stencil");
		//pContext->CopyResource(pCurrentDepthStencilResourceNoHair, pOriginalDepthTexture);
		//DrawShadows();
		////copy new depth with hair
		//pContext->CopyResource(pCurrentDepthStencilResourceWithHair, pOriginalDepthTexture);
		//if (Menu::GetSingleton()->communityShadersScreenSpaceShadowsCheckbox && pHairDepthSRV != nullptr) {
		//	Util::OverrideCommunityShadersScreenSpaceShadowsDepthTexture(pHairDepthSRV);
		//}
		////hack to enable AO even if we don't want shadows
		//if (!Menu::GetSingleton()->drawShadowsCheckbox) {
		//	logger::info("Restoring original depth to disable shadows");
		//	pContext->CopyResource(pOriginalDepthTexture, pCurrentDepthStencilResourceNoHair);
		//}
		////catch next copy to stop hair depth from being used elsewhere
		//catchNextResourceCopy = true;
		//overrideResource = pCurrentDepthStencilResourceNoHair;
		auto tfx = SkyrimTressFX::GetSingleton();
		tfx->m_currentState = draw_shadows;
		return func(a1, a2);
	}
	static inline REL::Relocation<decltype(thunk)> func;
};

struct Sub_ShadowMap
{
	static INT64 thunk(INT64 a1)
	{
		auto                    val = func(a1);
		//auto                    pContext = RE::BSGraphics::Renderer::GetSingleton()->GetRuntimeData().context;
		//if (pCurrentDepthStencilResourceNoHair == nullptr) {
		//	logger::warn("Copied depth texture is null!");
		//} else if(pOriginalDepthTexture == nullptr){
		//	logger::warn("Original depth texture is null!");
		//}
		//else {
		//	//TODO: Can I intercept the RTV used for shadows instead of copying again?
		//	logger::info("Copying depth stencil for HBAO");
		//	HBAOPlus::GetSingleton()->CopyDSVTexture(pCurrentDepthStencilResourceWithHair);
		//	pContext->CopyResource(pOriginalDepthTexture, pCurrentDepthStencilResourceNoHair);
		//}
		auto tfx = SkyrimTressFX::GetSingleton();
		tfx->m_currentState = draw_shadows;
		return val;
	}
	static inline REL::Relocation<decltype(thunk)> func;
};

void hookGameLoop() {
	REL::Relocation<uintptr_t> update{ RELOCATION_ID(35551, NULL) };
	stl::write_thunk_call<Main_Update>(update.address()+ RELOCATION_OFFSET(0x11F, NULL));
}
void hookMainDraw() {
	//stl::write_thunk_call<Hooks::Main_DrawWorld_MainDraw>(REL::RelocationID(100424, 82084).address() + REL::Relocate(0x2B4, 0x17A));  // EBF510 (EBF67F), F05BF0 (F05D6A) //TODO AE
	stl::write_thunk_call<Hooks::Main_DrawWorld_MainDraw>(REL::RelocationID(99939, 82084).address() + REL::Relocate(0x616, 0x17A));  // EBF510 (EBF67F), F05BF0 (F05D6A) //TODO AE
	
	//stl::write_thunk_call<Hooks::Main_DrawWorld_MainDraw_2>(REL::RelocationID(79947, 82084).address() + REL::Relocate(0x7E, 0x17A)); //TODO AE
	//stl::write_thunk_call<Hooks::Main_DrawWorld_MainDraw_2>(REL::RelocationID(35560, 82084).address() + REL::Relocate(0x492, 0x17A));  //TODO AE //shadow light draws
	
	//2nd depth pass
	stl::write_thunk_call<BSShadowLight_DrawShadows>(REL::RelocationID(101495, 82084).address() + REL::Relocate(0xC6, 0x17A));  //TODO AE //shadow light draws
	
	//3rd depth pass
	stl::write_thunk_call<BSShadowLight_DrawShadows>(REL::RelocationID(101495, 82084).address() + REL::Relocate(0x12B, 0x17A));  //TODO AE //shadow light draws

	//4th depth pass, copy depth
	stl::write_thunk_call<Sub_DrawShadows>(REL::RelocationID(100421, 82084).address() + REL::Relocate(0x4F9, 0x17A));            //TODO AE //shadow light draws

	//shadow map, replace depth
	stl::write_thunk_call<Sub_ShadowMap>(REL::RelocationID(35560, 82084).address() + REL::Relocate(0x492, 0x17A));  //TODO AE //shadow map
	
	//stl::write_vfunc<0x0A, BSShadowLight_DrawShadows>(RE::VTABLE_BSShadowDirectionalLight[0]);

	//draw markers
	stl::write_thunk_call<Hooks::Main_PostDrawWorld_MainDraw>(REL::RelocationID(79947, 82084).address() + REL::Relocate(0x16F, 0x17A));  // EBF510 (EBF67F), F05BF0 (F05D6A)
}
void hookFacegen()
{
	stl::write_vfunc<0x3E, BSFaceGenNiNode_FixSkinInstances>(RE::VTABLE_BSFaceGenNiNode[0]);
}
using namespace std::chrono;
namespace LightHooks{
	void Install(){
		UpdateHooks::Hook();
	}
	void UpdateHooks::Nullsub()
	{
		SkyrimTressFX::GetSingleton()->UpdateLights();
		//get current transforms now, because they're borked later
		for (auto& hair : SkyrimTressFX::GetSingleton()->m_activeScene.objects){
			hair.hairStrands->UpdateBones();
		}
		//simulate before render pass
		auto tfx = SkyrimTressFX::GetSingleton();
		auto camera = RE::PlayerCamera::GetSingleton();
		if (camera != nullptr && camera->currentState != nullptr && (camera->currentState->id == RE::CameraState::kThirdPerson || camera->currentState->id == RE::CameraState::kFree || camera->currentState->id == RE::CameraState::kDragon || camera->currentState->id == RE::CameraState::kFurniture || camera->currentState->id == RE::CameraState::kMount)) {
			uint64_t ms = duration_cast<milliseconds>(
				system_clock::now().time_since_epoch()).count();
			double time = (double)ms / 1000;
			time;
			tfx->Update();
			tfx->Simulate(time, false, false);
		}
		_Nullsub();
	}
}
