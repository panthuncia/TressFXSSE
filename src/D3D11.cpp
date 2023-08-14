#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d11.lib")
#include <d3d11.h>
#pragma comment(lib, "d3d12.lib")
#include <d3d12.h>
#include <d3d11on12.h>
#include "Detours.h"
#include "TressFX/TressFXAsset.h"
#include "TressFX/AMD_TressFX.h"
#include "LOCAL_RE/BSGraphics.h"
#include "LOCAL_RE/BSGraphicsTypes.h"
#include "SkyrimGPUResourceManager.h"
#include "ActorManager.h"
#include "Menu.h"
#include <iostream>
#include <filesystem>
#include "HBAOPlus.h"
#include "MarkerRender.h"
#include "SkyrimTressFX.h"
decltype(&IDXGISwapChain::Present) ptrPresent;
decltype(&D3D11CreateDeviceAndSwapChain)             ptrD3D11CreateDeviceAndSwapChain;
decltype(&ID3D11DeviceContext::DrawIndexed)          ptrDrawIndexed;
decltype(&ID3D11DeviceContext::DrawIndexedInstanced) ptrDrawIndexedInstanced;
decltype(&ID3D11DeviceContext::RSSetViewports) ptrRSSetViewports;
decltype(&ID3D11DeviceContext::CopyResource)         ptrCopyResource;

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
	Flags |= D3D11_CREATE_DEVICE_DEBUG; 
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
	
	logger::info("Accessing render device information");

	auto manager = RE::BSGraphics::Renderer::GetSingleton();
	auto device = manager->GetRuntimeData().forwarder;
	auto context = manager->GetRuntimeData().context;
	auto swapchain = manager->GetRuntimeData().renderWindows->swapChain;

	//init marker renderer
	MarkerRender::GetSingleton()->InitRenderResources();

	//init manager
	SkyrimGPUResourceManager::GetInstance(device, swapchain);
	//init hair resources
	auto tfx = SkyrimTressFX::GetSingleton();
	tfx->OnCreate();

	Menu::GetSingleton()->Init(swapchain, device, context);
	std::vector<std::string> hairNames;
	for (auto& hair : tfx->m_activeScene.objects) {
		hairNames.push_back(hair.hairStrands.get()->m_hairName);
	}
	Menu::GetSingleton()->UpdateActiveHairs(hairNames);

	HBAOPlus::GetSingleton()->Initialize(device);

	return hr;
}

HRESULT WINAPI hk_IDXGISwapChain_Present(IDXGISwapChain* This, UINT SyncInterval, UINT Flags)
{
	Menu::GetSingleton()->DrawOverlay();
	return (This->*ptrPresent)(SyncInterval, Flags);
}
//
//void hk_ID3D11DeviceContext_DrawIndexed(ID3D11DeviceContext* This, UINT IndexCount, UINT StartIndexLocation, INT BaseVertexLocation)
//{
//	//Lighting::GetSingleton()->OnDraw();
//	(This->*ptrDrawIndexed)(IndexCount, StartIndexLocation, BaseVertexLocation);
//}
//
//void hk_ID3D11DeviceContext_DrawIndexedInstanced(ID3D11DeviceContext* This, UINT IndexCountPerInstance,
//	UINT InstanceCount,
//	UINT StartIndexLocation,
//	INT  BaseVertexLocation,
//	UINT StartInstanceLocation)
//{
//	//Grass::GetSingleton()->OnDraw();
//	(This->*ptrDrawIndexedInstanced)(IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
//}

void hk_ID3D11DeviceContext_RSSetViewports(ID3D11DeviceContext* This, UINT NumViewports, const D3D11_VIEWPORT *pViewports){
	//logger::info("Num viewports: {}", NumViewports);
	//take first viewport
	if (NumViewports > 0) {
		auto tfx = SkyrimTressFX::GetSingleton();
		auto scene = tfx->m_activeScene.scene.get();
		scene->m_currentViewport.TopLeftX = pViewports->TopLeftX;
		scene->m_currentViewport.TopLeftY = pViewports->TopLeftY;
		scene->m_currentViewport.Height = pViewports->Height;
		scene->m_currentViewport.Width = pViewports->Width;
		scene->m_currentViewport.MinDepth = pViewports->MinDepth;
		scene->m_currentViewport.MaxDepth = pViewports->MaxDepth;
	}
	(This->*ptrRSSetViewports)(NumViewports, pViewports);
}

//hack necessary to compute hair depth for shadows
//but prevent it from being used in other parts of render loop
bool catchNextResourceCopy = false;
ID3D11Resource* overrideResource = nullptr;
void hk_ID3D11Device_CopyResource(ID3D11DeviceContext* This, ID3D11Resource* pDstResource, ID3D11Resource* pSrcResource) {
	if (catchNextResourceCopy) {
        logger::info("overriding resource copy");
        (This->*ptrCopyResource)(pDstResource, overrideResource);
        catchNextResourceCopy = false;
    } else {
        (This->*ptrCopyResource)(pDstResource, pSrcResource);
    }
}

GUID WKPDID_D3DDebugObjectNameT = { 0x429b8c22, 0x9188, 0x4b0c, 0x87, 0x42, 0xac, 0xb0, 0xbf, 0x85, 0xc2, 0x00 };


typedef void (*LoadShaders_t)(BSGraphics::BSShader* shader, std::uintptr_t stream);
LoadShaders_t oLoadShaders;
bool          loadinit = false;

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
			/**(uintptr_t*)&ptrDrawIndexed = Detours::X64::DetourClassVTable(*(uintptr_t*)context, &hk_ID3D11DeviceContext_DrawIndexed, 12);
			*(uintptr_t*)&ptrDrawIndexedInstanced = Detours::X64::DetourClassVTable(*(uintptr_t*)context, &hk_ID3D11DeviceContext_DrawIndexedInstanced, 20);*/
			*(uintptr_t*)&ptrRSSetViewports = Detours::X64::DetourClassVTable(*(uintptr_t*)context, &hk_ID3D11DeviceContext_RSSetViewports, 44);
			*(uintptr_t*)&ptrCopyResource = Detours::X64::DetourClassVTable(*(uintptr_t*)context, &hk_ID3D11Device_CopyResource, 47);
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
		logger::info("Installed render startup hooks");
	}
};

void PatchD3D11()
{
	Hooks::Install();
}
