#include "Hooks.h"
#include "HookEvents.h"
#include "ActorManager.h"
#include "Hair.h"
#include <sstream> //for std::stringstream 
#include <string>  //for std::string
#include <d3d11.h>
#include "Util.h"
#include "PPLLObject.h"
#include "Menu.h"
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
		PPLLObject* ppll = PPLLObject::GetSingleton();
		if (ppll->m_doReload) {
			ppll->ReloadAllHairs();
		}
		func();
	}
	static inline REL::Relocation<decltype(thunk)> func;
};
uint8_t skipFrame = 10;
struct Hooks
{
	struct Main_DrawWorld_MainDraw
	{
		static void thunk(INT64 BSGraphics_Renderer, int unk)
		{
			func(BSGraphics_Renderer, unk);
			//draw hair
			auto camera = RE::PlayerCamera::GetSingleton();
			auto ppll = PPLLObject::GetSingleton();
			if (camera != nullptr && camera->currentState != nullptr && (camera->currentState->id == RE::CameraState::kThirdPerson || camera->currentState->id == RE::CameraState::kFree || 
				camera->currentState->id == RE::CameraState::kDragon || camera->currentState->id == RE::CameraState::kFurniture || camera->currentState->id == RE::CameraState::kMount)) {
				ppll->PreDraw();
				ppll->UpdateVariables();
				ppll->Simulate();
				if (Menu::GetSingleton()->drawHairCheckbox) {
					ppll->Draw();
				}
				ppll->PostDraw();
				//MarkerRender::GetSingleton()->DrawWorldAxes(PPLLObject::GetSingleton()->m_cameraWorld, PPLLObject::GetSingleton()->m_viewXMMatrix, PPLLObject::GetSingleton()->m_projXMMatrix);
			}
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};
	struct Main_DrawWorld_MainDraw_2 {
		static INT64 thunk(INT64 a1)
		{
			//RE::BSShadowLight* val =  reinterpret_cast<RE::BSShadowLight*>(func(a1, a2));
			auto camera = RE::PlayerCamera::GetSingleton();
			auto ppll = PPLLObject::GetSingleton();
			if (ppll->m_gameLoaded && camera != nullptr && camera->currentState != nullptr && (camera->currentState->id == RE::CameraState::kThirdPerson || camera->currentState->id == RE::CameraState::kFree || camera->currentState->id == RE::CameraState::kDragon || camera->currentState->id == RE::CameraState::kFurniture || camera->currentState->id == RE::CameraState::kMount)) {
				//RE::ThirdPersonState* tps = reinterpret_cast<RE::ThirdPersonState*>(camera->currentState.get());
				if (skipFrame) {
					skipFrame--;
					logger::info("skipping frame");
				} else {
					logger::info("drawing axes");
					PPLLObject::GetSingleton()->DrawShadows();
					//MarkerRender::GetSingleton()->DrawWorldAxes(PPLLObject::GetSingleton()->m_cameraWorld, PPLLObject::GetSingleton()->m_viewXMMatrix, PPLLObject::GetSingleton()->m_projXMMatrix);
				}
			}
			return func(a1);
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};
};
struct BSShadowLight_DrawShadows
{
	static void thunk(RE::BSShadowLight* a1, INT64* a2, DWORD* a3, int a4)
	{
		func(a1, a2, a3, a4);
		//Menu::GetSingleton()->DrawVector3(a1->light.get()->world.translate, "shadowLight pos:");
		auto camera = RE::PlayerCamera::GetSingleton();
		auto ppll = PPLLObject::GetSingleton();
		if (ppll->m_gameLoaded && camera != nullptr && camera->currentState != nullptr && (camera->currentState->id == RE::CameraState::kThirdPerson || camera->currentState->id == RE::CameraState::kFree || camera->currentState->id == RE::CameraState::kDragon || camera->currentState->id == RE::CameraState::kFurniture || camera->currentState->id == RE::CameraState::kMount)) {
			//RE::ThirdPersonState* tps = reinterpret_cast<RE::ThirdPersonState*>(camera->currentState.get());
			if (skipFrame) {
				skipFrame--;
				logger::info("skipping frame");
			} else if (Menu::GetSingleton()->drawShadowsCheckbox){
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
				auto worldShadowLightPos = a1->light.get()->world.translate;
				auto worldShadowLightPosScaled = Util::ToRenderScale(glm::vec3(worldShadowLightPos.x, worldShadowLightPos.y, worldShadowLightPos.z));

				//draw marker at light
				auto shadowLightRot = a1->light.get()->world.rotate;
				auto translation = DirectX::XMMatrixTranspose(DirectX::XMMatrixTranslation(worldShadowLightPosScaled.x, worldShadowLightPosScaled.y, worldShadowLightPosScaled.z));
				auto rotation = DirectX::XMMATRIX(shadowLightRot.entry[0][0], shadowLightRot.entry[0][1], shadowLightRot.entry[0][2], 0, shadowLightRot.entry[1][0], shadowLightRot.entry[1][1], shadowLightRot.entry[1][2], 0, shadowLightRot.entry[2][0], shadowLightRot.entry[2][1], shadowLightRot.entry[2][2], 0, 0, 0, 0, 1);
				auto transform = translation * rotation;
				ppll->m_markerPositions.push_back(transform);

				auto forwardVector = shadowState->GetRuntimeData().cameraData.getEye().viewForward;
				DirectX::XMFLOAT3 forward;
				DirectX::XMStoreFloat3(&forward, forwardVector);
				auto rightVector = shadowState->GetRuntimeData().cameraData.getEye().viewRight;
				DirectX::XMFLOAT3 right;
				DirectX::XMStoreFloat3(&right, rightVector);
				auto upVector = shadowState->GetRuntimeData().cameraData.getEye().viewUp;
				DirectX::XMFLOAT3 up;
				DirectX::XMStoreFloat3(&up, upVector);
				//Menu::GetSingleton()->DrawVector3(glm::vec3(forward.x, forward.y, forward.z), "forward");
				//Menu::GetSingleton()->DrawVector3(glm::vec3(right.x, right.y, right.z), "right");
				//Menu::GetSingleton()->DrawVector3(glm::vec3(up.x, up.y, up.z), "up");
				/*glm::mat3 viewRot = glm::mat3({ { gameVFloats._11, gameVFloats._21, gameVFloats._31 },
					{ gameVFloats._12, gameVFloats._22, gameVFloats._32 },
					{ gameVFloats._13, gameVFloats._23, gameVFloats._33 } });*/
				glm::mat3 viewRot = glm::mat3({ { gameVFloats._11, gameVFloats._12, gameVFloats._13 },
							{ gameVFloats._21, gameVFloats._22, gameVFloats._23 },
							{ gameVFloats._31, gameVFloats._32, gameVFloats._33 } });
				glm::mat4 viewMatrix = glm::mat4({ viewRot[0][0], viewRot[0][1], viewRot[0][2], glm::dot(worldShadowLightPosScaled, glm::vec3(viewRot[0][0], viewRot[0][1], viewRot[0][2])) },
					{ viewRot[1][0], viewRot[1][1], viewRot[1][2], glm::dot(worldShadowLightPosScaled, glm::vec3(viewRot[1][0], viewRot[1][1], viewRot[1][2])) },
					{ viewRot[2][0], viewRot[2][1], viewRot[2][2], glm::dot(worldShadowLightPosScaled, glm::vec3(viewRot[2][0], viewRot[2][1], viewRot[2][2])) },
					{ 0, 0, 0, 1 });
				/*glm::mat4  viewMatrix = glm::mat4({ viewRot[0][0], viewRot[0][1], viewRot[0][2], glm::dot(worldShadowLightPosScaled, glm::vec3(viewRot[0][0], viewRot[0][1], viewRot[0][2])) },
							{ viewRot[1][0], viewRot[1][1], viewRot[1][2], glm::dot(worldShadowLightPosScaled, glm::vec3(viewRot[1][0], viewRot[1][1], viewRot[1][2])) },
							{ viewRot[2][0], viewRot[2][1], viewRot[2][2], glm::dot(worldShadowLightPosScaled, glm::vec3(viewRot[2][0], viewRot[2][1], viewRot[2][2])) },
							{ 0, 0, 0, 1 });*/
				auto      viewXMFloat = DirectX::XMFLOAT4X4(&viewMatrix[0][0]);
				auto viewXMMatrix = DirectX::XMMatrixSet(viewXMFloat._11, viewXMFloat._12, viewXMFloat._13, viewXMFloat._14, viewXMFloat._21, viewXMFloat._22, viewXMFloat._23, viewXMFloat._24, viewXMFloat._31, viewXMFloat._32, viewXMFloat._33, viewXMFloat._34, viewXMFloat._41, viewXMFloat._42, viewXMFloat._43, viewXMFloat._44);
				DirectX::XMMATRIX viewProjectionMatrix = projMatrix * viewXMMatrix;
				DirectX::XMMATRIX viewProjectionMatrixInverse = DirectX::XMMatrixInverse(nullptr, viewProjectionMatrix);
				//Menu::GetSingleton()->DrawMatrix(viewXMMatrix, "viewMat");
				//Menu::GetSingleton()->DrawMatrix(viewProjectionMatrix, "viewProjMat");
				ppll->m_pStrandEffect->GetVariableByName("g_mVP")->AsMatrix()->SetMatrix(reinterpret_cast<float*>(&viewProjectionMatrix));
				ppll->m_pStrandEffect->GetVariableByName("g_mInvViewProj")->AsMatrix()->SetMatrix(reinterpret_cast<float*>(&viewProjectionMatrixInverse));
				ppll->m_pStrandEffect->GetVariableByName("g_mView")->AsMatrix()->SetMatrix(reinterpret_cast<float*>(&viewXMMatrix));
				ppll->m_pStrandEffect->GetVariableByName("g_mProjection")->AsMatrix()->SetMatrix(reinterpret_cast<float*>(&projMatrix));
				ppll->m_pStrandEffect->GetVariableByName("g_vEye")->AsVector()->SetFloatVector(reinterpret_cast<float*>(&worldShadowLightPosScaled));
				//get viewport
				auto currentViewport = ppll->m_currentViewport;
				DirectX::XMVECTOR viewportVector = DirectX::XMVectorSet(currentViewport.TopLeftX, currentViewport.TopLeftY, currentViewport.Width, currentViewport.Height);
				ppll->m_pStrandEffect->GetVariableByName("g_vViewport")->AsVector()->SetFloatVector(reinterpret_cast<float*>(&viewportVector));
				logger::info("drawing hair shadows");
				PPLLObject::GetSingleton()->DrawShadows();
				//MarkerRender::GetSingleton()->DrawWorldAxes(PPLLObject::GetSingleton()->m_cameraWorld, PPLLObject::GetSingleton()->m_viewXMMatrix, PPLLObject::GetSingleton()->m_projXMMatrix);
			}
		}
	}
	static inline REL::Relocation<decltype(thunk)> func;
};
void hookGameLoop() {
	REL::Relocation<uintptr_t> update{ RELOCATION_ID(35551, NULL) };
	stl::write_thunk_call<Main_Update>(update.address()+ RELOCATION_OFFSET(0x11F, NULL));
}
void hookMainDraw() {
	//credit Doodlez
	stl::write_thunk_call<Hooks::Main_DrawWorld_MainDraw>(REL::RelocationID(79947, 82084).address() + REL::Relocate(0x16F, 0x17A));  // EBF510 (EBF67F), F05BF0 (F05D6A)
	//stl::write_thunk_call<Hooks::Main_DrawWorld_MainDraw_2>(REL::RelocationID(79947, 82084).address() + REL::Relocate(0x7E, 0x17A)); //TODO AE
	//stl::write_thunk_call<Hooks::Main_DrawWorld_MainDraw_2>(REL::RelocationID(35560, 82084).address() + REL::Relocate(0x492, 0x17A));  //TODO AE //shadow light draws
	stl::write_thunk_call<BSShadowLight_DrawShadows>(REL::RelocationID(101495, 82084).address() + REL::Relocate(0xC6, 0x17A));  //TODO AE //shadow light draws
	//stl::write_vfunc<0x0A, BSShadowLight_DrawShadows>(RE::VTABLE_BSShadowDirectionalLight[0]);
}
void hookFacegen()
{
	stl::write_vfunc<0x3E, BSFaceGenNiNode_FixSkinInstances>(RE::VTABLE_BSFaceGenNiNode[0]);
}
namespace LightHooks{
	void Install(){
		UpdateHooks::Hook();
	}
	void UpdateHooks::Nullsub()
	{
		PPLLObject::GetSingleton()->UpdateLights();
		//get current transforms now, because they're borked later
		for (auto hairPair : PPLLObject::GetSingleton()->m_hairs){
			hairPair.second->UpdateBones();
		}
		_Nullsub();
	}
}
