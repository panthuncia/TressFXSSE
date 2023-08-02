#include "PPLLObject.h"
#include "ShaderCompiler.h"
#include "TressFXLayouts.h"
#include "TressFXPPLL.h"
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

void PrintAllD3D11DebugMessages(ID3D11Device* d3dDevice);

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
void PPLLObject::DrawShadows() {
	
}
void PPLLObject::Draw()
{
	m_gameLoaded = true;
	//draw shadows
	DrawShadows();
	//start draw
	//PrintAllD3D11DebugMessages(m_pManager->m_pDevice);
	logger::info("Starting TFX Draw");
	ID3D11DeviceContext* pContext;
	m_pManager->m_pDevice->GetImmediateContext(&pContext);
	m_pPPLL->Clear((EI_CommandContextRef)pContext);
	m_pPPLL->BindForBuild((EI_CommandContextRef)pContext);
	//pContext->RSSetState(m_pWireframeRSState);
	//store variable states
	float originalBlendFactor;
	UINT originalSampleMask;
	ID3D11BlendState* originalBlendState;
	pContext->OMGetBlendState(&originalBlendState, &originalBlendFactor, &originalSampleMask);
	ID3D11DepthStencilState* originalDepthStencilState;
	UINT                     originalStencilRef;
	pContext->OMGetDepthStencilState(&originalDepthStencilState, &originalStencilRef);
	ID3D11RasterizerState* originalRSState;
	pContext->RSGetState(&originalRSState);
	ID3D11DepthStencilView* originalDepthStencil;
	ID3D11RenderTargetView* originalRenderTarget;
	pContext->OMGetRenderTargets(1, &originalRenderTarget, &originalDepthStencil);

	//set new states
	pContext->OMSetBlendState(m_pPPLLBuildBlendState, &originalBlendFactor, 0x000000FF);
	pContext->OMSetDepthStencilState(m_pPPLLBuildDepthStencilState, originalStencilRef);
	pContext->RSSetState(m_pPPLLBuildRasterizerState);

	//draw strands
	for (auto hair : m_hairs) {
		hair.second->Draw(pContext, m_pBuildPSO);
	}
	logger::info("End of TFX Draw Debug");
	m_pPPLL->DoneBuilding((EI_CommandContextRef)pContext);

	// TODO move this to a clear "after all pos and tan usage by rendering" place.
	
	//necessary?
	UnbindUAVs(pContext);
	//PrintAllD3D11DebugMessages(m_pManager->m_pDevice);
	logger::info("Bind for read");
	m_pPPLL->BindForRead((EI_CommandContextRef)pContext);
	pContext->OMSetBlendState(m_pPPLLReadBlendState, &originalBlendFactor, 0x000000FF);
	pContext->OMSetDepthStencilState(m_pPPLLReadDepthStencilState, originalStencilRef);
	pContext->RSSetState(m_pPPLLReadRasterizerState);
	pContext->OMSetRenderTargets(1, &originalRenderTarget, nullptr);
	m_pFullscreenPass->Draw(m_pManager, m_pReadPSO);
	//PrintAllD3D11DebugMessages(m_pManager->m_pDevice);
	//logger::info("End of read debug");
	m_pPPLL->DoneReading((EI_CommandContextRef)pContext);
	
	//reset states
	pContext->OMSetBlendState(originalBlendState, &originalBlendFactor, originalSampleMask);
	pContext->OMSetDepthStencilState(originalDepthStencilState, originalStencilRef);
	pContext->OMSetDepthStencilState(originalDepthStencilState, originalStencilRef);
	pContext->RSSetState(originalRSState);
	pContext->OMSetRenderTargets(1, &originalRenderTarget, originalDepthStencil);

	//draw debug markers
	//for (auto hair : m_hairs) {
	//	hair.second->DrawDebugMarkers();
	//}
	RE::NiCamera* playerCam = Util::GetPlayerNiCamera().get();
	auto          translation = Util::ToRenderScale(glm::vec3(playerCam->world.translate.x, playerCam->world.translate.y, playerCam->world.translate.z));
	RE::NiMatrix3 rotation = playerCam->world.rotate;

	auto cameraTrans = DirectX::XMMatrixTranspose(DirectX::XMMatrixTranslation(translation.x, translation.y, translation.z));
	auto cameraRot = DirectX::XMMatrixSet(rotation.entry[0][0], rotation.entry[0][1], rotation.entry[0][2], 0,
		rotation.entry[1][0], rotation.entry[1][1], rotation.entry[1][2], 0,
		rotation.entry[2][0], rotation.entry[2][1], rotation.entry[2][2], 0,
		0, 0, 0, 1);
	m_cameraWorld = XMMatrixMultiply(cameraTrans, cameraRot);
	//MarkerRender::GetSingleton()->DrawWorldAxes(m_cameraWorld, m_viewXMMatrix, m_projXMMatrix);
	logger::info("Drawing {} lights", m_lightPositions.size());
	//MarkerRender::GetSingleton()->DrawMarkers(m_lightPositions, m_viewXMMatrix, m_projXMMatrix);
	//auto&            shaderState = RE::BSShaderManager::State::GetSingleton();
	//RE::NiTransform& dalcTransform = shaderState.directionalAmbientTransform;
	//Menu::GetSingleton()->DrawNiTransform(dalcTransform, "ambient transform");
	//end draw
}

void PPLLObject::Simulate() {
	for (auto hair : m_hairs) {
		hair.second->UpdateVariables(m_gravityMagnitude);
		hair.second->Simulate(m_pManager, &m_Simulation);
	}
}

void PPLLObject::UpdateVariables()
{
	//if positional sliders changed, update relevant hair
	auto menu = Menu::GetSingleton();
	if (menu->offsetSlidersUpdated) {
		float x = menu->xSliderValue;
		float y = menu->ySliderValue;
		float z = menu->zSliderValue;
		float scale = menu->sSliderValue;
		logger::info("Updating offsets: {}, X: {}, Y: {}, Z: {} Scale: {}", menu->activeHairs[menu->selectedHair], x, y, z, scale);
		PPLLObject::GetSingleton()->m_hairs[menu->activeHairs[menu->selectedHair]]->UpdateOffsets(x, y, z, scale);
	}
	//update hair parameters
	//TODO: make not stupid
	PPLLObject::GetSingleton()->m_hairs[menu->activeHairs[menu->selectedHair]]->SetRenderingAndSimParameters(menu->fiberRadiusSliderValue, menu->fiberSpacingSliderValue, menu->fiberRatioSliderValue, menu->kdSliderValue, menu->ks1SliderValue, menu->ex1SliderValue, menu->ks2SliderValue, menu->ex2SliderValue, menu->localConstraintsIterationsSlider, menu->lengthConstraintsIterationsSlider, menu->localConstraintsStiffnessSlider, menu->globalConstraintsStiffnessSlider, menu->globalConstraintsRangeSlider, menu->dampingSlider, menu->vspAmountSlider, menu->vspAccelThresholdSlider, menu->hairOpacitySlider, menu->hairShadowAlphaSlider, menu->thinTipCheckbox);
	m_gravityMagnitude = Menu::GetSingleton()->gravityMagnitudeSlider;
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

	//Menu::GetSingleton()->DrawMatrix(m_projXMMatrix, "TFX projection");
	//Menu::GetSingleton()->DrawMatrix(m_viewXMMatrix, "TFX VP");

	//tps->thirdPersonCameraObj->world.rotate
	//view-projection matrix

	//auto tfxViewMatrix = DirectX::XMMatrixTranspose(viewXMMatrix);

	DirectX::XMMATRIX viewProjectionMatrix = m_projXMMatrix * m_viewXMMatrix;

	//Menu::GetSingleton()->DrawMatrix(viewProjectionMatrix, "TFX VP");

	auto viewProjectionMatrixInverse = DirectX::XMMatrixInverse(nullptr, viewProjectionMatrix);

	//strand effect vars
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

	//quad effect vars
	m_pQuadEffect->GetVariableByName("g_mInvViewProj")->AsMatrix()->SetMatrix(reinterpret_cast<float*>(&viewProjectionMatrixInverse));
	m_pQuadEffect->GetVariableByName("g_vViewport")->AsVector()->SetFloatVector(reinterpret_cast<float*>(&viewportVector));
	m_pQuadEffect->GetVariableByName("g_vEye")->AsVector()->SetFloatVector(reinterpret_cast<float*>(&cameraPosVectorScaled));

	//TODO: shadows
	m_pQuadEffect->GetVariableByName("bEnableShadows")->AsScalar()->SetBool(false);

	//lights

	/*uniform int    nNumLights;
	uniform int    nLightShape[SU_MAX_LIGHTS];
	uniform int    nLightIndex[SU_MAX_LIGHTS];
	uniform float  fLightIntensity[SU_MAX_LIGHTS];
	uniform float3 vLightPosWS[SU_MAX_LIGHTS];
	uniform float3 vLightDirWS[SU_MAX_LIGHTS];
	uniform float3 vLightColor[SU_MAX_LIGHTS];
	uniform float3 vLightConeAngles[SU_MAX_LIGHTS];
	uniform float3 vLightScaleWS[SU_MAX_LIGHTS];
	uniform float4 vLightParams[SU_MAX_LIGHTS];
	uniform float4 vLightOrientationWS[SU_MAX_LIGHTS];*/

	m_pQuadEffect->GetVariableByName("nNumLights")->AsScalar()->SetInt(m_nNumLights);
	m_pQuadEffect->GetVariableByName("nLightShape")->AsScalar()->SetIntArray(m_nLightShape, 0, m_nNumLights);
	m_pQuadEffect->GetVariableByName("nLightIndex")->AsScalar()->SetIntArray(m_nLightIndex, 0, m_nNumLights);
	m_pQuadEffect->GetVariableByName("fLightIntensity")->AsScalar()->SetFloatArray(m_fLightIntensity, 0, m_nNumLights);
	m_pQuadEffect->GetVariableByName("vLightPosWS")->AsVector()->SetFloatVectorArray(reinterpret_cast<float*>(m_vLightPosWS), 0, m_nNumLights);
	m_pQuadEffect->GetVariableByName("vLightDirWS")->AsVector()->SetFloatVectorArray(reinterpret_cast<float*>(m_vLightDirWS), 0, m_nNumLights);
	m_pQuadEffect->GetVariableByName("vLightColor")->AsVector()->SetFloatVectorArray(reinterpret_cast<float*>(m_vLightColor), 0, m_nNumLights);
	m_pQuadEffect->GetVariableByName("vLightConeAngles")->AsVector()->SetFloatVectorArray(reinterpret_cast<float*>(m_vLightConeAngles), 0, m_nNumLights);
	m_pQuadEffect->GetVariableByName("vLightScaleWS")->AsVector()->SetFloatVectorArray(reinterpret_cast<float*>(m_vLightScaleWS), 0, m_nNumLights);
	m_pQuadEffect->GetVariableByName("vLightParams")->AsVector()->SetFloatVectorArray(reinterpret_cast<float*>(m_vLightParams), 0, m_nNumLights);
	m_pQuadEffect->GetVariableByName("vLightOrientationWS")->AsVector()->SetFloatVectorArray(reinterpret_cast<float*>(m_vLightOrientationWS), 0, m_nNumLights);

	//???
	m_pQuadEffect->GetVariableByName("fLightScale")->AsScalar()->SetFloat(1.0f);
	auto& shaderState = RE::BSShaderManager::State::GetSingleton();
	RE::NiTransform& dalcTransform = shaderState.directionalAmbientTransform;
	DirectX::XMFLOAT3X4 ambientTransform;
	Util::StoreTransform3x4NoScale(ambientTransform, dalcTransform);
	DirectX::XMMATRIX ambientTransformMatrix = DirectX::XMLoadFloat3x4(&ambientTransform);
	float             ambientScale = Menu::GetSingleton()->ambientLightingAmount;
	DirectX::XMVECTOR ambientColor = DirectX::XMVectorSet(dalcTransform.translate.x*ambientScale, dalcTransform.translate.y*ambientScale, dalcTransform.translate.z*ambientScale, 1);
	m_pQuadEffect->GetVariableByName("mDirectionalAmbient")->AsMatrix()->SetMatrix(reinterpret_cast<float*>(&ambientTransformMatrix));
	m_pQuadEffect->GetVariableByName("vAmbientColor")->AsVector()->SetFloatVector(reinterpret_cast<float*>(&ambientColor));

	m_pQuadEffect->GetVariableByName("vSunlightColor")->AsVector()->SetFloatVector(reinterpret_cast<float*>(&m_vSunlightColor));
	m_pQuadEffect->GetVariableByName("vSunlightDir")->AsVector()->SetFloatVector(reinterpret_cast<float*>(&m_vSunlightDir));
	m_pQuadEffect->GetVariableByName("vSunlightParams")->AsVector()->SetFloatVector(reinterpret_cast<float*>(&m_vSunlightParams));
}
void PPLLObject::UpdateLights() {
	m_nNumLights = 0;
	m_lightPositions.clear();
	auto accumulator = RE::BSGraphics::BSShaderAccumulator::GetCurrentAccumulator();

	//auto shadowSceneNode = RE::BSShaderManager::State::GetSingleton().shadowSceneNode[0];
	auto shadowSceneNode = accumulator->GetRuntimeData().activeShadowSceneNode;
	//auto  state = RE::BSGraphics::RendererShadowState::GetSingleton();
	auto& runtimeData = shadowSceneNode->GetRuntimeData();
	auto  menu = Menu::GetSingleton();
	logger::info("Active shadow lights: {}", runtimeData.activeShadowLights.size());
	logger::info("num active lights: {}", std::size(runtimeData.activePointLights));
	for (auto& e : runtimeData.activePointLights) {
		if (auto bsLight = e.get()) {
			if (auto niLight = bsLight->light.get()) {
				////only ambients for now
				//if (!bsLight->ambientLight) {
				//	continue;
				//}
				logger::info("Found point light");
				m_nNumLights += 1;

				//pos
				RE::NiPoint3 pos = niLight->world.translate;
				logger::info("Light position: {}, {}, {}", pos.x, pos.y, pos.z);
				m_vLightPosWS[m_nNumLights - 1] = Util::ToRenderScale(glm::vec3(pos.x, pos.y, pos.z));

				//color (rgb?)
				auto color = niLight->diffuse;
				logger::info("Light color: {}, {}, {}", color.red, color.green, color.blue);
				m_vLightColor[m_nNumLights - 1] = glm::vec3(color.red, color.green, color.blue);

				//???
				m_fLightIntensity[m_nNumLights - 1] = bsLight->lodDimmer;

				//???
				m_nLightShape[m_nNumLights - 1] = 1;

				//point light
				m_vLightParams[m_nNumLights - 1].x = 0;
				//z is diffuse, w is specular
				//why is this in the light and not material?
				m_vLightParams[m_nNumLights - 1].z = menu->pointLightDiffuseAmount;
				m_vLightParams[m_nNumLights - 1].w = menu->pointLightSpecularAmount;
				
				//scale, always 1 for now
				m_vLightScaleWS[m_nNumLights - 1] = glm::vec3(1.0);

				Menu::GetSingleton()->DrawVector3(m_vLightPosWS[m_nNumLights - 1], "light pos: ");
				Menu::GetSingleton()->DrawVector3(m_vLightColor[m_nNumLights - 1], "light color: ");
				Menu::GetSingleton()->DrawFloat(bsLight->lodDimmer, "lod dimmer: ");

				auto lightPos = Util::ToRenderScale(glm::vec3(niLight->world.translate.x, niLight->world.translate.y, niLight->world.translate.z));
				auto lightRot = niLight->world.rotate;

				//auto bonePos = m_bones[i]->world.translate;
				//auto boneRot = m_bones[i]->world.rotate;

				auto translation = DirectX::XMMatrixTranspose(DirectX::XMMatrixTranslation(lightPos.x, lightPos.y, lightPos.z));
				auto rotation = DirectX::XMMATRIX(lightRot.entry[0][0], lightRot.entry[0][1], lightRot.entry[0][2], 0, lightRot.entry[1][0], lightRot.entry[1][1], lightRot.entry[1][2], 0, lightRot.entry[2][0], lightRot.entry[2][1], lightRot.entry[2][2], 0, 0, 0, 0, 1);
				auto transform = translation * rotation;
				//Menu::GetSingleton()->DrawMatrix(transform, "bone");
				m_lightPositions.push_back(transform);
			}
		}
	}
	//sunlight
	auto sunLight = skyrim_cast<RE::NiDirectionalLight*>(accumulator->GetRuntimeData().activeShadowSceneNode->GetRuntimeData().sunLight->light.get());
	if (sunLight) {
		auto imageSpaceManager = RE::ImageSpaceManager::GetSingleton();

		float sunlightScale =  imageSpaceManager->data.baseData.hdr.sunlightScale * sunLight->GetLightRuntimeData().fade;

		m_vSunlightColor = glm::vec3(sunLight->GetLightRuntimeData().diffuse.red*sunlightScale, sunLight->GetLightRuntimeData().diffuse.green*sunlightScale, sunLight->GetLightRuntimeData().diffuse.blue*sunlightScale);

		auto& direction = sunLight->GetWorldDirection();
		m_vSunlightDir = glm::vec3(direction.x, direction.y, direction.z);
		m_vSunlightParams = glm::vec4(0, 0, menu->sunLightDiffuseAmount, menu->sunLightSpecularAmount);
	}
	//sky light?
	bool skyLight = true;
	if (auto sky = RE::Sky::GetSingleton()) {
		skyLight = sky->mode.get() == RE::Sky::Mode::kFull;
	}
}
void PPLLObject::Initialize()
{
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
	m_pWireframeRSState = Util::CreateRasterizerState(m_pManager->m_pDevice, D3D11_FILL_WIREFRAME, D3D11_CULL_NONE, false, 0, 0, 0, false, false, true, false);

	//create blend states
	m_pPPLLBuildBlendState = Util::CreateBlendState(m_pManager->m_pDevice, false, false, false, D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_OP_ADD, D3D11_BLEND_OP_ADD, D3D11_BLEND_INV_SRC_ALPHA, D3D11_BLEND_INV_SRC_ALPHA, 0, D3D11_BLEND_SRC_ALPHA);
	m_pPPLLReadBlendState = Util::CreateBlendState(m_pManager->m_pDevice, false, false, true, D3D11_BLEND_ONE, D3D11_BLEND_OP_ADD, D3D11_BLEND_OP_ADD, D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_ZERO, 0b00001111, D3D11_BLEND_ZERO);
	logger::info("Created blend states");

	//depth stencil states
	m_pPPLLBuildDepthStencilState = Util::CreateDepthStencilState(m_pManager->m_pDevice, true, D3D11_DEPTH_WRITE_MASK_ZERO, D3D11_COMPARISON_LESS_EQUAL, true, 0b11111111, 0b11111111, D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_INCR_SAT, D3D11_COMPARISON_ALWAYS);
	m_pPPLLReadDepthStencilState = Util::CreateDepthStencilState(m_pManager->m_pDevice, false, D3D11_DEPTH_WRITE_MASK_ZERO, D3D11_COMPARISON_LESS_EQUAL, true, 0b11111111, 0, D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_COMPARISON_LESS);
	logger::info("Created depth stencil states");
	
	//rasterizer states
	m_pPPLLBuildRasterizerState = Util::CreateRasterizerState(m_pManager->m_pDevice, D3D11_FILL_SOLID, D3D11_CULL_BACK, true, 0, 0.0f, 0.0f, true, true, false, false);
	m_pPPLLReadRasterizerState = Util::CreateRasterizerState(m_pManager->m_pDevice, D3D11_FILL_SOLID, D3D11_CULL_NONE, true, 0, 0.0f, 0.0f, true, true, false, false);

	//create states for shadow mapping
	m_pShadowBlendState = Util::CreateBlendState(m_pManager->m_pDevice, false, false, false, D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_OP_ADD, D3D11_BLEND_OP_ADD, D3D11_BLEND_INV_SRC_ALPHA, D3D11_BLEND_INV_SRC_ALPHA, 0, D3D11_BLEND_SRC_ALPHA);
	m_pShadowDepthStencilState = Util::CreateDepthStencilState(m_pManager->m_pDevice, true, D3D11_DEPTH_WRITE_MASK_ALL, D3D11_COMPARISON_LESS_EQUAL, false, 0b11111111, 0b11111111, D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_COMPARISON_EQUAL);
	m_pShadowRasterizerState = Util::CreateRasterizerState(m_pManager->m_pDevice, D3D11_FILL_SOLID, D3D11_CULL_BACK, true, 0, 0.0f, 0.0f, true, true, false, false);

}
