#pragma comment(lib, "d3d11.lib")
#include <d3d11.h>

#include "Detours.h"
#include "TressFXAsset.h"
#include "TressFXLayouts.h"
#include "AMD_TressFX.h"
#include "LOCAL_RE/BSGraphics.h"
#include "LOCAL_RE/BSGraphicsTypes.h"
#include "Hair.h"
#include "SkyrimGPUInterface.h"
#include "SkeletonInterface.h"
decltype(&IDXGISwapChain::Present) ptrPresent;

decltype(&ID3D11DeviceContext::DrawIndexed)          ptrDrawIndexed;
decltype(&ID3D11DeviceContext::DrawIndexedInstanced) ptrDrawIndexedInstanced;

// This could instead be retrieved as a variable from the
// script manager, or passed as an argument.
static const size_t AVE_FRAGS_PER_PIXEL = 4;
static const size_t PPLL_NODE_SIZE = 16;



// See TressFXLayouts.h
// By default, app allocates space for each of these, and TressFX uses it.
// These are globals, because there should really just be one instance.
TressFXLayouts* g_TressFXLayouts = 0;


extern "C"
{
	void TressFX_DefaultRead(void* ptr, AMD::uint size, EI_Stream* pFile)
	{
		FILE* fp = reinterpret_cast<FILE*>(pFile);
		fread(ptr, size, 1, fp);
	}
	void TressFX_DefaultSeek(EI_Stream* pFile, AMD::uint offset)
	{
		FILE* fp = reinterpret_cast<FILE*>(pFile);
		fseek(fp, offset, SEEK_SET);
	}
}


HRESULT WINAPI hk_IDXGISwapChain_Present(IDXGISwapChain* This, UINT SyncInterval, UINT Flags)
{
	//Clustered::GetSingleton()->OnPresent();
	//hairObject.DrawStrands();
	return (This->*ptrPresent)(SyncInterval, Flags);
}

void hk_ID3D11DeviceContext_DrawIndexed(ID3D11DeviceContext* This, UINT IndexCount, UINT StartIndexLocation, INT BaseVertexLocation)
{
	//Lighting::GetSingleton()->OnDraw();
	(This->*ptrDrawIndexed)(IndexCount, StartIndexLocation, BaseVertexLocation);
}

void hk_ID3D11DeviceContext_DrawIndexedInstanced(ID3D11DeviceContext* This, UINT IndexCountPerInstance,
	UINT InstanceCount,
	UINT StartIndexLocation,
	INT  BaseVertexLocation,
	UINT StartInstanceLocation)
{
	//Grass::GetSingleton()->OnDraw();
	(This->*ptrDrawIndexedInstanced)(IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
}

GUID WKPDID_D3DDebugObjectNameT = { 0x429b8c22, 0x9188, 0x4b0c, 0x87, 0x42, 0xac, 0xb0, 0xbf, 0x85, 0xc2, 0x00 };

void SetResourceName(ID3D11DeviceChild* Resource, const char* Format, ...)
{
	if (!Resource)
		return;

	char    buffer[1024];
	va_list va;

	va_start(va, Format);
	int len = _vsnprintf_s(buffer, _TRUNCATE, Format, va);
	va_end(va);

	Resource->SetPrivateData(WKPDID_D3DDebugObjectNameT, len, buffer);
}

void AddTextureDebugInformation()
{
	auto r = BSGraphics::Renderer::QInstance();
	for (int i = 0; i < RenderTargets::RENDER_TARGET_COUNT; i++) {
		auto rt = r->pRenderTargets[i];
		auto name = RTNames[i];
		if (rt.RTV) {
			SetResourceName(rt.RTV, "%s RTV", name);
		}
		if (rt.SRV) {
			SetResourceName(rt.SRV, "%s SRV", name);
		}
		if (rt.SRVCopy) {
			SetResourceName(rt.SRVCopy, "%s COPY SRV", name);
		}
		if (rt.Texture) {
			SetResourceName(rt.Texture, "%s TEXTURE", name);
		}
	}
	for (int i = 0; i < RenderTargetsCubemaps::RENDER_TARGET_CUBEMAP_COUNT; i++) {
		auto rt = r->pCubemapRenderTargets[i];
		auto name = RTCubemapNames[i];
		if (rt.SRV) {
			SetResourceName(rt.SRV, "%s SRV", name);
		}
		for (int k = 0; k < 6; k++) {
			if (rt.CubeSideRTV[k]) {
				SetResourceName(rt.CubeSideRTV[k], "%s SIDE SRV", name);
			}
		}
		if (rt.Texture) {
			SetResourceName(rt.Texture, "%s TEXTURE", name);
		}
	}

		for (int i = 0; i < RenderTargetsDepthStencil::DEPTH_STENCIL_COUNT; i++) {
		auto rt = r->pDepthStencils[i];
			auto name = DepthNames[i];
		if (rt.DepthSRV) {
				SetResourceName(rt.DepthSRV, "%s DEPTH SRV", name);
		}
		if (rt.StencilSRV) {
			SetResourceName(rt.StencilSRV, "%s STENCIL SRV", name);
		}
		for (int k = 0; k < 8; k++) {
			if (rt.Views[k]) {
				SetResourceName(rt.Views[k], "%s VIEWS SRV", name);
			}
			if (rt.ReadOnlyViews[k]) {
				SetResourceName(rt.Views[k], "%s READONLY VIEWS SRV", name);
			}
		}
		if (rt.Texture) {
			SetResourceName(rt.Texture, "%s TEXTURE", name);
		}
	}
}


typedef void (*LoadShaders_t)(BSGraphics::BSShader* shader, std::uintptr_t stream);
LoadShaders_t oLoadShaders;
bool          loadinit = false;

void hk_LoadShaders(BSGraphics::BSShader* bsShader, std::uintptr_t stream)
{
	oLoadShaders(bsShader, stream);
	if (!loadinit) {
		AddTextureDebugInformation();
		loadinit = true;
	}

	for (const auto& entry : bsShader->m_PixelShaderTable) {
		SetResourceName(entry->m_Shader, "%s %u PS", bsShader->m_LoaderType, entry->m_TechniqueID);
	}
	for (const auto& entry : bsShader->m_VertexShaderTable) {
		SetResourceName(entry->m_Shader, "%s %u VS", bsShader->m_LoaderType, entry->m_TechniqueID);
	}
};

uintptr_t LoadShaders;

void InstallLoadShaders()
{
	logger::info("Installing BSShader::LoadShaders hook");
	{
		LoadShaders = RELOCATION_ID(101339, 108326).address();
		struct Patch : Xbyak::CodeGenerator
		{
			Patch()
			{
				Xbyak::Label origFuncJzLabel;

				test(rdx, rdx);
				jz(origFuncJzLabel);
				jmp(ptr[rip]);
				dq(LoadShaders + 0x9);

				L(origFuncJzLabel);
				jmp(ptr[rip]);
				dq(LoadShaders + 0xF0);
			}
		};

		Patch p;
		p.ready();

		SKSE::AllocTrampoline(1 << 6);

		auto& trampoline = SKSE::GetTrampoline();
		oLoadShaders = static_cast<LoadShaders_t>(trampoline.allocate(p));
		trampoline.write_branch<6>(
			LoadShaders,
			hk_LoadShaders);
	}
	logger::info("Installed");
}
void hookSkinAllGeometry() {
	REL::Relocation<uintptr_t> skinAllGeometryHook{ RELOCATION_ID(26405, 26986) };  // 0x3d87b0, 0x1403F0830, SkinAllGeometry
}
void setCallbacks() {
	AMD::g_Callbacks.pfMalloc = malloc;
	AMD::g_Callbacks.pfFree = free;
	AMD::g_Callbacks.pfError = LogError;

	AMD::g_Callbacks.pfRead = TressFX_DefaultRead;
	AMD::g_Callbacks.pfSeek = TressFX_DefaultSeek;

	//AMD::g_Callbacks.pfCreateLayout = SuCreateLayout;
	//AMD::g_Callbacks.pfDestroyLayout = SuDestroyLayout;

	AMD::g_Callbacks.pfCreateReadOnlySB = CreateReadOnlySB;
	AMD::g_Callbacks.pfCreateReadWriteSB = CreateReadWriteSB;
	AMD::g_Callbacks.pfCreateCountedSB = CreateCountedSB;

	//AMD::g_Callbacks.pfClearCounter = SuClearCounter;
	//AMD::g_Callbacks.pfCopy = SuCopy;
	AMD::g_Callbacks.pfMap = Map;
	AMD::g_Callbacks.pfUnmap = Unmap;

	//AMD::g_Callbacks.pfDestroySB = SuDestroy;

	//AMD::g_Callbacks.pfCreateRT = SuCreateRT;
	//AMD::g_Callbacks.pfCreate2D = SuCreate2D;

	//AMD::g_Callbacks.pfClear2D = SuClear2D;

	AMD::g_Callbacks.pfSubmitBarriers = SubmitBarriers;

	AMD::g_Callbacks.pfCreateBindSet = CreateBindSet;
	AMD::g_Callbacks.pfDestroyBindSet = DestroyBindSet;
	//AMD::g_Callbacks.pfBind = SuBind;

	//AMD::g_Callbacks.pfCreateComputeShaderPSO = SuCreateComputeShaderPSO;
	//AMD::g_Callbacks.pfDestroyPSO = SuDestroyPSO;
	//AMD::g_Callbacks.pfDispatch = SuDispatch;

	AMD::g_Callbacks.pfCreateIndexBuffer = CreateIndexBuffer;
	AMD::g_Callbacks.pfDestroyIB = DestroyIB;
	//AMD::g_Callbacks.pfDraw = SuDrawIndexedInstanced;
}
struct Hooks
{
	struct BSGraphics_Renderer_Init_InitD3D
	{
		static void thunk()
		{
			logger::info("Calling original Init3D");

			func();

			logger::info("Loading hair API");
			setCallbacks();
			FILE* fp;
			logger::info("Opening file");
			fopen_s(&fp, "D:\\Skyrim installations\\1.5.97\\FoxHair.tfx", "rb");
			if (!fp) {
				logger::info("File opening failed");
			}
			FILE* fpBone;
			logger::info("Opening bone file");
			fopen_s(&fpBone, "D:\\Skyrim installations\\1.5.97\\FoxHair.tfxbone", "rb");
			if (!fpBone) {
				logger::info("File opening failed");
			}
			AMD::TressFXAsset* asset = new AMD::TressFXAsset();
			logger::info("Loading hair data");
			asset->LoadHairData(fp);
			logger::info("Closing file");
			fclose(fp);
			logger::info("Loading bone data");
			SkeletonInterface placeholder;
			asset->LoadBoneData(placeholder, fpBone);
			logger::info("Closing file");
			fclose(fp);
			logger::info("Processing asset");
			asset->ProcessAsset();
			logger::info("Accessing render device information");

			auto manager = RE::BSRenderManager::GetSingleton();
			auto device = manager->GetRuntimeData().forwarder;
			auto context = manager->GetRuntimeData().context;
			auto swapchain = manager->GetRuntimeData().swapChain;

			SkyrimGPUResourceManager* gpuResourceManager = SkyrimGPUResourceManager::GetInstance(device);
			//init hair resources
			Hair hair(asset, gpuResourceManager, context, "hairTest");
			logger::info("Detouring virtual function tables");

			*(uintptr_t*)&ptrPresent = Detours::X64::DetourClassVTable(*(uintptr_t*)swapchain, &hk_IDXGISwapChain_Present, 8);
			*(uintptr_t*)&ptrDrawIndexed = Detours::X64::DetourClassVTable(*(uintptr_t*)context, &hk_ID3D11DeviceContext_DrawIndexed, 12);
			*(uintptr_t*)&ptrDrawIndexedInstanced = Detours::X64::DetourClassVTable(*(uintptr_t*)context, &hk_ID3D11DeviceContext_DrawIndexedInstanced, 20);
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	static void Install()
	{
		stl::write_thunk_call<BSGraphics_Renderer_Init_InitD3D>(REL::RelocationID(75595, 77226).address() + REL::Relocate(0x50, 0x2BC));
		//Detours::X64::DetourFunction(*(uintptr_t*)RELOCATION_ID(101339, 108326).address(), (uintptr_t)&hk_LoadShaders);
		InstallLoadShaders();
		logger::info("Installed render startup hooks");
	}
};

void PatchD3D11()
{
	Hooks::Install();
}
