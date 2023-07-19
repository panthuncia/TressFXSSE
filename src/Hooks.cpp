#include "Hooks.h"
#include "HookEvents.h"
#include "ActorManager.h"
#include "Hair.h"
#include <sstream> //for std::stringstream 
#include <string>  //for std::string
#include <d3d11.h>
#include "Util.h"
#include "PPLLObject.h"
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
				//RE::ThirdPersonState* tps = reinterpret_cast<RE::ThirdPersonState*>(camera->currentState.get());
				if (skipFrame) {
					skipFrame--;
					logger::info("skipping frame");
					return;
				}
				ppll->UpdateVariables();
				ppll->Simulate();
				ppll->Draw();
			}
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};
	struct Main_DrawWorld_MainDraw_2 {
		static void thunk(){
			func();
			logger::info("in unknown function");
			auto ppll = PPLLObject::GetSingleton();
			ppll->m_gameViewMatrix = RE::BSGraphics::RendererShadowState::GetSingleton()->GetRuntimeData().cameraData.getEye().viewMat;
			logger::info("Got view mat: ");
			Util::PrintXMMatrix(ppll->m_gameViewMatrix);
			ppll->m_gameProjMatrix = RE::BSGraphics::RendererShadowState::GetSingleton()->GetRuntimeData().cameraData.getEye().projMat;
			ppll->m_gameProjMatrixUnjittered = RE::BSGraphics::RendererShadowState::GetSingleton()->GetRuntimeData().cameraData.getEye().projMatrixUnjittered;
			ppll->m_gameViewProjMatrix = RE::BSGraphics::RendererShadowState::GetSingleton()->GetRuntimeData().cameraData.getEye().viewProjMat;
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};
};
void hookGameLoop() {
	REL::Relocation<uintptr_t> update{ RELOCATION_ID(35551, NULL) };
	stl::write_thunk_call<Main_Update>(update.address()+ RELOCATION_OFFSET(0x11F, NULL));
}
void hookMainDraw() {
	//credit Doodlez
	stl::write_thunk_call<Hooks::Main_DrawWorld_MainDraw>(REL::RelocationID(79947, 82084).address() + REL::Relocate(0x16F, 0x17A));  // EBF510 (EBF67F), F05BF0 (F05D6A)
	//stl::write_thunk_call<Hooks::Main_DrawWorld_MainDraw_2>(REL::RelocationID(79947, 82084).address() + REL::Relocate(0x7E, 0x17A));
}
void hookFacegen()
{
	stl::write_vfunc<0x3E, BSFaceGenNiNode_FixSkinInstances>(RE::VTABLE_BSFaceGenNiNode[0]);
}
namespace BoneHooks{
	void Install(){
		logger::info("Installing bone hooks");
		UpdateHooks::Hook();
	}
	void UpdateHooks::Nullsub()
	{
		_Nullsub();
		for (auto hairPair : PPLLObject::GetSingleton()->m_hairs){
			hairPair.second->UpdateBones();
		}
	}
}
