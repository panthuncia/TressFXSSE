#include "Hair.h"
#include <sys/stat.h>
#include "SkyrimGPUInterface.h"
#include "TressFXLayouts.h"
#include "TressFXPPLL.h"

// This could instead be retrieved as a variable from the
// script manager, or passed as an argument.
static const size_t AVE_FRAGS_PER_PIXEL = 4;
static const size_t PPLL_NODE_SIZE = 16;

// See TressFXLayouts.h
// By default, app allocates space for each of these, and TressFX uses it.
// These are globals, because there should really just be one instance.
TressFXLayouts* g_TressFXLayouts = 0;

Hair::Hair(AMD::TressFXAsset* asset, SkyrimGPUResourceManager* resourceManager, ID3D11DeviceContext* context, EI_StringHash name) {
	m_hairObject = new TressFXHairObject;
	m_hairEIResource = new EI_Resource;
	initialize(resourceManager);
	m_hairEIResource->srv = m_hairSRV;
	m_hairObject->Create(asset, (EI_Device*)resourceManager, (EI_CommandContextRef)context, name, m_hairEIResource);
	logger::info("Created hair object");
	hairs["hairTest"] = this;
}
void Hair::draw() {
	ID3D11DeviceContext* pContext;
	m_pManager->m_pDevice->GetImmediateContext(&pContext);

	m_hairObject->DrawStrands((EI_CommandContextRef)pContext, *m_pBuildPSO);
}
ID3DX11Effect* Hair::create_effect(std::string_view filePath){
	//compile effect
	const auto  path = std::filesystem::current_path() /= filePath;
	struct stat buffer;
	if (stat(path.string().c_str(), &buffer) == 0) {
		logger::info("file exists");
	} else {
		logger::error("Effect file {} missing!", path.string());
		throw std::runtime_error("File not found");
	}
	logger::info("Compiling hair effect shader");
	ID3DX11Effect* pEffect = ShaderCompiler::CompileAndRegisterEffectShader(path.wstring(), m_pManager->m_pDevice);
	if (pEffect->IsValid())
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
	return pEffect;
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
	m_pManager = pManager;
	pManager->m_pDevice->CreateTexture2D(&desc, NULL, &m_hairTexture);
	pManager->m_pDevice->CreateShaderResourceView(m_hairTexture, nullptr, &m_hairSRV);
	//compile effects
	logger::info("Create strand effect");
	m_pStrandEffect = create_effect("data\\Shaders\\TressFX\\oHair.fx"sv);
	logger::info("Create quad effect");
	m_pQuadEffect = create_effect("data\\Shaders\\TressFX\\qHair.fx"sv);
	// See TressFXLayouts.h
	// Global storage for layouts.

	//why? TFX sample does it
	if (g_TressFXLayouts != 0) {
		EI_Device* pDevice = (EI_Device*)pManager->m_pDevice;
		DestroyAllLayouts(pDevice);

		delete g_TressFXLayouts;
	}
	m_pBuildPSO = GetPSO("TressFX2", m_pStrandEffect);
	logger::info("Got strand PSO");
	m_pReadPSO = GetPSO("TressFX2", m_pQuadEffect);

	
	if (m_pPPLL) {
		logger::info("destroying old pll object");
		m_pPPLL->Destroy((EI_Device*)pManager->m_pDevice);
		logger::info("destroyed old pll object");
		delete m_pPPLL;
		m_pPPLL = nullptr;
	}
	if (m_pPPLL == nullptr) {
		m_pPPLL = new TressFXPPLL;
	}
	DXGI_SWAP_CHAIN_DESC swapChainDesc;
	pManager->m_pSwapChain->GetDesc(&swapChainDesc);
	m_nPPLLNodes = swapChainDesc.BufferDesc.Width * swapChainDesc.BufferDesc.Height * AVE_FRAGS_PER_PIXEL;
	m_pPPLL->Create((EI_Device*)pManager->m_pDevice, swapChainDesc.BufferDesc.Width, swapChainDesc.BufferDesc.Height, m_nPPLLNodes, PPLL_NODE_SIZE);
	logger::info("Created PLL object");

	if (g_TressFXLayouts == 0)
		g_TressFXLayouts = new TressFXLayouts;

	EI_LayoutManagerRef renderStrandsLayoutManager = (EI_LayoutManagerRef&)*m_pStrandEffect;
	CreateRenderPosTanLayout2((EI_Device*)pManager->m_pDevice, renderStrandsLayoutManager);
	logger::info("Created PosTanLayout");
	logger::info("srvs: {}", g_TressFXLayouts->pRenderPosTanLayout->srvs.size());
	logger::info("uavs: {}", g_TressFXLayouts->pRenderPosTanLayout->uavs.size());
	CreateRenderLayout2((EI_Device*)pManager->m_pDevice, renderStrandsLayoutManager);
	logger::info("Created RenderLayout");
	CreatePPLLBuildLayout2((EI_Device*)pManager->m_pDevice, renderStrandsLayoutManager);
	logger::info("Created PPLLBuildLayout");
	//CreateShortCutDepthsAlphaLayout2((EI_Device*)pManager->device, renderStrandsLayoutManager);
	//CreateShortCutFillColorsLayout2((EI_Device*)pManager->device, renderStrandsLayoutManager);

	EI_LayoutManagerRef readLayoutManager = (EI_LayoutManagerRef&)*m_pQuadEffect;
	CreatePPLLReadLayout2((EI_Device*)pManager->m_pDevice, readLayoutManager);
	logger::info("Created PPLLReadLayout");
	//CreateShortCutResolveDepthLayout2(pDevice, readLayoutManager);
	//CreateShortCutResolveColorLayout2(pDevice, readLayoutManager);
}
