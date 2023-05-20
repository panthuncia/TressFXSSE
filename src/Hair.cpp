#include "Hair.h"
#include <sys/stat.h>
#include "SkyrimGPUInterface.h"
#include "TressFXLayouts.h"
// See TressFXLayouts.h
// By default, app allocates space for each of these, and TressFX uses it.
// These are globals, because there should really just be one instance.
TressFXLayouts* g_TressFXLayouts = 0;

Hair::Hair(AMD::TressFXAsset* asset, SkyrimGPUResourceManager* resourceManager, ID3D11DeviceContext* context, EI_StringHash name) {
	hairObject = new TressFXHairObject;
	hairEIResource = new EI_Resource;
	deviceContext = context;
	initialize(resourceManager);
	hairEIResource->srv = hairSRV;
	hairObject->Create(asset, (EI_Device*)resourceManager, (EI_CommandContextRef)context, name, hairEIResource);
	logger::info("Created hair object");
	hairs["hairTest"] = this;
}
void Hair::draw() {
	hairObject->DrawStrands((EI_CommandContextRef)deviceContext, *m_pBuildPSO);
}
void Hair::initialize(SkyrimGPUResourceManager* pManager) {
	//create texture and SRV (empty for now)
	D3D11_TEXTURE2D_DESC desc;
	desc.Width = 256;
	desc.Height = 256;
	desc.MipLevels = desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D11_USAGE_DYNAMIC;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	desc.MiscFlags = 0;
	pManager->device->CreateTexture2D(&desc, NULL, &hairTexture);
	pManager->device->CreateShaderResourceView(hairTexture, nullptr, &hairSRV);
	//compile effect
	const auto path = std::filesystem::current_path() /= "data\\Shaders\\TressFX\\oHair.fx"sv;
	struct stat buffer;
	if (stat(path.string().c_str(), &buffer) == 0) {
		logger::info("file exists");
	}
	else {
		logger::error("Effect file {} missing!", path.string());
		throw std::runtime_error("File not found");
	}
	logger::info("Compiling hair effect shader");
	pStrandEffect = ShaderCompiler::CompileAndRegisterEffectShader(path.wstring(), pManager->device);
	if (pStrandEffect->IsValid())
		logger::info("Effect is valid");
	else
		logger::info("Error: Effect is invalid!");
	/*D3DX11_EFFECT_DESC effectDesc;
	pStrandEffect->GetDesc(&effectDesc);
	logger::info("Number of variables: {}", effectDesc.GlobalVariables);
	logger::info("Number of constant buffers: {}", effectDesc.ConstantBuffers);
	logger::info("Number of constant buffers: {}", effectDesc.ConstantBuffers);
	for (uint16_t i = 0; i < effectDesc.ConstantBuffers; ++i) {
		auto var = pStrandEffect->GetConstantBufferByIndex(i);
		D3DX11_EFFECT_VARIABLE_DESC vardesc;
		var->GetDesc(&vardesc);
		logger::info("{}", vardesc.Name);
	}
	logger::info("Number of variables: {}", effectDesc.GlobalVariables);
	for (uint16_t i = 0; i < effectDesc.GlobalVariables; ++i) {
		auto var = pStrandEffect->GetVariableByIndex(i);
		D3DX11_EFFECT_VARIABLE_DESC vardesc;
		var->GetDesc(&vardesc);
		logger::info("{}", vardesc.Name);
	}*/

	// See TressFXLayouts.h
	// Global storage for layouts.

	//why? TFX sample does it
	if (g_TressFXLayouts != 0) {
		EI_Device* pDevice = (EI_Device*)pManager->device;
		DestroyAllLayouts(pDevice);

		delete g_TressFXLayouts;
	}

	m_pBuildPSO = GetPSO("TressFX2", pStrandEffect);
	logger::info("Got strand PSO");

	if (g_TressFXLayouts == 0)
		g_TressFXLayouts = new TressFXLayouts;

	EI_LayoutManagerRef renderStrandsLayoutManager = (EI_LayoutManagerRef&)*pStrandEffect;
	CreateRenderPosTanLayout2((EI_Device*)pManager->device, renderStrandsLayoutManager);
	logger::info("Created PosTanLayout");
	logger::info("srvs: {}", g_TressFXLayouts->pRenderPosTanLayout->srvs.size());
	logger::info("uavs: {}", g_TressFXLayouts->pRenderPosTanLayout->uavs.size());
	CreateRenderLayout2((EI_Device*)pManager->device, renderStrandsLayoutManager);
	logger::info("Created RenderLayout");
	CreatePPLLBuildLayout2((EI_Device*)pManager->device, renderStrandsLayoutManager);
	logger::info("Created PPLLBuildLayout");
	//CreateShortCutDepthsAlphaLayout2((EI_Device*)pManager->device, renderStrandsLayoutManager);
	//CreateShortCutFillColorsLayout2((EI_Device*)pManager->device, renderStrandsLayoutManager);

	//EI_LayoutManagerRef readLayoutManager = GetLayoutManagerRef(m_pHairResolveEffect);
	//CreatePPLLReadLayout2(pDevice, readLayoutManager);
	//CreateShortCutResolveDepthLayout2(pDevice, readLayoutManager);
	//CreateShortCutResolveColorLayout2(pDevice, readLayoutManager);
}
