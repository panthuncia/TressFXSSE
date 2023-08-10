#include "Hair.h"
#include "ActorManager.h"
#include "DDSTextureLoader.h"
#include "PPLLObject.h"
#include "SkyrimGPUInterface.h"
#include "Util.h"
#include <Menu.h>
#include <RE/S/ShadowState.h>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <sys/stat.h>
void printEffectVariables(ID3DX11Effect* pEffect);

Hair::Hair(AMD::TressFXAsset* asset, SkyrimGPUResourceManager* resourceManager, ID3D11DeviceContext* context, EI_StringHash name, std::vector<std::string> boneNames, std::filesystem::path texturePath)
{
	m_pHairObject = new TressFXHairObject;
	m_hairAsset = asset;
	m_hairEIResource = new EI_Resource;
	initialize(resourceManager, texturePath);
	m_hairEIResource->srv = m_hairSRV;
	m_pHairObject->Create(asset, (EI_Device*)resourceManager, (EI_CommandContextRef)context, name, m_hairEIResource);
	logger::info("Created hair object");
	m_hairName = name;
	m_numBones = boneNames.size();
	logger::info("num bones: {}", m_numBones);
	for (int i = 0; i < m_numBones; i++) {
		m_boneNames[i] = boneNames[i];
	}
}

Hair::~Hair()
{
	auto pDevice = RE::BSGraphics::Renderer::GetSingleton()->GetRuntimeData().forwarder;
	logger::info("Destroying hair");
	m_pHairObject->Destroy((EI_Device*)pDevice);
	m_hairSRV->Release();
	m_hairTexture->Release();
	delete m_hairEIResource;
}

void Hair::Reload()
{
	auto pManager = SkyrimGPUResourceManager::GetInstance();
	m_pHairObject->Destroy((EI_Device*)pManager->m_pDevice);
	ID3D11DeviceContext* pContext;
	pManager->m_pDevice->GetImmediateContext(&pContext);
	m_pHairObject->Create(m_hairAsset, (EI_Device*)pManager, (EI_CommandContextRef)pContext, m_hairName, m_hairEIResource);
	m_pHairObject->UpdateStrandOffsets(m_hairAsset, (EI_Device*)pManager->m_pDevice, (EI_CommandContextRef)pContext, m_currentOffsets[0] * Util::RenderScale, m_currentOffsets[1] * Util::RenderScale, m_currentOffsets[2] * Util::RenderScale, m_currentOffsets[3]);
}

void Hair::DrawDebugMarkers()
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

void Hair::UpdateVariables(float gravityMagnitude)
{
	logger::info("In hair UpdateVariables");
	if (!m_gotSkeleton)
		return;
	/*hdt::ActorManager::Skeleton* playerSkeleton = hdt::ActorManager::instance()->m_playerSkeleton;
	if (playerSkeleton == NULL)
		return;*/

	PPLLObject* ppll = PPLLObject::GetSingleton();

	//thin tip
	ppll->m_pStrandEffect->GetVariableByName("g_bThinTip")->AsScalar()->SetBool(m_thinTip);

	//ratio TODO: What does this do?
	ppll->m_pStrandEffect->GetVariableByName("g_Ratio")->AsScalar()->SetFloat(m_fiberRatio);
	//strand shading params, cbuffer tressfxShadeParameters
	ppll->m_pStrandEffect->GetVariableByName("g_HairShadowAlpha")->AsScalar()->SetFloat(0.004);
	ppll->m_pStrandEffect->GetVariableByName("g_FiberRadius")->AsScalar()->SetFloat(m_fiberRadius);
	ppll->m_pStrandEffect->GetVariableByName("g_FiberSpacing")->AsScalar()->SetFloat(m_fiberSpacing);
	DirectX::XMVECTOR matBaseColorVector = DirectX::XMVectorSet(1, 1, 1, m_hairOpacity);
	ppll->m_pStrandEffect->GetVariableByName("g_MatBaseColor")->AsVector()->SetFloatVector(reinterpret_cast<float*>(&matBaseColorVector));
	DirectX::XMVECTOR matKValueVector = DirectX::XMVectorSet(0, m_kd, m_ks1, m_ex1);
	ppll->m_pStrandEffect->GetVariableByName("g_MatKValue")->AsVector()->SetFloatVector(reinterpret_cast<float*>(&matKValueVector));
	ppll->m_pStrandEffect->GetVariableByName("g_fHairKs2")->AsScalar()->SetFloat(m_ks2);
	ppll->m_pStrandEffect->GetVariableByName("g_fHairEx2")->AsScalar()->SetFloat(m_ex2);
	ppll->m_pStrandEffect->GetVariableByName("g_NumVerticesPerStrand")->AsScalar()->SetInt(m_hairAsset->m_numVerticesPerStrand);

	//quad shading params, cbuffer tressfxShadeParameters
	ppll->m_pQuadEffect->GetVariableByName("g_HairShadowAlpha")->AsScalar()->SetFloat(m_hairShadowAlpha);
	ppll->m_pQuadEffect->GetVariableByName("g_FiberRadius")->AsScalar()->SetFloat(m_fiberRadius);
	ppll->m_pQuadEffect->GetVariableByName("g_FiberSpacing")->AsScalar()->SetFloat(m_fiberSpacing);
	ppll->m_pQuadEffect->GetVariableByName("g_MatBaseColor")->AsVector()->SetFloatVector(reinterpret_cast<float*>(&matBaseColorVector));
	ppll->m_pQuadEffect->GetVariableByName("g_MatKValue")->AsVector()->SetFloatVector(reinterpret_cast<float*>(&matKValueVector));
	ppll->m_pQuadEffect->GetVariableByName("g_fHairKs2")->AsScalar()->SetFloat(m_ks2);
	ppll->m_pQuadEffect->GetVariableByName("g_fHairEx2")->AsScalar()->SetFloat(m_ex2);
	ppll->m_pQuadEffect->GetVariableByName("g_NumVerticesPerStrand")->AsScalar()->SetInt(m_hairAsset->m_numVerticesPerStrand);
	//sim parameters
	//float4 g_Wind;
	//float4 g_Wind1;
	//float4 g_Wind2;
	//float4 g_Wind3;

	//float4 g_Shape;        // damping, local stiffness, global stiffness, global range.
	//float4 g_GravTimeTip;  // gravity maginitude (assumed to be in negative y direction.)
	//int4   g_SimInts;      // Length iterations, local iterations, collision flag.
	//int4   g_Counts;       // num strands per thread group, num follow hairs per guid hair, num verts per strand.
	//float4 g_VSP;          // VSP parmeters

	TressFXSimulationSettings settings;
	settings.m_damping = m_damping;
	settings.m_localConstraintStiffness = m_localConstraintsStiffness;    //0.8;
	settings.m_globalConstraintStiffness = m_globalConstraintsStiffness;  //0.0;
	settings.m_globalConstraintsRange = m_globalConstraintsRange;
	settings.m_gravityMagnitude = gravityMagnitude;
	settings.m_vspAccelThreshold = m_vspAccelThreshold;
	settings.m_vspCoeff = m_vspAmount;
	settings.m_tipSeparation = 0;
	settings.m_windMagnitude = 0;
	settings.m_windAngleRadians = 0;
	settings.m_windDirection[0] = 0;
	settings.m_windDirection[1] = 0;
	settings.m_windDirection[2] = 0;
	settings.m_localConstraintsIterations = m_localConstraintsIterations;
	settings.m_lengthConstraintsIterations = m_lengthConstraintsIterations;  //?

	logger::info("Setting sim parameters");
	m_pHairObject->UpdateSimulationParameters(settings);
}
void Hair::SetRenderingAndSimParameters(float fiberRadius, float fiberSpacing, float fiberRatio, float kd, float ks1, float ex1, float ks2, float ex2, int localConstraintsIterations, int lengthConstraintsIterations, float localConstraintsStiffness, float globalConstraintsStiffness, float globalConstraintsRange, float damping, float vspAmount, float vspAccelThreshold, float hairOpacity, float hairShadowAlpha, bool thinTip)
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
void Hair::Draw(ID3D11DeviceContext* pContext, EI_PSO* pPSO)
{
	m_pHairObject->DrawStrands((EI_CommandContextRef)pContext, *pPSO);
}

void Hair::TransitionRenderingToSim(ID3D11DeviceContext* pContext)
{
	m_pHairObject->GetPosTanCollection().TransitionRenderingToSim((EI_CommandContextRef)pContext);
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
void Hair::RegisterBones()
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
}
void Hair::UpdateBones()
{
	if (!m_gotSkeleton) {
		logger::warn("UpdateBones called, but we have no skeleton!");
		return;
	}
	for (uint16_t i = 0; i < m_numBones; i++) {
		//skip unneeded nodes
		if (m_pBones[i] == nullptr) {
			m_boneTransforms[i] = RE::NiTransform();
			continue;
		}
		m_boneTransforms[i] = m_pBones[i]->world;
	}
}
bool Hair::Simulate(SkyrimGPUResourceManager* pManager, TressFXSimulation* pSimulation)
{
	//PrintAllD3D11DebugMessages(m_pManager->m_pDevice);
	if (!m_gotSkeleton) {
		return false;
	}
	//float scale = playerSkeleton->skeleton->world.scale;
	std::vector<float>* matrices = new std::vector<float>();
	//logger::info("Skeleton has {} bones", playerSkeleton->skeleton->GetChildren().size());
	//auto& children = playerSkeleton->skeleton->GetChildren();
	//ListChildren(children);
	logger::info("num bones: {}", m_numBones);
	for (uint16_t i = 0; i < m_numBones; i++) {
		auto bonePos = Util::ToRenderScale(glm::vec3(m_boneTransforms[i].translate.x, m_boneTransforms[i].translate.y, m_boneTransforms[i].translate.z));

		auto boneRot = m_boneTransforms[i].rotate.Transpose();
		//Menu::GetSingleton()->DrawMatrix(boneRot, "bone");
		logger::info("Bone transform:");
		logger::info("{}, {}, {}, {}", boneRot.entry[0][0], boneRot.entry[0][1], boneRot.entry[0][2], 0.0);
		logger::info("{}, {}, {}, {}", boneRot.entry[1][0], boneRot.entry[1][1], boneRot.entry[1][2], 0.0);
		logger::info("{}, {}, {}, {}", boneRot.entry[2][0], boneRot.entry[2][1], boneRot.entry[2][2], 0.0);
		logger::info("{}, {}, {}, {}", bonePos.x, bonePos.y, bonePos.z, 0.0);
		matrices->push_back(boneRot.entry[0][0]);
		matrices->push_back(boneRot.entry[0][1]);
		matrices->push_back(boneRot.entry[0][2]);
		matrices->push_back(0.0);
		matrices->push_back(boneRot.entry[1][0]);
		matrices->push_back(boneRot.entry[1][1]);
		matrices->push_back(boneRot.entry[1][2]);
		matrices->push_back(0.0);
		matrices->push_back(boneRot.entry[2][0]);
		matrices->push_back(boneRot.entry[2][1]);
		matrices->push_back(boneRot.entry[2][2]);
		matrices->push_back(0.0);
		matrices->push_back(bonePos.x);
		matrices->push_back(bonePos.y);
		matrices->push_back(bonePos.z);
		matrices->push_back(1.0);
	}

	logger::info("assembled matrices");
	m_pHairObject->UpdateBoneMatrices((EI_CommandContextRef)pManager, &(matrices->front()), BONE_MATRIX_SIZE * m_numBones);
	delete (matrices);
	logger::info("updated matrices");
	ID3D11DeviceContext* context;
	pManager->m_pDevice->GetImmediateContext(&context);
	logger::info("Before simulate call");
	pSimulation->Simulate((EI_CommandContextRef)context, *m_pHairObject);
	logger::info("After simulate call");
	//PrintAllD3D11DebugMessages(m_pManager->m_pDevice);
	logger::info("End of TFX Simulate Debug");
	ID3D11DeviceContext* pContext;
	pManager->m_pDevice->GetImmediateContext(&pContext);
	m_pHairObject->GetPosTanCollection().TransitionSimToRendering((EI_CommandContextRef)pContext);
	logger::info("simulation complete");
	return true;
}
void Hair::UpdateOffsets(float x, float y, float z, float scale)
{
	ID3D11Device*        pDevice = RE::BSGraphics::Renderer::GetSingleton()->GetRuntimeData().forwarder;
	ID3D11DeviceContext* pDeviceContext = RE::BSGraphics::Renderer::GetSingleton()->GetRuntimeData().context;
	m_currentOffsets[0] = x;
	m_currentOffsets[1] = y;
	m_currentOffsets[2] = z;
	m_currentOffsets[3] = scale;
	m_pHairObject->UpdateStrandOffsets(m_hairAsset, (EI_Device*)pDevice, (EI_CommandContextRef)pDeviceContext, x * Util::RenderScale, y * Util::RenderScale, z * Util::RenderScale, scale);
}
void Hair::ExportOffsets(float x, float y, float z, float scale)
{
	json offsets;
	offsets["x"] = x;
	offsets["y"] = y;
	offsets["z"] = z;
	offsets["scale"] = scale;
	m_config["offsets"] = offsets;
	std::ofstream file(m_configPath);
	file << m_config;
}
void Hair::ExportParameters()
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
void Hair::initialize(SkyrimGPUResourceManager* pManager, std::filesystem::path texturePath)
{
	//create texture and SRV (empty for now)
	ID3D11DeviceContext* pContext;
	pManager->GetInstance()->m_pDevice->GetImmediateContext(&pContext);
	DirectX::CreateDDSTextureFromFile(pManager->GetInstance()->m_pDevice, pContext, texturePath.generic_wstring().c_str(), &m_hairTexture, &m_hairSRV);
	logger::info("loaded hair texture");
	/*D3D11_TEXTURE2D_DESC desc;
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
	pManager = pManager;
	pManager->m_pDevice->CreateTexture2D(&desc, NULL, &m_hairTexture);
	pManager->m_pDevice->CreateShaderResourceView(m_hairTexture, nullptr, &m_hairSRV);*/
	//create input layout for rendering
	//D3D11_INPUT_ELEMENT_DESC inputDesc;
	//pManager->m_pDevice->CreateInputLayout();
	//compile effects
}
