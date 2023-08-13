#include "SkyrimTressFX.h"
#include "SDF.h"
#include "Scene.h"
#include "TressFX/TressFXPPLL.h"
#include "TressFX/TressFXShortCut.h"
#include "Simulation.h"
using json = nlohmann::json;

SkyrimTressFX::SkyrimTressFX()
{
}

SkyrimTressFX::~SkyrimTressFX()
{
}

void SkyrimTressFX::OnCreate()
{
	GetDevice()->OnCreate();
	m_activeScene.scene.reset(new EI_Scene);
	LoadScene();
}

void SkyrimTressFX::LoadScene()
{
	TressFXSceneDescription sceneDescription = LoadTFXUserFiles();

	// TODO Why?
	DestroyLayouts();

    InitializeLayouts();

	m_pPPLL.reset(new TressFXPPLL);
	m_pShortCut.reset(new TressFXShortCut);
	m_pSimulation.reset(new Simulation);
}

void SkyrimTressFX::Draw()
{
	// Do hair draw - will pick correct render approach
	if (m_drawHair) {
		DrawHair();
	}
	// Render debug collision mesh if needed
	EI_CommandContext& commandList = GetDevice()->GetCurrentCommandContext();

	if (m_drawCollisionMesh || m_drawMarchingCubes) {
		if (m_drawMarchingCubes) {
			GenerateMarchingCubes();
		}

		GetDevice()->BeginRenderPass(commandList, m_pDebugRenderTargetSet.get(), L"DrawCollisionMesh Pass");
		if (m_drawCollisionMesh) {
			DrawCollisionMesh();
		}
		if (m_drawMarchingCubes) {
			DrawSDF();
		}
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
		m_pPPLL->Draw(pRenderCommandList, numHairStrands, hairStrands.data(), m_activeScene.viewBindSet.get(), m_activeScene.lightBindSet.get());
		break;
	case OIT_METHOD_SHORTCUT:
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
	if (method == m_eOITMethod)
		return;

	// Flush the GPU before switching
	//GetDevice()->FlushGPU();

	// Destroy old resources
	DestroyOITResources(m_eOITMethod);

	m_eOITMethod = method;
	RecreateSizeDependentResources();
}

void SkyrimTressFX::DestroyOITResources(OITMethod method)
{
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

	//// Needs to be created in OnResize in case we have debug buffers bound which vary by screen width/height
	//EI_BindSetDescription lightSet = {
	//	{ m_activeScene.lightConstantBuffer.GetBufferResource(), GetDevice()->GetShadowBufferResource() }
	//};
	//m_activeScene.lightBindSet = GetDevice()->CreateBindSet(GetLightLayout(), lightSet);

	//// TressFX Usage
	//switch (m_eOITMethod) {
	//case OIT_METHOD_PPLL:
	//	m_nPPLLNodes = m_nScreenWidth * m_nScreenHeight * AVE_FRAGS_PER_PIXEL;
	//	m_pPPLL.reset(new TressFXPPLL);
	//	m_pPPLL->Initialize(m_nScreenWidth, m_nScreenHeight, m_nPPLLNodes, PPLL_NODE_SIZE);
	//	break;
	//case OIT_METHOD_SHORTCUT:
	//	m_pShortCut.reset(new TressFXShortCut);
	//	m_pShortCut->Initialize(m_nScreenWidth, m_nScreenHeight);
	//	break;
	//};
}

TressFXSceneDescription SkyrimTressFX::LoadTFXUserFiles()
{
	TressFXSceneDescription  sd;
	FILE*                    fp;
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
		float fiberRadius = 0.002;
		float fiberSpacing = 0.1;
		float fiberRatio = 0.5;
		float kd = 0.07;
		float ks1 = 0.17;
		float ex1 = 14.4;
		float ks2 = 0.72;
		float ex2 = 11.8;
		float hairOpacity = 0.63;
		float hairShadowAlpha = 0.35;
		bool  thinTip = true;
		int   localConstraintsIterations = 3;
		int   lengthConstraintsIterations = 3;
		float localConstraintsStiffness = 0.9;
		float globalConstraintsStiffness = 0.9;
		float globalConstraintsRange = 0.9;
		float damping = 0.06;
		float vspAmount = 0.75;
		float vspAccelThreshold = 1.2;
		float gravityMagnitude = 0.09;
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
				hairDesc.numFollowHairs = 0; //TODO
				hairDesc.mesh = 0; //???
				hairDesc.tipSeparationFactor = 1.0f; //???
				sd.objects.push_back(hairDesc);
				//TODO collision mesh
			}
		}
	}
}
