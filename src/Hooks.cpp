#include "Hooks.h"
#include "HookEvents.h"
#include "ActorManager.h"
#include "Hair.h"
#include <sstream> //for std::stringstream 
#include <string>  //for std::string
#include <d3d11.h>
decltype(&RE::BSFaceGenNiNode::FixSkinInstances) ptrFixSkinInstances;

struct BSFaceGenNiNode_FixSkinInstances
{
	static void thunk(RE::BSFaceGenNiNode* self, RE::NiNode* a_skeleton, bool a_arg2)
	{
		const char* name = "";
		uint32_t formId = 0x0;

		if (a_skeleton->GetUserData() && a_skeleton->GetUserData()->GetBaseObject())
		{
			logger::info("2");
			auto bname = skyrim_cast<RE::TESNPC*>(a_skeleton->GetUserData()->GetBaseObject());
			if (bname)
				name = bname->GetName();
			logger::info("3");
			auto bnpc = skyrim_cast<RE::TESNPC*>(a_skeleton->GetUserData()->GetBaseObject());
			if (bnpc && bnpc->faceNPC)
				formId = bnpc->faceNPC->formID;
		}
		logger::info("4");
		logger::info("SkinAllGeometry {} {}, {}, (formid {} base form {} head template form {})",
			a_skeleton->name, a_skeleton->GetChildren().capacity(), name,
			a_skeleton->GetUserData() ? a_skeleton->GetUserData()->formID : 0x0,
			a_skeleton->GetUserData() ? a_skeleton->GetUserData()->GetBaseObject()->formID : 0x0, formId);
		logger::info("5");
		if ((a_skeleton->GetUserData() && a_skeleton->GetUserData()->formID == 0x14) || hdt::ActorManager::instance()->m_skinNPCFaceParts)
		{
			hdt::SkinAllHeadGeometryEvent e;
			e.skeleton = a_skeleton;
			//e.headNode = this;
#ifdef ANNIVERSARY_EDITION
			SkinAllGeometryCalls(a_skeleton, a_unk);
#else
			//CALL_MEMBER_FN(this, SkinAllGeometry)(a_skeleton, a_unk);
			func(self, a_skeleton, a_arg2);
#endif
			e.hasSkinned = true;
			hdt::g_skinAllHeadGeometryEventDispatcher.dispatch(e);
		}
		else
		{
			//CALL_MEMBER_FN(this, SkinAllGeometry)(a_skeleton, a_unk);
			func(self, a_skeleton, a_arg2);
		}
	}
	static inline REL::Relocation<decltype(thunk)> func;
};
struct Main_Update
{
	static void thunk()
	{
		//call main game loop
		func();
		//draw hair
		auto hair = Hair::hairs.find("hairTest");
		hair->second->draw();
	}
	static inline REL::Relocation<decltype(thunk)> func;
};
void hookGameLoop() {
	REL::Relocation<uintptr_t> update{ RELOCATION_ID(35551, NULL) };
	stl::write_thunk_call<Main_Update>(update.address()+ RELOCATION_OFFSET(0x11F, NULL));
}
void hookFacegen() {
	stl::write_vfunc<0x3E, BSFaceGenNiNode_FixSkinInstances>(RE::VTABLE_BSFaceGenNiNode[0]);
}
