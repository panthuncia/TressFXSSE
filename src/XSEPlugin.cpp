#include "LOCAL_RE/BSGraphicsTypes.h"
#include "ENB/ENBSeriesAPI.h"
#include "D:/Modding/Dev/TressFXSSE/build/CMakeFiles/TressFXSSE.dir/Release/cmake_pch.hxx"
#include "Hooks.h"
#include "ActorManager.h"
#include "HookEvents.h"
#include "Menu.h"
ENB_API::ENBSDKALT1002* g_ENB = nullptr;

HMODULE m_hModule;

extern "C" __declspec(dllexport) const char* NAME = "TressFXSSE";
extern "C" __declspec(dllexport) const char* DESCRIPTION = "AMD TressFX 4.0 implementation for Skyrim SE";

std::list<std::string> errors;

BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID)
{
	if (dwReason == DLL_PROCESS_ATTACH) m_hModule = hModule;
	return TRUE;
}

void MessageHandler(SKSE::MessagingInterface::Message* message)
{
	switch (message->type) {
	case SKSE::MessagingInterface::kPostLoad:
		g_ENB = reinterpret_cast<ENB_API::ENBSDKALT1002*>(ENB_API::RequestENBAPI(ENB_API::SDKVersion::V1002));
		if (g_ENB) {
			logger::info("Obtained ENB API");
		} else
			logger::info("Unable to acquire ENB API");
		break;
	case SKSE::MessagingInterface::kDataLoaded:
		{
			for (auto it = errors.begin(); it != errors.end(); ++it) {
				auto& errorMessage = *it;
				RE::DebugMessageBox(std::format("TressFXSSE\n{}, will disable all hooks and features", errorMessage).c_str());
			}

			if (errors.empty()) {
				RE::BSInputDeviceManager::GetSingleton()->AddEventSink(Menu::GetSingleton());
			}

			break;
		}
	}
}

void PatchD3D11();

void Load()
{
	//for GPU profiling/debugging: game crashes if SKSE is launched through PIX with forceD3D11on12.
	//Workaround: load PIX DLL manually here, force D3D11on12 through D3DConfig, then attach in PIX.
	const wchar_t wtext[] = L"D:\\Dev\\Microsoft PIX\\2305.10\\WinPixGpuCapturer.dll";
	HINSTANCE hL = LoadLibrary((LPCWSTR)wtext);
	if (!hL) {
		logger::info("Could not acquire PIX dll");
	}
	auto messaging = SKSE::GetMessagingInterface();
	messaging->RegisterListener("SKSE", MessageHandler);

	PatchD3D11();

	hdt::g_skinAllHeadGeometryEventDispatcher.addListener(hdt::ActorManager::instance());
	//hdt::g_skinSingleHeadGeometryEventDispatcher.addListener(hdt::ActorManager::instance());
	hookFacegen();
	hookMainDraw();

	SKSE::AllocTrampoline(1 << 10);
	BoneHooks::Install();
	//hookGameLoop();
	logger::info("Installed skeleton hook");
}

