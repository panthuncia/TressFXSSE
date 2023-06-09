#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d11.lib")
#include <d3d11.h>
#pragma comment(lib, "d3d12.lib")
#include <d3d12.h>
#include <d3d11on12.h>
#include "Detours.h"
#include "TressFXAsset.h"
#include "TressFXLayouts.h"
#include "AMD_TressFX.h"
#include "LOCAL_RE/BSGraphics.h"
#include "LOCAL_RE/BSGraphicsTypes.h"
#include "Hair.h"
#include "SkyrimGPUInterface.h"
#include "SkeletonInterface.h"
#include "ActorManager.h"
decltype(&IDXGISwapChain::Present) ptrPresent;
decltype(&D3D11CreateDeviceAndSwapChain)             ptrD3D11CreateDeviceAndSwapChain;
decltype(&ID3D11DeviceContext::DrawIndexed)          ptrDrawIndexed;
decltype(&ID3D11DeviceContext::DrawIndexedInstanced) ptrDrawIndexedInstanced;
decltype(&ID3D11DeviceContext::RSSetViewports) ptrRSSetViewports;

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
void setCallbacks()
{
	AMD::g_Callbacks.pfMalloc = malloc;
	AMD::g_Callbacks.pfFree = free;
	AMD::g_Callbacks.pfError = LogError;

	AMD::g_Callbacks.pfRead = TressFX_DefaultRead;
	AMD::g_Callbacks.pfSeek = TressFX_DefaultSeek;

	AMD::g_Callbacks.pfCreateLayout = CreateLayout;
	AMD::g_Callbacks.pfDestroyLayout = DestroyLayout;

	AMD::g_Callbacks.pfCreateReadOnlySB = CreateReadOnlySB;
	AMD::g_Callbacks.pfCreateReadWriteSB = CreateReadWriteSB;
	AMD::g_Callbacks.pfCreateCountedSB = CreateCountedSB;

	AMD::g_Callbacks.pfClearCounter = ClearCounter;
	//AMD::g_Callbacks.pfCopy = SuCopy;
	AMD::g_Callbacks.pfMap = Map;
	AMD::g_Callbacks.pfUnmap = Unmap;

	AMD::g_Callbacks.pfDestroySB = Destroy;

	//AMD::g_Callbacks.pfCreateRT = SuCreateRT;
	AMD::g_Callbacks.pfCreate2D = Create2D;

	AMD::g_Callbacks.pfClear2D = Clear2D;

	AMD::g_Callbacks.pfSubmitBarriers = SubmitBarriers;

	AMD::g_Callbacks.pfCreateBindSet = CreateBindSet;
	AMD::g_Callbacks.pfDestroyBindSet = DestroyBindSet;
	AMD::g_Callbacks.pfBind = Bind;

	AMD::g_Callbacks.pfCreateComputeShaderPSO = CreateComputeShaderPSO;
	//AMD::g_Callbacks.pfDestroyPSO = SuDestroyPSO;
	AMD::g_Callbacks.pfDispatch = Dispatch;

	AMD::g_Callbacks.pfCreateIndexBuffer = CreateIndexBuffer;
	AMD::g_Callbacks.pfDestroyIB = DestroyIB;
	AMD::g_Callbacks.pfDraw = DrawIndexedInstanced;
}
HRESULT WINAPI hk_D3D11CreateDeviceAndSwapChain(
	IDXGIAdapter*               pAdapter,
	D3D_DRIVER_TYPE             DriverType,
	HMODULE                     Software,
	UINT                        Flags,
	const D3D_FEATURE_LEVEL*    pFeatureLevels,
	UINT                        FeatureLevels,
	UINT                        SDKVersion,
	const DXGI_SWAP_CHAIN_DESC* pSwapChainDesc,
	IDXGISwapChain**            ppSwapChain,
	ID3D11Device**              ppDevice,
	D3D_FEATURE_LEVEL*          pFeatureLevel,
	ID3D11DeviceContext**       ppImmediateContext)
{
	/*pFeatureLevel;
	SDKVersion;
	Software;
	DriverType;*/
	//Flags |= D3D11_CREATE_DEVICE_DEBUG; 
	logger::info("Calling original D3D11CreateDeviceAndSwapChain");
	HRESULT hr = (*ptrD3D11CreateDeviceAndSwapChain)(pAdapter,
		DriverType,
		Software,
		Flags,
		pFeatureLevels,
		FeatureLevels,
		SDKVersion,
		pSwapChainDesc,
		ppSwapChain,
		ppDevice,
		pFeatureLevel,
		ppImmediateContext);
	
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

	auto manager = RE::BSGraphics::Renderer::GetSingleton();
	auto device = manager->GetRuntimeData().forwarder;
	auto context = manager->GetRuntimeData().context;
	auto swapchain = manager->GetRuntimeData().renderWindows->swapChain;

	SkyrimGPUResourceManager* gpuResourceManager = SkyrimGPUResourceManager::GetInstance(device, swapchain);
	//init hair resources
	new Hair(asset, gpuResourceManager, context, "hairTest");

	return hr;
}

HRESULT WINAPI hk_IDXGISwapChain_Present(IDXGISwapChain* This, UINT SyncInterval, UINT Flags)
{
	//Clustered::GetSingleton()->OnPresent();
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

void hk_ID3D11DeviceContext_RSSetViewports(ID3D11DeviceContext* This, UINT NumViewports, const D3D11_VIEWPORT *pViewports){
	//logger::info("Num viewports: {}", NumViewports);
	//take first viewport
	if (NumViewports > 0) {
		Hair::currentViewport.TopLeftX = pViewports->TopLeftX;
		Hair::currentViewport.TopLeftY = pViewports->TopLeftY;
		Hair::currentViewport.Height = pViewports->Height;
		Hair::currentViewport.Width = pViewports->Width;
		Hair::currentViewport.MinDepth = pViewports->MinDepth;
		Hair::currentViewport.MaxDepth = pViewports->MaxDepth;
	}
	(This->*ptrRSSetViewports)(NumViewports, pViewports);
}

GUID WKPDID_D3DDebugObjectNameT = { 0x429b8c22, 0x9188, 0x4b0c, 0x87, 0x42, 0xac, 0xb0, 0xbf, 0x85, 0xc2, 0x00 };


typedef void (*LoadShaders_t)(BSGraphics::BSShader* shader, std::uintptr_t stream);
LoadShaders_t oLoadShaders;
bool          loadinit = false;

void hk_LoadShaders(BSGraphics::BSShader* bsShader, std::uintptr_t stream)
{
	oLoadShaders(bsShader, stream);
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
struct Hooks
{
	struct BSGraphics_Renderer_Init_InitD3D
	{
		static void thunk()
		{
			logger::info("Calling original Init3D");

			func();

			auto manager = RE::BSGraphics::Renderer::GetSingleton();
			//auto device = manager->GetRuntimeData().forwarder;
			auto context = manager->GetRuntimeData().context;
			auto swapchain = manager->GetRuntimeData().renderWindows->swapChain;

			logger::info("Detouring virtual function tables");

			*(uintptr_t*)&ptrPresent = Detours::X64::DetourClassVTable(*(uintptr_t*)swapchain, &hk_IDXGISwapChain_Present, 8);
			*(uintptr_t*)&ptrDrawIndexed = Detours::X64::DetourClassVTable(*(uintptr_t*)context, &hk_ID3D11DeviceContext_DrawIndexed, 12);
			*(uintptr_t*)&ptrDrawIndexedInstanced = Detours::X64::DetourClassVTable(*(uintptr_t*)context, &hk_ID3D11DeviceContext_DrawIndexedInstanced, 20);
			*(uintptr_t*)&ptrRSSetViewports = Detours::X64::DetourClassVTable(*(uintptr_t*)context, &hk_ID3D11DeviceContext_RSSetViewports, 44);
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	static void Install()
	{
		//hook CreateDeviceAndSwapChain
		//Thanks PureDark
		LPCWSTR ptr = nullptr;
		auto  moduleBase = (uintptr_t)GetModuleHandle(ptr);
		auto  dllD3D11 = GetModuleHandleA("d3d11.dll");
		*(FARPROC*)&ptrD3D11CreateDeviceAndSwapChain = GetProcAddress(dllD3D11, "D3D11CreateDeviceAndSwapChain");
		Detours::IATHook(moduleBase, "d3d11.dll", "D3D11CreateDeviceAndSwapChain", (uintptr_t)hk_D3D11CreateDeviceAndSwapChain);

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
