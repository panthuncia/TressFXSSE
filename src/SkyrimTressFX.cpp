#include "SkyrimTressFX.h"
#include "glm/gtc/matrix_transform.hpp"
#include "Util.h"
#include "SkyrimGPUResourceManager.h"
using json = nlohmann::json;
// This could instead be retrieved as a variable from the
// script manager, or passed as an argument.
static const size_t AVE_FRAGS_PER_PIXEL = 12;
static const size_t PPLL_NODE_SIZE = 16;

SkyrimTressFX::~SkyrimTressFX()
{
}

void SkyrimTressFX::OnCreate(int width, int height)
{
	m_nScreenHeight = height;
	m_nScreenWidth = width;
	m_nPPLLNodes = m_nScreenWidth * m_nScreenHeight * AVE_FRAGS_PER_PIXEL;

	// TODO Why?
	DestroyLayouts();

	logger::info("initialize layouts");
	InitializeLayouts();

	GetDevice()->OnCreate();

	logger::info("Create PPLL");
	m_pPPLL.reset(new TressFXPPLL);
	logger::info("Create ShortCut");
	m_pShortCut.reset(new TressFXShortCut);
	logger::info("Create Simulation");
	m_pSimulation.reset(new Simulation);
	m_activeScene.scene.reset(new EI_Scene);

	const EI_ResourceFormat FormatArray[] = { GetDevice()->GetColorBufferFormat(), GetDevice()->GetDepthBufferFormat() };
	// Create a debug render pass
	{
		const EI_AttachmentParams AttachmentParams[] = { { EI_RenderPassFlags::load | EI_RenderPassFlags::Store },
			{ EI_RenderPassFlags::Depth | EI_RenderPassFlags::load | EI_RenderPassFlags::Store } };

		m_pDebugRenderTargetSet = GetDevice()->CreateRenderTargetSet(FormatArray, 2, AttachmentParams, nullptr);
	}

	LoadScene();

	m_activeScene.viewConstantBuffer.CreateBufferResource("viewConstants");
	EI_BindSetDescription set = { { m_activeScene.viewConstantBuffer.GetBufferResource() } };
	m_activeScene.viewBindSet = GetDevice()->CreateBindSet(GetViewLayout(), set);

	m_activeScene.shadowViewConstantBuffer.CreateBufferResource("shadowViewConstants");
	EI_BindSetDescription shadowSet = { { m_activeScene.shadowViewConstantBuffer.GetBufferResource() } };
	m_activeScene.shadowViewBindSet = GetDevice()->CreateBindSet(GetViewLayout(), shadowSet);

	m_activeScene.lightConstantBuffer.CreateBufferResource("LightConstants");
}

void SkyrimTressFX::Simulate(double fTime, bool bUpdateCollMesh, bool bSDFCollisionResponse)
{
	SimulationContext ctx;
	ctx.hairStrands.resize(m_activeScene.objects.size());
	ctx.collisionMeshes.resize(m_activeScene.collisionMeshes.size());
	for (size_t i = 0; i < m_activeScene.objects.size(); ++i) {
		ctx.hairStrands[i] = m_activeScene.objects[i].hairStrands.get();
	}
	for (size_t i = 0; i < m_activeScene.collisionMeshes.size(); ++i) {
		ctx.collisionMeshes[i] = m_activeScene.collisionMeshes[i].get();
	}
	m_pSimulation->StartSimulation(fTime, ctx, bUpdateCollMesh, bSDFCollisionResponse);
}

//this function has to be executed once at a specific location in render loop
void SkyrimTressFX::CreateRenderResources() {
	m_gameLoaded = true;
	//get current render target and depth stencil
	ID3D11RenderTargetView* pRTV;
	ID3D11DepthStencilView* pDSV;
	logger::info("RTV address: {}", Util::ptr_to_string(pRTV));
	logger::info("DSV address: {}", Util::ptr_to_string(pDSV));
	logger::info("Getting game RTV and DSV");
	SkyrimGPUResourceManager::GetInstance()->m_pContext->OMGetRenderTargets(1, &pRTV, &pDSV);
	GetDevice()->CreateColorAndDepthResources(pRTV, pDSV);
	logger::info("Creating OIT resources");
	RecreateSizeDependentResources();
}

void SkyrimTressFX::Update(){

	RE::NiCamera* playerCam = Util::GetPlayerNiCamera().get();
	auto          wtc = playerCam->worldToCam;
	//transpose rotation
	glm::mat worldToCamMat = glm::mat4({ wtc[0][0], wtc[1][0], wtc[2][0], wtc[0][3] },
		{ wtc[0][1], wtc[1][1], wtc[2][1], wtc[1][3] },
		{ wtc[0][2], wtc[1][2], wtc[2][2], wtc[2][3] },
		{ wtc[3][0], wtc[3][1], wtc[3][2], wtc[3][3] });

	RE::NiPoint3 translation = playerCam->world.translate;  //tps->translation;

	glm::vec3 cameraPos = Util::ToRenderScale(glm::vec3(translation.x, translation.y, translation.z));

	DXGI_SWAP_CHAIN_DESC swapDesc;
	SkyrimGPUResourceManager::GetInstance()->m_pSwapChain->GetDesc(&swapDesc);
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

	AMD::float4x4 amdProjMatrix;
	amdProjMatrix.m[0] = projMatrix[0][0];
	amdProjMatrix.m[1] = projMatrix[0][1];
	amdProjMatrix.m[2] = projMatrix[0][2];
	amdProjMatrix.m[3] = projMatrix[0][3];
	amdProjMatrix.m[4] = projMatrix[1][0];
	amdProjMatrix.m[5] = projMatrix[1][1];
	amdProjMatrix.m[6] = projMatrix[1][2];
	amdProjMatrix.m[7] = projMatrix[1][3];
	amdProjMatrix.m[8] = projMatrix[2][0];
	amdProjMatrix.m[9] = projMatrix[2][1];
	amdProjMatrix.m[10] = projMatrix[2][2];
	amdProjMatrix.m[11] = projMatrix[2][3];
	amdProjMatrix.m[12] = projMatrix[3][0];
	amdProjMatrix.m[13] = projMatrix[3][1];
	amdProjMatrix.m[14] = projMatrix[3][2];
	amdProjMatrix.m[15] = projMatrix[3][3];
	m_activeScene.scene.get()->m_projMatrix = amdProjMatrix;

	AMD::float4x4 amdViewMatrix;
	amdViewMatrix.m[0] = viewMatrix[0][0];
	amdViewMatrix.m[1] = viewMatrix[0][1];
	amdViewMatrix.m[2] = viewMatrix[0][2];
	amdViewMatrix.m[3] = viewMatrix[0][3];
	amdViewMatrix.m[4] = viewMatrix[1][0];
	amdViewMatrix.m[5] = viewMatrix[1][1];
	amdViewMatrix.m[6] = viewMatrix[1][2];
	amdViewMatrix.m[7] = viewMatrix[1][3];
	amdViewMatrix.m[8] = viewMatrix[2][0];
	amdViewMatrix.m[9] = viewMatrix[2][1];
	amdViewMatrix.m[10] = viewMatrix[2][2];
	amdViewMatrix.m[11] = viewMatrix[2][3];
	amdViewMatrix.m[12] = viewMatrix[3][0];
	amdViewMatrix.m[13] = viewMatrix[3][1];
	amdViewMatrix.m[14] = viewMatrix[3][2];
	amdViewMatrix.m[15] = viewMatrix[3][3];
	m_activeScene.scene.get()->m_viewMatrix = amdViewMatrix;
	
	auto          viewProjMatrix = glm::transpose(viewMatrix * projMatrix);
	AMD::float4x4 amdViewProjMatrix;
	amdViewProjMatrix.m[0] = viewProjMatrix[0][0];
	amdViewProjMatrix.m[1] = viewProjMatrix[0][1];
	amdViewProjMatrix.m[2] = viewProjMatrix[0][2];
	amdViewProjMatrix.m[3] = viewProjMatrix[0][3];
	amdViewProjMatrix.m[4] = viewProjMatrix[1][0];
	amdViewProjMatrix.m[5] = viewProjMatrix[1][1];
	amdViewProjMatrix.m[6] = viewProjMatrix[1][2];
	amdViewProjMatrix.m[7] = viewProjMatrix[1][3];
	amdViewProjMatrix.m[8] = viewProjMatrix[2][0];
	amdViewProjMatrix.m[9] = viewProjMatrix[2][1];
	amdViewProjMatrix.m[10] = viewProjMatrix[2][2];
	amdViewProjMatrix.m[11] = viewProjMatrix[2][3];
	amdViewProjMatrix.m[12] = viewProjMatrix[3][0];
	amdViewProjMatrix.m[13] = viewProjMatrix[3][1];
	amdViewProjMatrix.m[14] = viewProjMatrix[3][2];
	amdViewProjMatrix.m[15] = viewProjMatrix[3][3];
	m_activeScene.scene.get()->m_viewProjMatrix = amdViewProjMatrix;

	auto viewProjMatrixInverse = glm::inverse(viewProjMatrix);
	AMD::float4x4 amdViewProjMatrixInverse;
	amdViewProjMatrixInverse.m[0] = viewProjMatrixInverse[0][0];
	amdViewProjMatrixInverse.m[1] = viewProjMatrixInverse[0][1];
	amdViewProjMatrixInverse.m[2] = viewProjMatrixInverse[0][2];
	amdViewProjMatrixInverse.m[3] = viewProjMatrixInverse[0][3];
	amdViewProjMatrixInverse.m[4] = viewProjMatrixInverse[1][0];
	amdViewProjMatrixInverse.m[5] = viewProjMatrixInverse[1][1];
	amdViewProjMatrixInverse.m[6] = viewProjMatrixInverse[1][2];
	amdViewProjMatrixInverse.m[7] = viewProjMatrixInverse[1][3];
	amdViewProjMatrixInverse.m[8] = viewProjMatrixInverse[2][0];
	amdViewProjMatrixInverse.m[9] = viewProjMatrixInverse[2][1];
	amdViewProjMatrixInverse.m[10] = viewProjMatrixInverse[2][2];
	amdViewProjMatrixInverse.m[11] = viewProjMatrixInverse[2][3];
	amdViewProjMatrixInverse.m[12] = viewProjMatrixInverse[3][0];
	amdViewProjMatrixInverse.m[13] = viewProjMatrixInverse[3][1];
	amdViewProjMatrixInverse.m[14] = viewProjMatrixInverse[3][2];
	amdViewProjMatrixInverse.m[15] = viewProjMatrixInverse[3][3];
	m_activeScene.scene.get()->m_invViewProjMatrix = amdViewProjMatrixInverse;

	AMD::float4 amdCameraPos;
	amdCameraPos.x = cameraPos.x;
	amdCameraPos.y = cameraPos.y;
	amdCameraPos.z = cameraPos.z;
	amdCameraPos.w = 1.0;
	m_activeScene.scene.get()->m_cameraPos = amdCameraPos;
	UpdateSimulationParameters();
	UpdateRenderingParameters();
}

void SkyrimTressFX::UpdateSimulationParameters()
{
	for (int i = 0; i < m_activeScene.objects.size(); ++i) {
		m_activeScene.objects[i].hairStrands->GetTressFXHandle()->UpdateSimulationParameters(&m_activeScene.objects[i].simulationSettings, m_deltaTime);
	}
}

void SkyrimTressFX::UpdateRenderingParameters()
{
	std::vector<const TressFXRenderingSettings*> RenderSettings;

	m_activeScene.viewConstantBuffer->vEye = m_activeScene.scene->GetCameraPos();
	m_activeScene.viewConstantBuffer->mVP = m_activeScene.scene->GetMVP();
	m_activeScene.viewConstantBuffer->mInvViewProj = m_activeScene.scene->GetInvViewProjMatrix();
	m_activeScene.viewConstantBuffer->vViewport = { 0, 0, (float)m_nScreenWidth, (float)m_nScreenHeight };
	m_activeScene.viewConstantBuffer.Update(GetDevice()->GetCurrentCommandContext());

	for (int i = 0; i < m_activeScene.objects.size(); ++i) {
		// For now, just using distance of camera to 0, 0, 0, but should be passing in a root position for the hair object we want to LOD
		float Distance = 0;  //sqrtf(m_activeScene.scene->GetCameraPos().x * m_activeScene.scene->GetCameraPos().x + m_activeScene.scene->GetCameraPos().y * m_activeScene.scene->GetCameraPos().y + m_activeScene.scene->GetCameraPos().z * m_activeScene.scene->GetCameraPos().z);
		logger::info("Fiber radius: {}", m_activeScene.objects[i].renderingSettings.m_FiberRadius);
		m_activeScene.objects[i].hairStrands->GetTressFXHandle()->UpdateRenderingParameters(&m_activeScene.objects[i].renderingSettings, m_nPPLLNodes, m_deltaTime, Distance);
		RenderSettings.push_back(&m_activeScene.objects[i].renderingSettings);
	}

	// Update shade parameters for correct implementation
	switch (m_eOITMethod) {
	case OIT_METHOD_SHORTCUT:
		m_pShortCut->UpdateShadeParameters(RenderSettings);
		break;
	case OIT_METHOD_PPLL:
		m_pPPLL->UpdateShadeParameters(RenderSettings);
		break;
	default:
		break;
	}
}

void SkyrimTressFX::LoadScene()
{
	TressFXSceneDescription desc = LoadTFXUserFiles();

	//EI_Device*         pDevice = GetDevice();
	//EI_CommandContext& uploadCommandContext = pDevice->GetCurrentCommandContext();
	for (size_t i = 0; i < desc.objects.size(); ++i) {
		HairStrands* hair = new HairStrands(
			m_activeScene.scene.get(),
			desc.objects[i].mesh,
			(int)i,
			desc.objects[i].tfxFilePath,
			desc.objects[i].tfxBoneFilePath,
			desc.objects[i].numFollowHairs,
			desc.objects[i].tipSeparationFactor,
			desc.objects[i].hairObjectName,
			desc.objects[i].tressfxSSEData.boneNames,
			desc.objects[i].tressfxSSEData.m_configData,
			desc.objects[i].tressfxSSEData.m_configPath,
			desc.objects[i].tressfxSSEData.m_initialOffsets,
			desc.objects[i].tressfxSSEData.m_userEditorID);

		logger::info("Populate DrawStrands bindset");
		hair->GetTressFXHandle()->PopulateDrawStrandsBindSet(GetDevice(), &desc.objects[i].initialRenderingSettings);
		m_activeScene.objects.push_back({ std::unique_ptr<HairStrands>(hair), desc.objects[i].initialSimulationSettings, desc.objects[i].initialRenderingSettings, desc.objects[i].name.c_str() });
	}

	for (size_t i = 0; i < desc.collisionMeshes.size(); ++i) {
		CollisionMesh* mesh = new CollisionMesh(
			m_activeScene.scene.get(), m_pDebugRenderTargetSet.get(),
			desc.collisionMeshes[i].name.c_str(),
			desc.collisionMeshes[i].tfxMeshFilePath.c_str(),
			desc.collisionMeshes[i].numCellsInXAxis,
			desc.collisionMeshes[i].collisionMargin,
			desc.collisionMeshes[i].mesh,
			desc.collisionMeshes[i].followBone.c_str());
		m_activeScene.collisionMeshes.push_back(std::unique_ptr<CollisionMesh>(mesh));
	}
	logger::info("LoadScene done");
}
void SkyrimTressFX::UpdateLights() {
	auto  accumulator = RE::BSGraphics::BSShaderAccumulator::GetCurrentAccumulator();
	auto shadowSceneNode = accumulator->GetRuntimeData().activeShadowSceneNode;
	auto& runtimeData = shadowSceneNode->GetRuntimeData();
	int   i = 0;
	for (auto& e : runtimeData.activePointLights) {
		auto bsLight = e.get();
		if (!bsLight) {
			continue;
		}
		auto niLight = bsLight->light.get();
		if (!niLight) {
			continue;
		}
		i += 1;
		float x;
		float y;
		float z;
		niLight->world.rotate.ToEulerAnglesXYZ(z, y, x);
		m_activeScene.lightConstantBuffer->LightInfo[i].LightColor = { niLight->diffuse.red, niLight->diffuse.blue, niLight->diffuse.green };
		m_activeScene.lightConstantBuffer->LightInfo[i].LightDirWS = { x, y, z };
		//m_activeScene.lightConstantBuffer->LightInfo[i].LightInnerConeCos = lightInfo.innerConeCos;
		m_activeScene.lightConstantBuffer->LightInfo[i].LightIntensity = bsLight->luminance;
		//m_activeScene.lightConstantBuffer->LightInfo[i].LightOuterConeCos = lightInfo.outerConeCos;
		m_activeScene.lightConstantBuffer->LightInfo[i].LightPositionWS = { niLight->world.translate.x, niLight->world.translate.x, niLight->world.translate.x };
		m_activeScene.lightConstantBuffer->LightInfo[i].LightRange = 1000;//???
		m_activeScene.lightConstantBuffer->LightInfo[i].LightType = 0;//???
		//m_activeScene.lightConstantBuffer->LightInfo[i].ShadowMapIndex = lightInfo.shadowMapIndex;
		//m_activeScene.lightConstantBuffer->LightInfo[i].ShadowProjection = *(AMD::float4x4*)&lightInfo.mLightViewProj;  // ugh .. need a proper math library
		//m_activeScene.lightConstantBuffer->LightInfo[i].ShadowParams = { lightInfo.depthBias, .1f, 100.0f, 0.f };       // Near and Far are currently hard-coded because we are hard-coding them elsewhere
		//m_activeScene.lightConstantBuffer->LightInfo[i].ShadowMapSize = GetDevice()->GetShadowBufferResource()->GetWidth() / 2;
		m_activeScene.lightConstantBuffer->UseDepthApproximation = m_useDepthApproximation;
	}
	m_activeScene.lightConstantBuffer.Update(GetDevice()->GetCurrentCommandContext());
	m_activeScene.lightConstantBuffer->NumLights = i;
}
void SkyrimTressFX::ReloadAllHairs()
{
	for (auto& object : m_activeScene.objects) {
		object.hairStrands.get()->Reload();
	}
	m_doReload = false;
	logger::info("Done reloading");
}

void SkyrimTressFX::Draw()
{
	// Do hair draw - will pick correct render approach
	if (m_drawHair) {
		logger::info("Draw hair");
		DrawHair();
	}
	// Render debug collision mesh if needed
	EI_CommandContext& commandList = GetDevice()->GetCurrentCommandContext();

	if (m_drawCollisionMesh || m_drawMarchingCubes) {
		if (m_drawMarchingCubes) {
			logger::info("Generate marching cubes");
			GenerateMarchingCubes();
		}

		logger::info("Begin render pass");
		GetDevice()->BeginRenderPass(commandList, m_pDebugRenderTargetSet.get(), L"DrawCollisionMesh Pass");
		if (m_drawCollisionMesh) {
			logger::info("Draw collision mesh");
			DrawCollisionMesh();
		}
		if (m_drawMarchingCubes) {
			logger::info("Draw SDF");
			DrawSDF();
		}
		logger::info("End render pass");
		GetDevice()->EndRenderPass(GetDevice()->GetCurrentCommandContext());
	}
}

void SkyrimTressFX::DrawHair()
{
	int                       numHairStrands = (int)m_activeScene.objects.size();
	std::vector<HairStrands*> hairStrands(numHairStrands);
	for (int i = 0; i < numHairStrands; ++i) {
		hairStrands[i] = m_activeScene.objects[i].hairStrands.get();
	}

	EI_CommandContext& pRenderCommandList = GetDevice()->GetCurrentCommandContext();

	switch (m_eOITMethod) {
	case OIT_METHOD_PPLL:
		logger::info("Draw PPLL");
		m_pPPLL->Draw(pRenderCommandList, numHairStrands, hairStrands.data(), m_activeScene.viewBindSet.get(), m_activeScene.lightBindSet.get());
		break;
	case OIT_METHOD_SHORTCUT:
		logger::info("Draw ShortCut");
		m_pShortCut->Draw(pRenderCommandList, numHairStrands, hairStrands.data(), m_activeScene.viewBindSet.get(), m_activeScene.lightBindSet.get());
		break;
	};

	for (size_t i = 0; i < hairStrands.size(); i++)
		hairStrands[i]->TransitionRenderingToSim(pRenderCommandList);
}

void SkyrimTressFX::DrawCollisionMesh()
{
	EI_CommandContext& commandList = GetDevice()->GetCurrentCommandContext();
	for (size_t i = 0; i < m_activeScene.collisionMeshes.size(); i++)
		m_activeScene.collisionMeshes[i]->DrawMesh(commandList);
}

void SkyrimTressFX::GenerateMarchingCubes()
{
#if ENABLE_MARCHING_CUBES
	EI_CommandContext& commandList = GetDevice()->GetCurrentCommandContext();
	for (size_t i = 0; i < m_activeScene.collisionMeshes.size(); i++)
		m_activeScene.collisionMeshes[i]->GenerateIsoSurface(commandList);
#endif
}

void SkyrimTressFX::DrawSDF()
{
#if ENABLE_MARCHING_CUBES
	EI_CommandContext& commandList = GetDevice()->GetCurrentCommandContext();

	for (size_t i = 0; i < m_activeScene.collisionMeshes.size(); i++)
		m_activeScene.collisionMeshes[i]->DrawIsoSurface(commandList);
#endif
}

void SkyrimTressFX::InitializeLayouts()
{
	InitializeAllLayouts(GetDevice());
}

void SkyrimTressFX::DestroyLayouts()
{
	DestroyAllLayouts(GetDevice());
}

void SkyrimTressFX::SetOITMethod(OITMethod method)
{
	if (method == m_eOITMethod) {
		logger::warn("Already in this OIT method...");
		return;
	}

	// Flush the GPU before switching
	//GetDevice()->FlushGPU();

	// Destroy old resources
	DestroyOITResources(m_eOITMethod);

	m_eOITMethod = method;
	RecreateSizeDependentResources();
}

void SkyrimTressFX::DestroyOITResources(OITMethod method)
{
	UNREFERENCED_PARAMETER(method);
	// TressFX Usage
	switch (m_eOITMethod) {
	case OIT_METHOD_PPLL:
		m_pPPLL.reset();
		break;
	case OIT_METHOD_SHORTCUT:
		m_pShortCut.reset();
		break;
	};
}

void SkyrimTressFX::RecreateSizeDependentResources()
{
	//GetDevice()->FlushGPU();
	//GetDevice()->OnResize(m_nScreenWidth, m_nScreenHeight);

	// Whenever there is a resize, we need to re-create on render pass set as it depends on
	// the main color/depth buffers which get resized
	//const EI_Resource* ResourceArray[] = { GetDevice()->GetColorBufferResource(), GetDevice()->GetDepthBufferResource() };
	//m_pGLTFRenderTargetSet->SetResources(ResourceArray);
	//m_pDebugRenderTargetSet->SetResources(ResourceArray);

	//m_activeScene.scene->OnResize(m_nScreenWidth, m_nScreenHeight);

	// Needs to be created in OnResize in case we have debug buffers bound which vary by screen width/height
	EI_BindSetDescription lightSet = {
		{ m_activeScene.lightConstantBuffer.GetBufferResource(), GetDevice()->GetShadowBufferResource() }
	};
	m_activeScene.lightBindSet = GetDevice()->CreateBindSet(GetLightLayout(), lightSet);

	// TressFX Usage
	switch (m_eOITMethod) {
	case OIT_METHOD_PPLL:
		logger::info("Setting OIT method to PPLL");
		m_nPPLLNodes = m_nScreenWidth * m_nScreenHeight * AVE_FRAGS_PER_PIXEL;
		m_pPPLL.reset(new TressFXPPLL);
		m_pPPLL->Initialize(m_nScreenWidth, m_nScreenHeight, m_nPPLLNodes, PPLL_NODE_SIZE);
		break;
	case OIT_METHOD_SHORTCUT:
		logger::info("Setting OIT method to ShortCut");
		m_pShortCut.reset(new TressFXShortCut);
		m_pShortCut->Initialize(m_nScreenWidth, m_nScreenHeight);
		break;
	};
}

TressFXSceneDescription SkyrimTressFX::LoadTFXUserFiles()
{
	TressFXSceneDescription  sd;
	//FILE*                    fp;
	const auto               configPath = std::filesystem::current_path() / "data\\TressFX\\HairConfig";
	const auto               assetPath = std::filesystem::current_path() / "data\\TressFX\\HairAssets";
	const auto               texturePath = std::filesystem::current_path() / "data\\Textures\\TressFX";
	std::vector<std::string> usedNames;
	for (const auto& entry : std::filesystem::directory_iterator(configPath)) {
		std::string configFile = entry.path().generic_string();
		logger::info("config file: {}", configFile);
		std::ifstream f(configFile);
		json          data;
		try {
			data = json::parse(f);
		} catch (json::parse_error e) {
			logger::error("Error parsing hair config at {}: {}", configFile, e.what());
		}
		std::string baseAssetName = data["asset"].get<std::string>();
		const auto  assetTexturePath = texturePath / (baseAssetName + ".dds");
		std::string assetName = baseAssetName;
		uint32_t    i = 1;
		while (std::find(usedNames.begin(), usedNames.end(), assetName) != usedNames.end()) {
			i += 1;
			assetName = baseAssetName + "#" + std::to_string(i);
		}
		logger::info("Asset name: {}", assetName);
		std::string assetFileName = baseAssetName + ".tfx";
		std::string boneFileName = baseAssetName + ".tfxbone";
		const auto  thisAssetPath = assetPath / assetFileName;
		const auto  thisBonePath = assetPath / boneFileName;

		const auto bones = data["bones"].get<std::vector<std::string>>();
		for (auto bone : bones) {
			logger::info("bone: {}", bone);
		}

		if (!data.contains("editorids")) {
			logger::warn("Hair config {} has no editor IDs, discarding.", configFile);
		}
		const auto editorIDs = data["editorids"].get<std::vector<std::string>>();

		if (data.contains("offsets")) {
			logger::info("loading offsets");
			auto  offsets = data["offsets"];
			float x = 0.0;
			if (offsets.contains("x")) {
				x = offsets["x"].get<float>();
			}
			float y = 0.0;
			if (offsets.contains("y")) {
				y = offsets["y"].get<float>();
			}
			float z = 0.0;
			if (offsets.contains("z")) {
				z = offsets["z"].get<float>();
			}
			float scale = 0.0;
			if (offsets.contains("scale")) {
				scale = offsets["scale"].get<float>();
			}
		}
		float fiberRadius = 0.002f;
		float fiberSpacing = 0.1f;
		float fiberRatio = 0.5f;
		float kd = 0.07f;
		float ks1 = 0.17f;
		float ex1 = 14.4f;
		float ks2 = 0.72f;
		float ex2 = 11.8f;
		float hairOpacity = 0.63f;
		float hairShadowAlpha = 0.35f;
		bool  thinTip = true;
		int   localConstraintsIterations = 3;
		int   lengthConstraintsIterations = 3;
		float localConstraintsStiffness = 0.9f;
		float globalConstraintsStiffness = 0.9f;
		float globalConstraintsRange = 0.9f;
		float damping = 0.06f;
		float vspAmount = 0.75f;
		float vspAccelThreshold = 1.2f;
		float gravityMagnitude = 0.09f;
		if (data.contains("parameters")) {
			logger::info("params");
			auto params = data["parameters"];
			if (params.contains("fiberRadius")) {
				fiberRadius = params["fiberRadius"].get<float>();
			}
			if (params.contains("fiberSpacing")) {
				fiberSpacing = params["fiberSpacing"].get<float>();
			}
			if (params.contains("fiberRatio")) {
				fiberRatio = params["fiberRatio"].get<float>();
			}
			if (params.contains("kd")) {
				kd = params["kd"].get<float>();
			}
			if (params.contains("ks1")) {
				ks1 = params["ks1"].get<float>();
			}
			if (params.contains("ex1")) {
				ex1 = params["ex1"].get<float>();
			}
			if (params.contains("ks2")) {
				ks2 = params["ks2"].get<float>();
			}
			if (params.contains("ex2")) {
				ex2 = params["ex2"].get<float>();
			}
			if (params.contains("hairOpacity")) {
				hairOpacity = params["hairOpacity"].get<float>();
			}
			if (params.contains("hairShadowAlpha")) {
				hairShadowAlpha = params["hairShadowAlpha"].get<float>();
			}
			if (params.contains("thinTip")) {
				thinTip = params["thinTip"].get<bool>();
			}
			if (params.contains("localConstraintsIterations")) {
				localConstraintsIterations = params["localConstraintsIterations"].get<int>();
			}
			if (params.contains("lengthConstraintsIterations")) {
				localConstraintsIterations = params["lengthConstraintsIterations"].get<int>();
			}
			if (params.contains("localConstraintsStiffness")) {
				localConstraintsStiffness = params["localConstraintsStiffness"].get<float>();
			}
			if (params.contains("globalConstraintsStiffness")) {
				globalConstraintsStiffness = params["globalConstraintsStiffness"].get<float>();
			}
			if (params.contains("globalConstraintsRange")) {
				globalConstraintsRange = params["globalConstraintsRange"].get<float>();
			}
			if (params.contains("damping")) {
				damping = params["damping"].get<float>();
			}
			if (params.contains("vspAmount")) {
				vspAmount = params["vspAmount"].get<float>();
			}
			if (params.contains("vspAccelThreshold")) {
				vspAccelThreshold = params["vspAccelThreshold"].get<float>();
			}
			if (params.contains("gravityMagnitude")) {
				vspAccelThreshold = params["gravityMagnitude"].get<float>();
			}

			for (auto editorID : editorIDs) {
				logger::info("adding hair for editor id: {}", editorID);
				auto hairName = editorID + ":" + assetName;

				//ppll->m_hairs[hairName]->SetRenderingAndSimParameters(fiberRadius, fiberSpacing, fiberRatio, kd, ks1, ex1, ks2, ex2, localConstraintsIterations, lengthConstraintsIterations, localConstraintsStiffness, globalConstraintsStiffness, globalConstraintsRange, damping, vspAmount, vspAccelThreshold, hairOpacity, hairShadowAlpha, thinTip);
				TressFXSSEData tfxData;
				tfxData.m_configData = data;
				tfxData.m_configPath = configPath;
				tfxData.m_userEditorID = editorID;
				tfxData.boneNames = bones;

				TressFXSimulationSettings simSettings;
				simSettings.m_vspCoeff = vspAmount;
				simSettings.m_vspAccelThreshold = vspAccelThreshold;
				simSettings.m_localConstraintStiffness = localConstraintsStiffness;
				simSettings.m_localConstraintsIterations = localConstraintsIterations;
				simSettings.m_globalConstraintStiffness = globalConstraintsStiffness;
				simSettings.m_globalConstraintsRange = globalConstraintsRange;
				simSettings.m_lengthConstraintsIterations = lengthConstraintsIterations;
				simSettings.m_damping = damping;
				simSettings.m_gravityMagnitude = gravityMagnitude;

				TressFXRenderingSettings renderSettings;
				renderSettings.m_BaseAlbedoName = assetTexturePath.generic_string();
				logger::info("albedo texture path: {}", renderSettings.m_BaseAlbedoName);
				renderSettings.m_EnableThinTip = thinTip;
				renderSettings.m_FiberRadius = fiberRadius;
				renderSettings.m_FiberRatio = fiberRatio;
				renderSettings.m_HairKDiffuse = kd;
				renderSettings.m_HairKSpec1 = ks1;
				renderSettings.m_HairSpecExp1 = ex1;
				renderSettings.m_HairKSpec2 = ks2;
				renderSettings.m_HairSpecExp2 = ex2;

				TressFXObjectDescription hairDesc;
				hairDesc.hairObjectName = hairName;
				hairDesc.name = hairName;
				hairDesc.tfxFilePath = thisAssetPath.generic_string();
				hairDesc.tfxBoneFilePath = thisBonePath.generic_string();
				hairDesc.initialSimulationSettings = simSettings;
				hairDesc.initialRenderingSettings = renderSettings;
				hairDesc.tressfxSSEData = tfxData;
				hairDesc.numFollowHairs = 0;          //TODO
				hairDesc.mesh = (int)m_activeScene.scene.get()->skinIDBoneNamesMap.size();
				hairDesc.tipSeparationFactor = 1.0f;  //???
				sd.objects.push_back(hairDesc);
				//TODO collision mesh
			}
			m_activeScene.scene.get()->skinIDBoneNamesMap[(int)m_activeScene.scene.get()->skinIDBoneNamesMap.size()] = bones;
		}
	}
	return sd;
}
