#include "LOCAL_RE/BSGraphicsTypes.h"
#include "ENB/ENBSeriesAPI.h"

#include "Hooks.h"
#include "ActorManager.h"
#include "HookEvents.h"

ENB_API::ENBSDKALT1002* g_ENB = nullptr;

HMODULE m_hModule;

extern "C" __declspec(dllexport) const char* NAME = "Playground";
extern "C" __declspec(dllexport) const char* DESCRIPTION = "Advanced ReShade utility by doodlez";

BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID)
{
	if (dwReason == DLL_PROCESS_ATTACH) m_hModule = hModule;
	return TRUE;
}


void MessageHandler(SKSE::MessagingInterface::Message* a_msg)
{
	switch (a_msg->type) {
	case SKSE::MessagingInterface::kPostLoad:
		g_ENB = reinterpret_cast<ENB_API::ENBSDKALT1002*>(ENB_API::RequestENBAPI(ENB_API::SDKVersion::V1002));
		if (g_ENB) {
			logger::info("Obtained ENB API");
		} else
			logger::info("Unable to acquire ENB API");
		break;
	}
}

void PatchD3D11();

void Load()
{
	PatchD3D11();

	hdt::g_skinAllHeadGeometryEventDispatcher.addListener(hdt::ActorManager::instance());
	//hdt::g_skinSingleHeadGeometryEventDispatcher.addListener(hdt::ActorManager::instance());
	hookFacegen();
	hookGameLoop();
	logger::info("Installed skeleton hook");

	/*if (reshade::register_addon(m_hModule)) {
		logger::info("Registered addon");
		Lighting::GetSingleton()->Initialise();
		Grass::GetSingleton()->Initialise();
		Clustered::GetSingleton()->Initialise();
	} else {
		logger::info("ReShade not present, not installing hooks");
	}*/
}

