#include "HairStrands.h"
#include "ActorManager.h"
#include "DDSTextureLoader.h"
#include "Util.h"
#include <Menu.h>
#include <RE/S/ShadowState.h>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <sys/stat.h>
#include "SkyrimTressFX.h"

HairStrands::HairStrands(EI_Scene* scene, int skinNumber, int renderIndex, std::string tfxFilePath, std::string tfxboneFilePath, int numFollowHairsPerGuideHair, float tipSeparationFactor, std::string name, std::vector<std::string> boneNames, json config, std::filesystem::path configPath, float initialOffsets[4], std::string userEditorID)
{
	logger::info("Hair strands constructor");
	m_pHairAsset = new TressFXAsset();
	FILE* fp = fopen(tfxFilePath.c_str(), "rb");
	logger::info("Load hair data");
	m_pHairAsset->LoadHairData(fp);
	fclose(fp);

	logger::info("Generate follow hairs");
	m_pHairAsset->GenerateFollowHairs(numFollowHairsPerGuideHair, tipSeparationFactor, 0.012f);
	
	logger::info("Process asset");
	m_pHairAsset->ProcessAsset();

	// Load *.tfxbone
	fp = fopen(tfxboneFilePath.c_str(), "rb");
	logger::info("Load bone data");
	m_pHairAsset->LoadBoneData(fp, skinNumber, scene);
	fclose(fp);

	EI_Device*         pDevice = GetDevice();
	EI_CommandContext& uploadCommandContext = pDevice->GetCurrentCommandContext();

	logger::info("Create hair object");
	auto hair = new TressFXHairObject(m_pHairAsset, pDevice, uploadCommandContext, name.c_str(), renderIndex);
	m_pHairObject.reset(hair);
	m_pScene = scene;
	m_skinNumber = skinNumber;
	m_renderIndex = renderIndex;

	logger::info("Created hair object");
	m_hairName = name;
	m_numBones = boneNames.size();
	logger::info("num bones: {}", m_numBones);
	for (int i = 0; i < m_numBones; i++) {
		m_boneNames[i] = boneNames[i];
	}
	m_config = config;
	m_configPath = configPath.generic_string();
	m_currentOffsets[0] = initialOffsets[0];
	m_currentOffsets[1] = initialOffsets[1];
	m_currentOffsets[2] = initialOffsets[2];
	m_currentOffsets[3] = initialOffsets[3];
	m_userEditorID = userEditorID;
	logger::info("Initial offsets: {}, {}, {}, {}", m_currentOffsets[0], m_currentOffsets[1], m_currentOffsets[2], m_currentOffsets[3]);
	//m_pHairObject.get()->UpdateStrandOffsets(m_pHairAsset, GetDevice()->GetCurrentCommandContext(), initialOffsets[0] * Util::RenderScale, initialOffsets[1] * Util::RenderScale, initialOffsets[2] * Util::RenderScale, initialOffsets[3] * Util::RenderScale);
}

HairStrands::~HairStrands()
{
	//auto pDevice = RE::BSGraphics::Renderer::GetSingleton()->GetRuntimeData().forwarder;
	logger::info("Destroying hair");
	delete m_pHairAsset;
	m_hairSRV->Release();
	m_hairTexture->Release();
	delete m_hairEIResource;
}

void HairStrands::Reload()
{
	EI_Device*         pDevice = GetDevice();
	EI_CommandContext& uploadCommandContext = pDevice->GetCurrentCommandContext();
	m_pHairObject.release();
	auto hair = new TressFXHairObject(m_pHairAsset, pDevice, uploadCommandContext, m_hairName.c_str(), m_renderIndex);
	m_pHairObject.reset(hair);
	
	logger::info("Offsets: {}, {}, {}, {}", m_currentOffsets[0], m_currentOffsets[1], m_currentOffsets[2], m_currentOffsets[3]);
	m_pHairObject->UpdateStrandOffsets(m_pHairAsset, GetDevice()->GetCurrentCommandContext(), m_currentOffsets[0] * Util::RenderScale, m_currentOffsets[1] * Util::RenderScale, m_currentOffsets[2] * Util::RenderScale, m_currentOffsets[3]);
}

void HairStrands::DrawDebugMarkers()
{
	//bone debug markers
	std::vector<DirectX::XMMATRIX> positions;
	for (uint16_t i = 0; i < m_numBones; i++) {
		auto boneTransform = m_boneTransforms[i];
		auto bonePos = Util::ToRenderScale(glm::vec3(boneTransform.translate.x, boneTransform.translate.y, boneTransform.translate.z));
		auto boneRot = boneTransform.rotate;

		//auto bonePos = m_bones[i]->world.translate;
		//auto boneRot = m_bones[i]->world.rotate;

		auto translation = DirectX::XMMatrixTranspose(DirectX::XMMatrixTranslation(bonePos.x, bonePos.y, bonePos.z));
		auto rotation = DirectX::XMMATRIX(boneRot.entry[0][0], boneRot.entry[0][1], boneRot.entry[0][2], 0, boneRot.entry[1][0], boneRot.entry[1][1], boneRot.entry[1][2], 0, boneRot.entry[2][0], boneRot.entry[2][1], boneRot.entry[2][2], 0, 0, 0, 0, 1);
		auto transform = translation * rotation;
		//Menu::GetSingleton()->DrawMatrix(transform, "bone");
		MarkerRender::GetSingleton()->m_markerPositions.push_back(transform);
	}
}

void HairStrands::UpdateVariables(float gravityMagnitude)
{
	UNREFERENCED_PARAMETER(gravityMagnitude);
}
void HairStrands::SetRenderingAndSimParameters(float fiberRadius, float fiberSpacing, float fiberRatio, float kd, float ks1, float ex1, float ks2, float ex2, int localConstraintsIterations, int lengthConstraintsIterations, float localConstraintsStiffness, float globalConstraintsStiffness, float globalConstraintsRange, float damping, float vspAmount, float vspAccelThreshold, float hairOpacity, float hairShadowAlpha, bool thinTip)
{
	m_fiberRadius = fiberRadius;
	m_fiberSpacing = fiberSpacing;
	m_fiberRatio = fiberRatio;
	m_kd = kd;
	m_ks1 = ks1;
	m_ex1 = ex1;
	m_ks2 = ks2;
	m_ex2 = ex2;
	m_localConstraintsIterations = localConstraintsIterations;
	m_lengthConstraintsIterations = lengthConstraintsIterations;
	m_localConstraintsStiffness = localConstraintsStiffness;
	m_globalConstraintsStiffness = globalConstraintsStiffness;
	m_globalConstraintsRange = globalConstraintsRange;
	m_damping = damping;
	m_vspAmount = vspAmount;
	m_vspAccelThreshold = vspAccelThreshold;
	m_hairOpacity = hairOpacity;
	m_hairShadowAlpha = hairShadowAlpha;
	m_thinTip = thinTip;
}

void HairStrands::TransitionRenderingToSim(EI_CommandContext& context)
{
	m_pHairObject->GetDynamicState().TransitionRenderingToSim(context);
}

void HairStrands::TransitionSimToRendering(EI_CommandContext& context)
{
	m_pHairObject->GetDynamicState().TransitionSimToRendering(context);
}


void ListChildren(RE::NiTObjectArray<RE::NiPointer<RE::NiAVObject>>& nodes, int currentOffset = 0)
{
	for (uint16_t i = 0; i < nodes.size(); i++) {
		//logger::info("\nlayer: {}", currentOffset);
		auto        node = nodes[i].get();
		std::string output = "    ";
		for (int j = 0; j < currentOffset; j++) {
			output.append("    ");
		}
		if (node == nullptr) {
			output.append("null node");
			logger::info("{}", output);
			continue;
		}
		if (node->name.contains("FaceGen")) {
			output.append("Skipping FaceGen node: ");
			output.append(node->name);
			logger::info("{}", output);
			continue;
		} else if (node->name.contains("%")) {
			output.append("Skipping node with %: ");
			output.append(node->name);
			logger::info("{}", output);
			continue;
		} else if (node->name.contains("Weapon")) {
			output.append("Skipping weapon: ");
			output.append(node->name);
			logger::info("{}", output);
			continue;
		} else if (node->name.contains("Helmet")) {
			output.append("Skipping helmet: ");
			output.append(node->name);
			logger::info("{}", output);
			continue;
		} else if (node->name.contains("shades")) {
			output.append("Skipping shades: ");
			output.append(node->name);
			logger::info("{}", output);
			continue;
		} else if (node->name.contains("Body")) {
			output.append("Skipping body: ");
			output.append(node->name);
			logger::info("{}", output);
			continue;
		} else if (node->name.contains("Face")) {
			output.append("Skipping face: ");
			output.append(node->name);
			logger::info("{}", output);
			continue;
		}
		output.append(node->name);
		logger::info("{}", output);
		//logger::info("Getting children");
		auto& children = node->AsNode()->GetChildren();
		if (std::addressof(children) == NULL) {
			logger::info("Children is null");
			continue;
		}
		//logger::info("Got children");
		//logger::info("{}", ptr_to_string(std::addressof(children)));
		//logger::info("capacity: {}", children.capacity());
		uint16_t size = children.size();
		//logger::info("size: {}", size);
		if (size != 0) {
			ListChildren(children, currentOffset + 1);
		}
	}
}
void HairStrands::RegisterBones()
{
	hdt::ActorManager::Skeleton* playerSkeleton = hdt::ActorManager::instance()->m_playerSkeleton;
	if (playerSkeleton == nullptr) {
		logger::info("not player skeleton");
		return;
	}
	//ListChildren(playerSkeleton->skeleton->GetChildren());
	//logger::info("Skeleton address: {}", Util::ptr_to_string(playerSkeleton));
	//ListChildren(playerSkeleton->skeleton->GetChildren());
	logger::info("Got player skeleton");
	logger::info("Num bones to get: {}", m_numBones);
	for (uint16_t i = 0; i < m_numBones; i++) {
		logger::info("Getting bone, name: {}", m_boneNames[i]);
		if (m_boneNames[i] == "SKIP_NODE") {
			m_pBones[i] = nullptr;
		}
		auto bone = playerSkeleton->skeleton->GetObjectByName(m_boneNames[i]);
		if (!bone) {
			logger::warn("Failed to get bone: {}", m_boneNames[i]);
			return;
		}

		m_pBones[i] = bone;
	}
	logger::info("Got all bones");
	m_gotSkeleton = true;

	logger::info("Offsets: {}, {}, {}, {}", m_currentOffsets[0], m_currentOffsets[1], m_currentOffsets[2], m_currentOffsets[3]);
	m_pHairObject.get()->UpdateStrandOffsets(m_pHairAsset, GetDevice()->GetCurrentCommandContext(), m_currentOffsets[0] * Util::RenderScale, m_currentOffsets[1] * Util::RenderScale, m_currentOffsets[2] * Util::RenderScale, m_currentOffsets[3] * Util::RenderScale);
}
void HairStrands::UpdateBones()
{
	if (!m_gotSkeleton) {
		//logger::warn("UpdateBones called, but we have no skeleton!");
		return;
	}
	for (uint16_t i = 0; i < m_numBones; i++) {
		//skip unneeded nodes
		if (m_pBones[i] == nullptr) {
			m_boneTransforms[i] = RE::NiTransform();
			continue;
		}
		m_boneTransforms[i] = RE::NiTransform(m_pBones[i]->world);
	}
}
bool HairStrands::GetUpdatedBones(EI_CommandContext context)
{
	//PrintAllD3D11DebugMessages(m_pManager->m_pDevice);
	if (!m_gotSkeleton) {
		return false;
	}
	//float scale = playerSkeleton->skeleton->world.scale;
	//logger::info("Skeleton has {} bones", playerSkeleton->skeleton->GetChildren().size());
	//auto& children = playerSkeleton->skeleton->GetChildren();
	//ListChildren(children);
	//logger::info("num bones: {}", m_numBones);
	m_boneMatricesXMMATRIX.clear();
	m_boneMatrices.clear();
	for (uint16_t i = 0; i < m_numBones; i++) {
		auto bonePos = Util::ToRenderScale(glm::vec3(m_boneTransforms[i].translate.x, m_boneTransforms[i].translate.y, m_boneTransforms[i].translate.z));

		auto boneRot = m_boneTransforms[i].rotate.Transpose();
		//Menu::GetSingleton()->DrawMatrix(boneRot, "bone");
		AMD::float4x4 mat = { boneRot.entry[0][0],
			boneRot.entry[0][1],
			boneRot.entry[0][2],
			0.0,
			boneRot.entry[1][0],
			boneRot.entry[1][1],
			boneRot.entry[1][2],
			0.0,
			boneRot.entry[2][0],
			boneRot.entry[2][1],
			boneRot.entry[2][2],
			0.0,
			bonePos.x,
			bonePos.y,
			bonePos.z,
			0.0 };
		m_boneMatrices.push_back(mat);
		m_boneMatricesXMMATRIX.push_back(XMMatrixSet(boneRot.entry[0][0],
			boneRot.entry[0][1],
			boneRot.entry[0][2],
			0.0,
			boneRot.entry[1][0],
			boneRot.entry[1][1],
			boneRot.entry[1][2],
			0.0,
			boneRot.entry[2][0],
			boneRot.entry[2][1],
			boneRot.entry[2][2],
			0.0,
			bonePos.x,
			bonePos.y,
			bonePos.z,
			0.0));
	}

	//logger::info("assembled matrices");
	m_pHairObject->UpdateBoneMatrices(&(m_boneMatrices.front()), (int)m_numBones);
	SkyrimTressFX::GetSingleton()->m_activeScene.scene.get()->skinIDBoneTransformsMap[m_skinNumber] = m_boneMatricesXMMATRIX;
	return true;
}
void HairStrands::UpdateOffsets(float x, float y, float z, float scale)
{
	m_currentOffsets[0] = x;
	m_currentOffsets[1] = y;
	m_currentOffsets[2] = z;
	m_currentOffsets[3] = scale;
	m_pHairObject->UpdateStrandOffsets(m_pHairAsset, GetDevice()->GetCurrentCommandContext(), x * Util::RenderScale, y * Util::RenderScale, z * Util::RenderScale, scale);
}
void HairStrands::ExportOffsets(float x, float y, float z, float scale)
{
	json offsets;
	offsets["x"] = x;
	offsets["y"] = y;
	offsets["z"] = z;
	offsets["scale"] = scale;
	m_config["offsets"] = offsets;
	logger::info("Exporting offsets: {}, {}, {}, {}, path: {}", x, y, z, scale, m_configPath);
	std::remove(m_configPath.c_str());
	std::ofstream file(m_configPath);
	file << std::setw(4) << m_config <<std::endl;
}
void HairStrands::ExportParameters()
{
	json parameters;
	parameters["fiberRadius"] = m_fiberRadius;
	parameters["fiberSpacing"] = m_fiberSpacing;
	parameters["fiberRatio"] = m_fiberRatio;
	parameters["kd"] = m_kd;
	parameters["ks1"] = m_ks1;
	parameters["ex1"] = m_ex1;
	parameters["ks2"] = m_ks2;
	parameters["ex2"] = m_ex2;
	parameters["hairOpacity"] = m_hairOpacity;
	parameters["hairShadowAlpha"] = m_hairShadowAlpha;
	parameters["thinTip"] = m_thinTip;
	parameters["localConstraintsIterations"] = m_localConstraintsIterations;
	parameters["lengthConstraintsIterations"] = m_lengthConstraintsIterations;
	parameters["localConstraintsStiffness"] = m_localConstraintsStiffness;
	parameters["globalConstraintsStiffness"] = m_globalConstraintsStiffness;
	parameters["globalConstraintsRange"] = m_globalConstraintsRange;
	parameters["damping"] = m_damping;
	parameters["vspAmount"] = m_vspAmount;
	parameters["vspAccelThreshold"] = m_vspAccelThreshold;
	m_config["parameters"] = parameters;
	std::ofstream file(m_configPath);
	file << m_config;
}

void HairStrands::initialize(std::filesystem::path texturePath)
{
}
