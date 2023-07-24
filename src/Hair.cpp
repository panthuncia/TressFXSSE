#include "Hair.h"
#include "ActorManager.h"
#include "SkyrimGPUInterface.h"
#include "Util.h"
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <sys/stat.h>
#include <RE/S/ShadowState.h>
#include <Menu.h>
#include "PPLLObject.h"
void        PrintAllD3D11DebugMessages(ID3D11Device* d3dDevice);
void        printEffectVariables(ID3DX11Effect* pEffect);

Hair::Hair(AMD::TressFXAsset* asset, SkyrimGPUResourceManager* resourceManager, ID3D11DeviceContext* context, EI_StringHash name, std::vector<std::string> boneNames)
{
	m_pHairObject = new TressFXHairObject;
	m_hairAsset = asset;
	m_hairEIResource = new EI_Resource;
	initialize(resourceManager);
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

Hair::~Hair() {
	auto pDevice = RE::BSGraphics::Renderer::GetSingleton()->GetRuntimeData().forwarder;
	logger::info("Destroying hair");
	m_pHairObject->Destroy((EI_Device*)pDevice);
	m_hairSRV->Release();
	m_hairTexture->Release();
	delete m_hairEIResource;
}

void Hair::Reload() {
	auto pManager = SkyrimGPUResourceManager::GetInstance();
	m_pHairObject->Destroy((EI_Device*)pManager->m_pDevice);
	ID3D11DeviceContext* pContext;
	pManager->m_pDevice->GetImmediateContext(&pContext);
	m_pHairObject->Create(m_hairAsset, (EI_Device*)pManager, (EI_CommandContextRef)pContext, m_hairName, m_hairEIResource);
}

void Hair::DrawDebugMarkers()
{
	//bone debug markers
	std::vector<DirectX::XMMATRIX> positions;
	for (uint16_t i = 0; i < m_numBones; i++) {
		
		auto              boneTransform = m_boneTransforms[i];
		auto              bonePos = boneTransform.translate;
		auto              boneRot = boneTransform.rotate;

		//auto bonePos = m_bones[i]->world.translate;
		//auto boneRot = m_bones[i]->world.rotate;
		
		auto translation = DirectX::XMMatrixTranspose(DirectX::XMMatrixTranslation(bonePos.x, bonePos.y, bonePos.z));
		auto rotation = DirectX::XMMATRIX(boneRot.entry[0][0], boneRot.entry[0][1], boneRot.entry[0][2], 0, boneRot.entry[1][0], boneRot.entry[1][1], boneRot.entry[1][2], 0, boneRot.entry[2][0], boneRot.entry[2][1], boneRot.entry[2][2], 0, 0, 0, 0, 1);
		auto transform = translation*rotation;
		//Menu::GetSingleton()->DrawMatrix(transform, "bone");
		positions.push_back(transform);
	}
	//auto state = RE::BSGraphics::RendererShadowState::GetSingleton();
	//auto viewMatrix = state->GetRuntimeData().cameraData.getEye().viewMat;

	RE::NiCamera* playerCam = Util::GetPlayerNiCamera().get();
	RE::NiPoint3  translation = playerCam->world.translate;
	RE::NiMatrix3 rotation = playerCam->world.rotate;
	
	//viewMatrix = DirectX::XMMatrixTranslation(translation.x, translation.y, translation.z), viewMatrix;*/
	auto cameraTrans = DirectX::XMMatrixTranspose(DirectX::XMMatrixTranslation(translation.x, translation.y, translation.z));
	auto cameraRot = DirectX::XMMatrixSet(rotation.entry[0][0], rotation.entry[0][1], rotation.entry[0][2], 0, 
											rotation.entry[1][0], rotation.entry[1][1], rotation.entry[1][2], 0, 
											rotation.entry[2][0], rotation.entry[2][1], rotation.entry[2][2], 0, 
											0, 0, 0, 1);
	auto cameraWorld = XMMatrixMultiply(cameraTrans, cameraRot);
	PPLLObject* ppll = PPLLObject::GetSingleton();
	MarkerRender::GetSingleton()->DrawMarkers(positions, cameraWorld, ppll->m_viewXMMatrix, ppll->m_projXMMatrix);
}

void Hair::UpdateVariables()
{
	logger::info("In hair UpdateVariables");
	if (!m_gotSkeleton)
		return;
	/*hdt::ActorManager::Skeleton* playerSkeleton = hdt::ActorManager::instance()->m_playerSkeleton;
	if (playerSkeleton == NULL)
		return;*/

	PPLLObject* ppll = PPLLObject::GetSingleton();

	//thin tip
	ppll->m_pStrandEffect->GetVariableByName("g_bThinTip")->AsScalar()->SetBool(true);

	//ratio TODO: What does this do?
	ppll->m_pStrandEffect->GetVariableByName("g_Ratio")->AsScalar()->SetFloat(0.5);
	//shading params, cbuffer tressfxShadeParameters
	ppll->m_pStrandEffect->GetVariableByName("g_HairShadowAlpha")->AsScalar()->SetFloat(0.004);
	ppll->m_pStrandEffect->GetVariableByName("g_FiberRadius")->AsScalar()->SetFloat(0.21);
	ppll->m_pStrandEffect->GetVariableByName("g_FiberSpacing")->AsScalar()->SetFloat(0.1);
	DirectX::XMVECTOR matBaseColorVector = DirectX::XMVectorSet(1, 1, 1, 0.63);
	ppll->m_pStrandEffect->GetVariableByName("g_MatBaseColor")->AsVector()->SetFloatVector(reinterpret_cast<float*>(&matBaseColorVector));
	DirectX::XMVECTOR matKValueVector = DirectX::XMVectorSet(0, 0.07, 0.0017, 14.4);
	ppll->m_pStrandEffect->GetVariableByName("g_MatKValue")->AsVector()->SetFloatVector(reinterpret_cast<float*>(&matKValueVector));
	ppll->m_pStrandEffect->GetVariableByName("g_fHairKs2")->AsScalar()->SetFloat(0.072);
	ppll->m_pStrandEffect->GetVariableByName("g_fHairEx2")->AsScalar()->SetFloat(11.8);
	ppll->m_pStrandEffect->GetVariableByName("g_NumVerticesPerStrand")->AsScalar()->SetInt(32);
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
	settings.m_damping = 0.035;
	settings.m_localConstraintStiffness = .8;  //0.8;
	settings.m_globalConstraintStiffness = 0.01;  //0.0;
	settings.m_globalConstraintsRange = 1.0;
	settings.m_gravityMagnitude = 0.09;
	settings.m_vspAccelThreshold = 1.208;
	settings.m_vspCoeff = 0.758;
	settings.m_tipSeparation = 0;
	settings.m_windMagnitude = 0;
	settings.m_windAngleRadians = 0;
	settings.m_windDirection[0] = 0;
	settings.m_windDirection[1] = 0;
	settings.m_windDirection[2] = 0;
	settings.m_localConstraintsIterations = 3;
	settings.m_lengthConstraintsIterations = 6;  //?

	logger::info("Setting sim parameters");
	m_pHairObject->UpdateSimulationParameters(settings);
}
void Hair::Draw(ID3D11DeviceContext* pContext, EI_PSO* pPSO)
{
	m_pHairObject->DrawStrands((EI_CommandContextRef)pContext, *pPSO);
}


void Hair::TransitionRenderingToSim(ID3D11DeviceContext* pContext) {
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
void Hair::UpdateBones()
{
	hdt::ActorManager::Skeleton* playerSkeleton = hdt::ActorManager::instance()->m_playerSkeleton;
	if (playerSkeleton == NULL) {
		logger::warn("Player skeleton is null!");
		return;
	}
	//ListChildren(playerSkeleton->skeleton->GetChildren());
	if (!m_gotSkeleton) {
		//logger::info("Skeleton address: {}", Util::ptr_to_string(playerSkeleton));
		//ListChildren(playerSkeleton->skeleton->GetChildren());
		logger::info("Got player skeleton");
		m_gotSkeleton = true;
		logger::info("Num bones to get: {}", m_numBones);
		for (uint16_t i = 0; i < m_numBones; i++) {
			logger::info("Getting bone, name: {}", m_boneNames[i]);
			if (m_boneNames[i] == "SKIP_NODE") {
				m_pBones[i] = nullptr;
			}
			m_pBones[i] = playerSkeleton->skeleton->GetObjectByName(m_boneNames[i]);
		}
		logger::info("Got all bones");
	}
	//get current transforms now, because they're borked later
	for (uint16_t i = 0; i < m_numBones; i++) {
		//skip unneeded nodes
		if (m_pBones[i] == nullptr) {
			m_boneTransforms[i] = RE::NiTransform();
			continue;
		}
		m_boneTransforms[i] = m_pBones[i]->world;
	}
	for (uint16_t i = 0; i < m_numBones; i++) {
		float size = m_pBones[i]->world.scale;
		Menu::GetSingleton()->DrawFloat(size, "node scale:");
	}
}
bool Hair::Simulate(SkyrimGPUResourceManager* pManager, TressFXSimulation* pSimulation)
{
	//PrintAllD3D11DebugMessages(m_pManager->m_pDevice);
	if (!m_gotSkeleton){
		return false;
	}
	//float scale = playerSkeleton->skeleton->world.scale;
	std::vector<float>* matrices = new std::vector<float>();
	//logger::info("Skeleton has {} bones", playerSkeleton->skeleton->GetChildren().size());
	//auto& children = playerSkeleton->skeleton->GetChildren();
	//ListChildren(children);
	logger::info("num bones: {}", m_numBones);
	for (uint16_t i = 0; i < m_numBones; i++) {
		auto bonePos = m_boneTransforms[i].translate;
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
void Hair::UpdateOffsets(float x, float y, float z, float scale) {
	ID3D11Device* pDevice = RE::BSGraphics::Renderer::GetSingleton()->GetRuntimeData().forwarder;
	ID3D11DeviceContext* pDeviceContext = RE::BSGraphics::Renderer::GetSingleton()->GetRuntimeData().context;
	m_pHairObject->UpdateStrandOffsets(m_hairAsset, (EI_Device*)pDevice, (EI_CommandContextRef)pDeviceContext, x, y, z, scale);
}
void Hair::initialize(SkyrimGPUResourceManager* pManager)
{
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
	pManager = pManager;
	pManager->m_pDevice->CreateTexture2D(&desc, NULL, &m_hairTexture);
	pManager->m_pDevice->CreateShaderResourceView(m_hairTexture, nullptr, &m_hairSRV);
	//create input layout for rendering
	//D3D11_INPUT_ELEMENT_DESC inputDesc;
	//pManager->m_pDevice->CreateInputLayout();
	//compile effects
}
