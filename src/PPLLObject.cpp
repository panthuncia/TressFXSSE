#include "PPLLObject.h"
#include "ShaderCompiler.h"
#include "TressFXLayouts.h"
#include "TressFXPPLL.h"
#include "glm/ext/matrix_common.hpp"
#include "Util.h"
#include "Menu.h"

// This could instead be retrieved as a variable from the
// script manager, or passed as an argument.
static const size_t AVE_FRAGS_PER_PIXEL = 4;
static const size_t PPLL_NODE_SIZE = 16;

// See TressFXLayouts.h
// By default, app allocates space for each of these, and TressFX uses it.
// These are globals, because there should really just be one instance.
TressFXLayouts* g_TressFXLayouts = 0;

PPLLObject::PPLLObject() : m_Simulation(), m_SDFCollisionSystem()
{
	
}

PPLLObject::~PPLLObject() {
}

void PPLLObject::ReloadAllHairs()
{
	for (auto hair : m_hairs) {
		hair.second->Reload();
	}
	m_doReload = false;
	logger::info("Done reloading");
}

void PPLLObject::Draw() {
	//start draw
	//PrintAllD3D11DebugMessages(m_pManager->m_pDevice);
	logger::info("Starting TFX Draw");
	ID3D11DeviceContext* pContext;
	m_pManager->m_pDevice->GetImmediateContext(&pContext);
	m_pPPLL->Clear((EI_CommandContextRef)pContext);
	m_pPPLL->BindForBuild((EI_CommandContextRef)pContext);
	pContext->RSSetState(m_pWireframeRSState);

	//draw strands
	for (auto hair : m_hairs) {
		hair.second->Draw(pContext, m_pBuildPSO);
	}
	logger::info("End of TFX Draw Debug");
	m_pPPLL->DoneBuilding((EI_CommandContextRef)pContext);

	// TODO move this to a clear "after all pos and tan usage by rendering" place.
	
	//necessary?
	//UnbindUAVS();
	m_pPPLL->BindForRead((EI_CommandContextRef)pContext);
	//m_pFullscreenPass->Draw(m_pManager, m_pReadPSO);
	m_pPPLL->DoneReading((EI_CommandContextRef)pContext);

	//draw debug markers
	for (auto hair : m_hairs) {
		hair.second->DrawDebugMarkers();
	}
	//end draw
}

void PPLLObject::Simulate() {
	for (auto hair : m_hairs) {
		hair.second->UpdateVariables();
		hair.second->Simulate(m_pManager, &m_Simulation);
	}
}

void PPLLObject::UpdateVariables(){
	//setup variables
	DXGI_SWAP_CHAIN_DESC swapDesc;
	m_pManager->m_pSwapChain->GetDesc(&swapDesc);
	float4 vFragmentBufferSize;
	vFragmentBufferSize.x = (float)swapDesc.BufferDesc.Width;
	vFragmentBufferSize.y = (float)swapDesc.BufferDesc.Height;
	vFragmentBufferSize.z = (float)(swapDesc.BufferDesc.Width * swapDesc.BufferDesc.Height);
	vFragmentBufferSize.w = 0;
	m_pStrandEffect->GetVariableByName("nNodePoolSize")->AsScalar()->SetInt(m_nPPLLNodes);
	m_pQuadEffect->GetVariableByName("nNodePoolSize")->AsScalar()->SetInt(m_nPPLLNodes);
	m_pStrandEffect->GetVariableByName("vFragmentBufferSize")->AsVector()->SetFloatVector(&vFragmentBufferSize.x);
	m_pQuadEffect->GetVariableByName("vFragmentBufferSize")->AsVector()->SetFloatVector(&vFragmentBufferSize.x);
	//get view, projection, viewProj, and inverse viewProj matrix
	RE::NiCamera* playerCam = Util::GetPlayerNiCamera().get();
	auto          wtc = playerCam->worldToCam;
	//transpose rotation
	glm::mat worldToCamMat = glm::mat4({ wtc[0][0], wtc[1][0], wtc[2][0], wtc[0][3] },
		{ wtc[0][1], wtc[1][1], wtc[2][1], wtc[1][3] },
		{ wtc[0][2], wtc[1][2], wtc[2][2], wtc[2][3] },
		{ wtc[3][0], wtc[3][1], wtc[3][2], wtc[3][3] });

	RE::NiPoint3 translation = playerCam->world.translate;  //tps->translation;

	glm::vec3 cameraPos = Util::ToRenderScale(glm::vec3(translation.x, translation.y, translation.z));

	//projection matrix
	glm::mat4 projMatrix = Util::GetPlayerProjectionMatrix(playerCam->GetRuntimeData2().viewFrustum, swapDesc.BufferDesc.Width, swapDesc.BufferDesc.Height);

	//calculate view matrix from worldToCam
	auto projCalcInverse = glm::inverse(projMatrix);
	auto wtcInvProj = projCalcInverse * worldToCamMat;

	glm::mat4 camRotCalc = (glm::mat4({ wtcInvProj[0][0], wtcInvProj[0][1], worldToCamMat[0][2], 0 },
		{ wtcInvProj[1][0], wtcInvProj[1][1], worldToCamMat[1][2], 0 },
		{ wtcInvProj[2][0], wtcInvProj[2][1], worldToCamMat[2][2], 0 },
		{ 0, 0, 0, 1 }));
	auto      camRotCalcTransp = glm::transpose(camRotCalc);

	glm::vec3 eye = -cameraPos;
	glm::mat4 viewMatrix = glm::mat4({ camRotCalcTransp[0][0], camRotCalcTransp[0][1], camRotCalcTransp[0][2], glm::dot(eye, glm::vec3(camRotCalcTransp[0][0], camRotCalcTransp[0][1], camRotCalcTransp[0][2])) },
		{ camRotCalcTransp[1][0], camRotCalcTransp[1][1], camRotCalcTransp[1][2], glm::dot(eye, glm::vec3(camRotCalcTransp[1][0], camRotCalcTransp[1][1], camRotCalcTransp[1][2])) },
		{ camRotCalcTransp[2][0], camRotCalcTransp[2][1], camRotCalcTransp[2][2], glm::dot(eye, glm::vec3(camRotCalcTransp[2][0], camRotCalcTransp[2][1], camRotCalcTransp[2][2])) },
		{ 0, 0, 0, 1 });

	//move to DirectXMath
	auto projXMFloat = DirectX::XMFLOAT4X4(&projMatrix[0][0]);
	m_projXMMatrix = DirectX::XMMatrixSet(projXMFloat._11, projXMFloat._12, projXMFloat._13, projXMFloat._14, projXMFloat._21, projXMFloat._22, projXMFloat._23, projXMFloat._24, projXMFloat._31, projXMFloat._32, projXMFloat._33, projXMFloat._34, projXMFloat._41, projXMFloat._42, projXMFloat._43, projXMFloat._44);
	auto viewXMFloat = DirectX::XMFLOAT4X4(&viewMatrix[0][0]);
	m_viewXMMatrix = DirectX::XMMatrixSet(viewXMFloat._11, viewXMFloat._12, viewXMFloat._13, viewXMFloat._14, viewXMFloat._21, viewXMFloat._22, viewXMFloat._23, viewXMFloat._24, viewXMFloat._31, viewXMFloat._32, viewXMFloat._33, viewXMFloat._34, viewXMFloat._41, viewXMFloat._42, viewXMFloat._43, viewXMFloat._44);

	Menu::GetSingleton()->DrawMatrix(m_projXMMatrix, "TFX projection");
	Menu::GetSingleton()->DrawMatrix(m_viewXMMatrix, "TFX VP");

	//tps->thirdPersonCameraObj->world.rotate
	//view-projection matrix

	//auto tfxViewMatrix = DirectX::XMMatrixTranspose(viewXMMatrix);

	DirectX::XMMATRIX viewProjectionMatrix = m_projXMMatrix * m_viewXMMatrix;

	Menu::GetSingleton()->DrawMatrix(viewProjectionMatrix, "TFX VP");

	auto viewProjectionMatrixInverse = DirectX::XMMatrixInverse(nullptr, viewProjectionMatrix);
	m_pStrandEffect->GetVariableByName("g_mVP")->AsMatrix()->SetMatrix(reinterpret_cast<float*>(&viewProjectionMatrix));
	m_pStrandEffect->GetVariableByName("g_mInvViewProj")->AsMatrix()->SetMatrix(reinterpret_cast<float*>(&viewProjectionMatrixInverse));
	m_pStrandEffect->GetVariableByName("g_mView")->AsMatrix()->SetMatrix(reinterpret_cast<float*>(&m_viewXMMatrix));
	m_pStrandEffect->GetVariableByName("g_mProjection")->AsMatrix()->SetMatrix(reinterpret_cast<float*>(&projMatrix));
	//get camera position
	DirectX::XMVECTOR cameraPosVectorScaled = DirectX::XMVectorSet(cameraPos.x, cameraPos.y, cameraPos.z, 0);
	m_pStrandEffect->GetVariableByName("g_vEye")->AsVector()->SetFloatVector(reinterpret_cast<float*>(&cameraPosVectorScaled));
	//viewport
	DirectX::XMVECTOR viewportVector = DirectX::XMVectorSet(m_currentViewport.TopLeftX, m_currentViewport.TopLeftY, m_currentViewport.Width, m_currentViewport.Height);
	m_pStrandEffect->GetVariableByName("g_vViewport")->AsVector()->SetFloatVector(reinterpret_cast<float*>(&viewportVector));

}

void PPLLObject::Initialize() {
	logger::info("Create strand effect");
	std::vector<D3D_SHADER_MACRO> strand_defines = { { "SU_sRGB_TO_LINEAR", "2.2" }, { "SU_LINEAR_SPACE_LIGHTING", "" }, { "SM_CONST_BIAS", "0.000025" }, { "SU_CRAZY_IF", "[flatten] if" }, { "SU_MAX_LIGHTS", "20" }, { "KBUFFER_SIZE", "4" }, { "SU_LOOP_UNROLL", "[unroll]" }, { "SU_LINEAR_TO_sRGB", "0.4545454545" }, { "SU_3D_API_D3D11", "1" }, { NULL, NULL } };
	m_pStrandEffect = ShaderCompiler::CreateEffect("data\\Shaders\\TressFX\\oHair.fx"sv, m_pManager->m_pDevice, strand_defines);
	//printEffectVariables(m_pStrandEffect);
	logger::info("Create quad effect");
	std::vector<D3D_SHADER_MACRO> quad_defines = { { "SU_sRGB_TO_LINEAR", "2.2" }, { "SU_LINEAR_SPACE_LIGHTING", "" }, { "SM_CONST_BIAS", "0.000025" }, { "SU_CRAZY_IF", "[flatten] if" }, { "SU_MAX_LIGHTS", "20" }, { "KBUFFER_SIZE", "4" }, { "SU_LOOP_UNROLL", "[unroll]" }, { "SU_LINEAR_TO_sRGB", "0.4545454545" }, { "SU_3D_API_D3D11", "1" }, { NULL, NULL } };
	m_pQuadEffect = ShaderCompiler::CreateEffect("data\\Shaders\\TressFX\\qHair.fx"sv, m_pManager->m_pDevice, quad_defines);
	//printEffectVariables(m_pQuadEffect);
	logger::info("Create simulation effect");
	std::vector<D3D_SHADER_MACRO> macros;
	std::string                   s = std::to_string(AMD_TRESSFX_MAX_NUM_BONES);
	macros.push_back({ "AMD_TRESSFX_MAX_NUM_BONES", s.c_str() });
	m_pTressFXSimEffect = ShaderCompiler::CreateEffect("data\\Shaders\\TressFX\\cTressFXSimulation.fx"sv, m_pManager->m_pDevice);
	//printEffectVariables(m_pTressFXSimEffect);
	logger::info("Create collision effect");
	m_pTressFXSDFCollisionEffect = ShaderCompiler::CreateEffect("data\\Shaders\\TressFX\\cTressFXSDFCollision.fx"sv, m_pManager->m_pDevice);
	//printEffectVariables(m_pTressFXSDFCollisionEffect);

	// See TressFXLayouts.h
	// Global storage for layouts.

	//why? TFX sample does it
	if (g_TressFXLayouts != 0) {
		EI_Device* pDevice = (EI_Device*)m_pManager->m_pDevice;
		DestroyAllLayouts(pDevice);

		delete g_TressFXLayouts;
	}
	m_pBuildPSO = GetPSO("TressFX2", m_pStrandEffect);
	logger::info("Got strand PSO");
	m_pReadPSO = GetPSO("TressFX2", m_pQuadEffect);

	m_pFullscreenPass = new FullscreenPass(m_pManager);

	if (m_pPPLL) {
		logger::info("destroying old pll object");
		m_pPPLL->Destroy((EI_Device*)m_pManager->m_pDevice);
		logger::info("destroyed old pll object");
		delete m_pPPLL;
		m_pPPLL = nullptr;
	}
	if (m_pPPLL == nullptr) {
		m_pPPLL = new TressFXPPLL;
	}
	DXGI_SWAP_CHAIN_DESC swapChainDesc;
	m_pManager->m_pSwapChain->GetDesc(&swapChainDesc);
	m_nPPLLNodes = swapChainDesc.BufferDesc.Width * swapChainDesc.BufferDesc.Height * AVE_FRAGS_PER_PIXEL;
	m_pPPLL->Create((EI_Device*)m_pManager->m_pDevice, swapChainDesc.BufferDesc.Width, swapChainDesc.BufferDesc.Height, m_nPPLLNodes, PPLL_NODE_SIZE);
	logger::info("Created PLL object");

	//if (g_TressFXLayouts == 0)
	g_TressFXLayouts = new TressFXLayouts;

	EI_LayoutManagerRef renderStrandsLayoutManager = (EI_LayoutManagerRef&)*m_pStrandEffect;
	CreateRenderPosTanLayout2((EI_Device*)m_pManager->m_pDevice, renderStrandsLayoutManager);
	logger::info("Created PosTanLayout");
	logger::info("srvs: {}", g_TressFXLayouts->pRenderPosTanLayout->srvs.size());
	logger::info("uavs: {}", g_TressFXLayouts->pRenderPosTanLayout->uavs.size());
	CreateRenderLayout2((EI_Device*)m_pManager->m_pDevice, renderStrandsLayoutManager);
	logger::info("Created RenderLayout");
	CreatePPLLBuildLayout2((EI_Device*)m_pManager->m_pDevice, renderStrandsLayoutManager);
	logger::info("Created PPLLBuildLayout");
	//CreateShortCutDepthsAlphaLayout2((EI_Device*)pManager->device, renderStrandsLayoutManager);
	//CreateShortCutFillColorsLayout2((EI_Device*)pManager->device, renderStrandsLayoutManager);

	EI_LayoutManagerRef readLayoutManager = (EI_LayoutManagerRef&)*m_pQuadEffect;
	CreatePPLLReadLayout2((EI_Device*)m_pManager->m_pDevice, readLayoutManager);
	logger::info("Created PPLLReadLayout");
	//CreateShortCutResolveDepthLayout2(pDevice, readLayoutManager);
	//CreateShortCutResolveColorLayout2(pDevice, readLayoutManager);

	EI_LayoutManagerRef simLayoutManager = (EI_LayoutManagerRef&)*m_pTressFXSimEffect;
	CreateSimPosTanLayout2((EI_Device*)m_pManager->m_pDevice, simLayoutManager);
	logger::info("Created SimPosTanLayout");
	CreateSimLayout2((EI_Device*)m_pManager->m_pDevice, simLayoutManager);
	logger::info("Created SimLayout");

	EI_LayoutManagerRef applySDFManager = (EI_LayoutManagerRef&)*m_pTressFXSDFCollisionEffect;
	CreateApplySDFLayout2((EI_Device*)m_pManager->m_pDevice, applySDFManager);
	logger::info("Created ApplySDFLayout");

	EI_LayoutManagerRef sdfCollisionManager = (EI_LayoutManagerRef&)*m_pTressFXSDFCollisionEffect;
	CreateGenerateSDFLayout2((EI_Device*)m_pManager->m_pDevice, sdfCollisionManager);
	logger::info("Created GenerateSDFLayout");

	logger::info("Initialize simulation");
	m_Simulation.Initialize((EI_Device*)m_pManager->m_pDevice, simLayoutManager);
	logger::info("Initialize SDF collision");
	m_SDFCollisionSystem.Initialize((EI_Device*)m_pManager->m_pDevice, sdfCollisionManager);

	//create wireframe rasterizer state for testing
	D3D11_RASTERIZER_DESC rsState;
	rsState.FillMode = D3D11_FILL_WIREFRAME;
	rsState.CullMode = D3D11_CULL_NONE;
	rsState.FrontCounterClockwise = false;
	rsState.DepthBias = 0;
	rsState.DepthBiasClamp = 0;
	rsState.SlopeScaledDepthBias = 0;
	rsState.DepthClipEnable = false;
	rsState.ScissorEnable = false;
	rsState.MultisampleEnable = true;
	rsState.AntialiasedLineEnable = false;
	m_pManager->m_pDevice->CreateRasterizerState(&rsState, &m_pWireframeRSState);
}
