#include "Hair.h"
#include "ActorManager.h"
#include "SkyrimGPUInterface.h"
#include "TressFXLayouts.h"
#include "TressFXPPLL.h"
#include "Util.h"
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <sys/stat.h>
#include <RE/S/ShadowState.h>
void        PrintAllD3D11DebugMessages(ID3D11Device* d3dDevice);
void        printEffectVariables(ID3DX11Effect* pEffect);
std::string ptr_to_string(void* ptr);
// This could instead be retrieved as a variable from the
// script manager, or passed as an argument.
static const size_t AVE_FRAGS_PER_PIXEL = 4;
static const size_t PPLL_NODE_SIZE = 16;

// See TressFXLayouts.h
// By default, app allocates space for each of these, and TressFX uses it.
// These are globals, because there should really just be one instance.
TressFXLayouts* g_TressFXLayouts = 0;

Hair::Hair(AMD::TressFXAsset* asset, SkyrimGPUResourceManager* resourceManager, ID3D11DeviceContext* context, EI_StringHash name) :
	mSimulation(), mSDFCollisionSystem()
{
	m_pHairObject = new TressFXHairObject;
	m_hairEIResource = new EI_Resource;
	initialize(resourceManager);
	m_hairEIResource->srv = m_hairSRV;
	m_pHairObject->Create(asset, (EI_Device*)resourceManager, (EI_CommandContextRef)context, name, m_hairEIResource);
	logger::info("Created hair object");
	hairs["hairTest"] = this;
}
void Hair::RunTestEffect()
{
	ID3D11DeviceContext* pContext;
	m_pManager->m_pDevice->GetImmediateContext(&pContext);
	//m_pHairObject->GetPosTanCollection().TransitionRenderingToSim((EI_CommandContextRef)pContext);
	//ID3D11ShaderResourceView* SRV_NULL = NULL;
	//ID3D11UnorderedAccessView* UAV_NULL = NULL;
	//pContext->VSSetShaderResources(1, 1, &SRV_NULL);
	//pContext->VSSetShaderResources(2, 1, &SRV_NULL);
	//pContext->CSSetUnorderedAccessViews(0, 1, &UAV_NULL, NULL);
	//pContext->CSSetUnorderedAccessViews(1, 1, &UAV_NULL, NULL);
	//pContext->CSSetUnorderedAccessViews(2, 1, &UAV_NULL, NULL);
	//ID3D11UnorderedAccessView* pUAV;
	ID3D11ShaderResourceView* pSRV;
	m_pStrandEffect->GetVariableByName("g_GuideHairVertexPositions")->AsShaderResource()->GetResource(&pSRV);
	//ID3D11Resource* buffer;
	//pUAV->GetResource(&buffer);
	////D3D11_BUFFER_DESC desc;
	////sb.buffer->GetDesc(&desc);
	//D3D11_MAPPED_SUBRESOURCE mapped_buffer = {};
	//HRESULT hr;
	//ID3D11DeviceContext*     pContext;
	//m_pManager->m_pDevice->GetImmediateContext(&pContext);
	//hr = pContext->Map(sb.buffer, 0u, D3D11_MAP_WRITE, 0u, &mapped_buffer);
	//AMD::TRESSFX::float4* vertices = (AMD::TRESSFX::float4*)mapped_buffer.pData;
	//int numVertices = desc.ByteWidth / sizeof(AMD::TRESSFX::float4);
	//for (int j = 0; j < numVertices; j++) {
	//	logger::info("Vertex: {} {} {}, w: {}", vertices[j].x, vertices[j].y, vertices[j].z, vertices[j].w);
	//}
	//pDeviceContext->Unmap(sb.buffer, 0u);
	D3DX11_EFFECT_VARIABLE_DESC desc;
	m_pTestEffect->GetVariableByIndex(0)->GetDesc(&desc);
	logger::info("Var name: {}", desc.Name);
	m_pTestEffect->GetVariableByName("g_GuideHairVertexPositions")->AsShaderResource()->SetResource(pSRV);
	ID3DX11EffectTechnique* pTechnique = m_pTestEffect->GetTechniqueByName("TressFX2");
	ID3DX11EffectPass*      pPass = pTechnique->GetPassByIndex(0);
	pPass->Apply(0, pContext);
	//int numOfGroupsForCS_StrandLevel = (int)(((float)(m_pHairObject->GetNumTotalHairStrands()) / (float)TRESSFX_SIM_THREAD_GROUP_SIZE));
	pContext->Draw(1000, 0);
	//m_pHairObject->GetPosTanCollection().TransitionSimToRendering((EI_CommandContextRef)pContext);
};
void Hair::UpdateVariables(RE::ThirdPersonState* tps)
{
	logger::info("In UpdateVariables");
	hdt::ActorManager::Skeleton* playerSkeleton = hdt::ActorManager::instance()->m_playerSkeleton;
	if (playerSkeleton == NULL)
		return;
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
	logger::info("Current zoom: {}", tps->currentZoomOffset);
	///*logger::info("WorldToCam:");
	//logger::info("{}, {}, {}, {}", playerCam->GetRuntimeData().worldToCam[0][0], playerCam->GetRuntimeData().worldToCam[0][1], playerCam->GetRuntimeData().worldToCam[0][2], playerCam->GetRuntimeData().worldToCam[0][3]);
	//logger::info("{}, {}, {}, {}", playerCam->GetRuntimeData().worldToCam[1][0], playerCam->GetRuntimeData().worldToCam[1][1], playerCam->GetRuntimeData().worldToCam[1][2], playerCam->GetRuntimeData().worldToCam[1][3]);
	//logger::info("{}, {}, {}, {}", playerCam->GetRuntimeData().worldToCam[2][0], playerCam->GetRuntimeData().worldToCam[2][1], playerCam->GetRuntimeData().worldToCam[2][2], playerCam->GetRuntimeData().worldToCam[2][3]);
	//logger::info("{}, {}, {}, {}", playerCam->GetRuntimeData().worldToCam[3][0], playerCam->GetRuntimeData().worldToCam[3][1], playerCam->GetRuntimeData().worldToCam[3][2], playerCam->GetRuntimeData().worldToCam[3][3]);*/
	RE::NiPoint3 translation = playerCam->world.translate;  //tps->translation;
	//tps->thirdPersonFOVControl->local.translate.z
	//logger::info("Camera position reported by thirdPersonState.translation: {}, {}, {}", translation.x, translation.y, translation.z);
	////translation = playerCam->world.translate;
	////logger::info("Camera position reported by nicamera world.translate: {}, {}, {}", translation.x, translation.y, translation.z);
	RE::NiQuaternion rotation = tps->rotation;
	glm::vec3        cameraPos = glm::vec3(translation.x, translation.y, translation.z);
	glm::vec3        cameraPosScaled = Util::ToRenderScale(cameraPos);
	glm::quat        glm_rotate = glm::quat{ rotation.w, rotation.x, rotation.y, rotation.z };
	float            pitch = glm::pitch(glm_rotate) * -1.0f;
	float            yaw = glm::roll(glm_rotate) * -1.0f;  // The game stores yaw in the Z axis
	glm::vec2        cameraRot = glm::vec2(pitch, yaw);
	//RE::NiMatrix3    rotation_nimatrix = playerCam->world.rotate;  //playerSkeleton->skeleton->world.rotate;
	//float            x = 0;
	//float            y = 0;
	//float            z = 0;
	//rotation_nimatrix.ToEulerAnglesXYZ(x, y, z);
	////glm::mat3 camera_rotation = glm::eulerAngleXYZ(x, y, z);
	//glm::mat4 camera_rotation = glm::mat3({ { rotation_nimatrix.entry[0][0], rotation_nimatrix.entry[0][1], rotation_nimatrix.entry[0][2], 0 },
	//										{ rotation_nimatrix.entry[1][0], rotation_nimatrix.entry[1][1], rotation_nimatrix.entry[1][2], 0 },
	//										{ rotation_nimatrix.entry[2][0], rotation_nimatrix.entry[2][1], rotation_nimatrix.entry[2][2], 0 },
	//										{ 0, 0, 0, 1 } });
	//glm::mat4 camera_translation = glm::mat4(
	//	{ { 1, 0, 0, -cameraPosScaled[0] },
	//		{ 0, 1, 0, -cameraPosScaled[1] },
	//		{ 0, 0, 1, -cameraPosScaled[2] },
	//		{ 0, 0, 0, 1 } });
	////camera_rotation = glm::transpose(camera_rotation);

	glm::mat4 viewMatrix = Util::BuildViewMatrix(cameraPos, cameraRot);
	////glm::mat4 viewMatrix = camera_translation * camera_rotation;
	//logger::info("View:");
	//logger::info("{}, {}, {}, {}", viewMatrix[0][0], viewMatrix[0][1], viewMatrix[0][2], viewMatrix[0][3]);
	//logger::info("{}, {}, {}, {}", viewMatrix[1][0], viewMatrix[1][1], viewMatrix[1][2], viewMatrix[1][3]);
	//logger::info("{}, {}, {}, {}", viewMatrix[2][0], viewMatrix[2][1], viewMatrix[2][2], viewMatrix[2][3]);
	//logger::info("{}, {}, {}, {}", viewMatrix[3][0], viewMatrix[3][1], viewMatrix[3][2], viewMatrix[3][3]);
	////projection matrix
	glm::mat4 projMatrix = Util::GetPlayerProjectionMatrix(playerCam->GetRuntimeData2().viewFrustum, swapDesc.BufferDesc.Width, swapDesc.BufferDesc.Height);
	//logger::info("Proj:");
	//logger::info("{}, {}, {}, {}", projMatrix[0][0], projMatrix[0][1], projMatrix[0][2], projMatrix[0][3]);
	//logger::info("{}, {}, {}, {}", projMatrix[1][0], projMatrix[1][1], projMatrix[1][2], projMatrix[1][3]);
	//logger::info("{}, {}, {}, {}", projMatrix[2][0], projMatrix[2][1], projMatrix[2][2], projMatrix[2][3]);
	//logger::info("{}, {}, {}, {}", projMatrix[3][0], projMatrix[3][1], projMatrix[3][2], projMatrix[3][3]);
	////model-view-projection matrix
	//glm::mat4 viewProjectionMatrix = projMatrix*viewMatrix;

	//glm::mat4 viewProjectionMatrixInverse = glm::inverse(viewProjectionMatrix);

	//auto shadowState = RE::BSGraphics::RendererShadowState::GetSingleton();
	//auto viewMatrix = shadowState->GetRuntimeData().cameraData.getEye().viewMat;
	/*DirectX::XMFLOAT4X4 viewTest;
	DirectX::XMStoreFloat4x4(&viewTest, viewMatrix);
	logger::info("View:");
	logger::info("{}, {}, {}, {}", viewTest._11, viewTest._12, viewTest._13, viewTest._14);
	logger::info("{}, {}, {}, {}", viewTest._21, viewTest._22, viewTest._23, viewTest._24);
	logger::info("{}, {}, {}, {}", viewTest._31, viewTest._32, viewTest._33, viewTest._34);
	logger::info("{}, {}, {}, {}", viewTest._41, viewTest._42, viewTest._43, viewTest._44);*/

	auto viewXMFloat = DirectX::XMFLOAT4X4(&viewMatrix[0][0]);
	auto viewXMMatrix = DirectX::XMMatrixSet(viewXMFloat._11, viewXMFloat._12, viewXMFloat._13, viewXMFloat._14, viewXMFloat._21, viewXMFloat._22, viewXMFloat._23, viewXMFloat._24, viewXMFloat._31, viewXMFloat._32, viewXMFloat._33, viewXMFloat._34, viewXMFloat._41, viewXMFloat._42, viewXMFloat._43, viewXMFloat._44);
	//auto projMatrix = shadowState->GetRuntimeData().cameraData.getEye().projMatrixUnjittered;
	auto                projXMFloat = DirectX::XMFLOAT4X4(&projMatrix[0][0]);
	auto                projXMMatrix = DirectX::XMMatrixSet(projXMFloat._11, projXMFloat._12, projXMFloat._13, projXMFloat._14, projXMFloat._21, projXMFloat._22, projXMFloat._23, projXMFloat._24, projXMFloat._31, projXMFloat._32, projXMFloat._33, projXMFloat._34, projXMFloat._41, projXMFloat._42, projXMFloat._43, projXMFloat._44);
	auto viewProjectionMatrix = DirectX::XMMatrixMultiply(viewXMMatrix, projXMMatrix);
	//auto viewProjectionMatrix = shadowState->GetRuntimeData().cameraData.getEye().viewProjMat;

	/*DirectX::XMFLOAT4X4 viewProjTest;
	DirectX::XMStoreFloat4x4(&viewProjTest, viewProjectionMatrix);
	logger::info("ViewProj:");
	logger::info("{}, {}, {}, {}", viewProjTest._11, viewProjTest._12, viewProjTest._13, viewProjTest._14);
	logger::info("{}, {}, {}, {}", viewProjTest._21, viewProjTest._22, viewProjTest._23, viewProjTest._24);
	logger::info("{}, {}, {}, {}", viewProjTest._31, viewProjTest._32, viewProjTest._33, viewProjTest._34);
	logger::info("{}, {}, {}, {}", viewProjTest._41, viewProjTest._42, viewProjTest._43, viewProjTest._44);*/
	auto viewProjectionMatrixInverse = DirectX::XMMatrixInverse(nullptr, viewProjectionMatrix);
	m_pStrandEffect->GetVariableByName("g_mVP")->AsMatrix()->SetMatrix(reinterpret_cast<float*>(&viewProjectionMatrix));
	m_pStrandEffect->GetVariableByName("g_mInvViewProj")->AsMatrix()->SetMatrix(reinterpret_cast<float*>(&viewProjectionMatrixInverse));
	m_pStrandEffect->GetVariableByName("g_mView")->AsMatrix()->SetMatrix(reinterpret_cast<float*>(&viewMatrix));
	m_pStrandEffect->GetVariableByName("g_mProjection")->AsMatrix()->SetMatrix(reinterpret_cast<float*>(&projMatrix));
	//get camera position
	DirectX::XMVECTOR cameraPosVectorScaled = DirectX::XMVectorSet(cameraPosScaled.x, cameraPosScaled.y, cameraPosScaled.z, 0);
	m_pStrandEffect->GetVariableByName("g_vEye")->AsVector()->SetFloatVector(reinterpret_cast<float*>(&cameraPosVectorScaled));
	//viewport
	DirectX::XMVECTOR viewportVector = DirectX::XMVectorSet(currentViewport.TopLeftX, currentViewport.TopLeftY, currentViewport.Width, currentViewport.Height);
	m_pStrandEffect->GetVariableByName("g_vViewport")->AsVector()->SetFloatVector(reinterpret_cast<float*>(&viewportVector));

	//thin tip
	m_pStrandEffect->GetVariableByName("g_bThinTip")->AsScalar()->SetBool(true);

	//ratio TODO: What does this do?
	m_pStrandEffect->GetVariableByName("g_Ratio")->AsScalar()->SetFloat(0.5);
	//shading params, cbuffer tressfxShadeParameters
	m_pStrandEffect->GetVariableByName("g_HairShadowAlpha")->AsScalar()->SetFloat(0.004);
	m_pStrandEffect->GetVariableByName("g_FiberRadius")->AsScalar()->SetFloat(0.21);
	m_pStrandEffect->GetVariableByName("g_FiberSpacing")->AsScalar()->SetFloat(0.1);
	DirectX::XMVECTOR matBaseColorVector = DirectX::XMVectorSet(1, 1, 1, 0.63);
	m_pStrandEffect->GetVariableByName("g_MatBaseColor")->AsVector()->SetFloatVector(reinterpret_cast<float*>(&matBaseColorVector));
	DirectX::XMVECTOR matKValueVector = DirectX::XMVectorSet(0, 0.07, 0.0017, 14.4);
	m_pStrandEffect->GetVariableByName("g_MatKValue")->AsVector()->SetFloatVector(reinterpret_cast<float*>(&matKValueVector));
	m_pStrandEffect->GetVariableByName("g_fHairKs2")->AsScalar()->SetFloat(0.072);
	m_pStrandEffect->GetVariableByName("g_fHairEx2")->AsScalar()->SetFloat(11.8);
	m_pStrandEffect->GetVariableByName("g_NumVerticesPerStrand")->AsScalar()->SetInt(32);
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
	/*DirectX::XMVECTOR windVector = DirectX::XMVectorSet(0, 0, 0, 0);
	m_pStrandEffect->GetVariableByName("g_Wind")->AsVector()->SetFloatVector(reinterpret_cast<float*>(&windVector));
	DirectX::XMVECTOR wind1Vector = DirectX::XMVectorSet(0, 0, 0, 0);
	m_pStrandEffect->GetVariableByName("g_Wind1")->AsVector()->SetFloatVector(reinterpret_cast<float*>(&wind1Vector));
	DirectX::XMVECTOR wind2Vector = DirectX::XMVectorSet(0, 0, 0, 0);
	m_pStrandEffect->GetVariableByName("g_Wind2")->AsVector()->SetFloatVector(reinterpret_cast<float*>(&wind2Vector));
	DirectX::XMVECTOR wind3Vector = DirectX::XMVectorSet(0, 0, 0, 0);
	m_pStrandEffect->GetVariableByName("g_Wind3")->AsVector()->SetFloatVector(reinterpret_cast<float*>(&wind3Vector));
	DirectX::XMVECTOR shapeVector = DirectX::XMVectorSet(0.035, 0.800, 0, 0);
	m_pStrandEffect->GetVariableByName("g_Shape")->AsVector()->SetFloatVector(reinterpret_cast<float*>(&shapeVector));
	DirectX::XMVECTOR gravTipTimeVector = DirectX::XMVectorSet(0, 0.0908, 0, 0);
	m_pStrandEffect->GetVariableByName("g_GravTipTime")->AsVector()->SetFloatVector(reinterpret_cast<float*>(&gravTipTimeVector));
	DirectX::XMVECTOR simIntsVector = DirectX::XMVectorSetInt(1, 1, 0, 0);
	m_pStrandEffect->GetVariableByName("g_SimInts")->AsVector()->SetIntVector(reinterpret_cast<int*>(&shapeVector));
	DirectX::XMVECTOR vspVector = DirectX::XMVectorSet(0.758, 1.208, 0, 0);
	m_pStrandEffect->GetVariableByName("g_VSP")->AsVector()->SetFloatVector(reinterpret_cast<float*>(&vspVector));*/
	TressFXSimulationSettings settings;
	settings.m_damping = 0.035;
	settings.m_localConstraintStiffness = 1.0;  //0.8;
	settings.m_globalConstraintStiffness = 0.05;  //0.0;
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
	settings.m_lengthConstraintsIterations = 3;  //?

	logger::info("Setting sim parameters");
	m_pHairObject->UpdateSimulationParameters(settings);
}
void Hair::Draw()
{
	//start draw
	PrintAllD3D11DebugMessages(m_pManager->m_pDevice);
	logger::info("Starting TFX Draw");
	ID3D11DeviceContext* pContext;
	m_pManager->m_pDevice->GetImmediateContext(&pContext);
	m_pPPLL->Clear((EI_CommandContextRef)pContext);
	m_pPPLL->BindForBuild((EI_CommandContextRef)pContext);
	//RunTestEffect();
	pContext->RSSetState(m_pWireframeRSState);
	//UINT counter[2] = { 0, 0 };
	//ID3D11UnorderedAccessView* uavs;
	//pContext->OMGetRenderTargetsAndUnorderedAccessViews(0, NULL, NULL, 3, 2, &uavs);
	//pContext->OMSetRenderTargetsAndUnorderedAccessViews(D3D11_KEEP_RENDER_TARGETS_AND_DEPTH_STENCIL, NULL, NULL, 3, 2, &uavs, counter);
	m_pHairObject->DrawStrands((EI_CommandContextRef)pContext, *m_pBuildPSO);
	PrintAllD3D11DebugMessages(m_pManager->m_pDevice);
	logger::info("End of TFX Draw Debug");
	m_pPPLL->DoneBuilding((EI_CommandContextRef)pContext);

	// TODO move this to a clear "after all pos and tan usage by rendering" place.
	m_pHairObject->GetPosTanCollection().TransitionRenderingToSim((EI_CommandContextRef)pContext);
	//necessary?
	//UnbindUAVS();
	m_pPPLL->BindForRead((EI_CommandContextRef)pContext);
	m_pFullscreenPass->Draw(m_pManager, m_pReadPSO);
	m_pPPLL->DoneReading((EI_CommandContextRef)pContext);
	//end draw
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

bool Hair::Simulate()
{
	PrintAllD3D11DebugMessages(m_pManager->m_pDevice);
	logger::info("Starting TFX Simulate");
	if (!m_gotSkeleton) {
		hdt::ActorManager::Skeleton* playerSkeleton = hdt::ActorManager::instance()->m_playerSkeleton;
		logger::info("Skeleton address: {}", ptr_to_string(playerSkeleton));
		if (playerSkeleton != NULL) {
			logger::info("Got player skeleton");
			m_gotSkeleton = true;
			m_bones[0] = playerSkeleton->skeleton->GetObjectByName("NPC Root [Root]")->AsNode();
			logger::info("Got bone 1");
			m_bones[1] = playerSkeleton->skeleton->GetObjectByName("NPC COM [COM ]")->AsNode();
			logger::info("Got bone 2");
			m_bones[2] = playerSkeleton->skeleton->GetObjectByName("NPC Pelvis [Pelv]")->AsNode();
			m_bones[3] = playerSkeleton->skeleton->GetObjectByName("NPC Spine [Spn0]")->AsNode();
			m_bones[4] = playerSkeleton->skeleton->GetObjectByName("NPC Spine1 [Spn1]")->AsNode();
			m_bones[5] = playerSkeleton->skeleton->GetObjectByName("NPC Spine2 [Spn2]")->AsNode();
			m_bones[6] = playerSkeleton->skeleton->GetObjectByName("NPC Neck [Neck]")->AsNode();
			m_bones[7] = playerSkeleton->skeleton->GetObjectByName("NPC Head MagicNode [Hmag]")->AsNode();
			logger::info("Got all bones");
		} else {
			return false;
		}
	}
	//float scale = playerSkeleton->skeleton->world.scale;
	std::vector<float>* matrices = new std::vector<float>();
	//logger::info("Skeleton has {} bones", playerSkeleton->skeleton->GetChildren().size());
	//auto& children = playerSkeleton->skeleton->GetChildren();
	//ListChildren(children);
	for (uint16_t i = 0; i < m_numBones; i++) {
		auto          child = m_bones[i];
		RE::NiPoint3* translation = &child->world.translate;
		glm::vec3     translation_vector_scaled = Util::ToRenderScale(glm::vec3(translation->x, translation->y-10000, translation->z));
		//logger::info("Current bone translation: {}, {}, {}",translation->x, translation->y, translation->z);
		RE::NiMatrix3* rotation_nimatrix = &child->world.rotate;  //playerSkeleton->skeleton->world.rotate;
		float          x = 0;
		float          y = 0;
		float          z = 0;
		rotation_nimatrix->ToEulerAnglesXYZ(x, y, z);
		//glm::mat3 rotation = glm::eulerAngleXYZ(x, y, z);
		glm::mat3      rotation = glm::mat3({ { rotation_nimatrix->entry[0][0], rotation_nimatrix->entry[0][1], rotation_nimatrix->entry[0][2] },
				 { rotation_nimatrix->entry[1][0], rotation_nimatrix->entry[1][1], rotation_nimatrix->entry[1][2] },
				 { rotation_nimatrix->entry[2][0], rotation_nimatrix->entry[2][1], rotation_nimatrix->entry[2][2] } });
		rotation = glm::transpose(rotation);
		matrices->insert(matrices->end(), { rotation[0][0], rotation[0][1], rotation[0][2], translation_vector_scaled.x,
											  rotation[1][0], rotation[1][1], rotation[1][2], translation_vector_scaled.y,
											  rotation[2][0], rotation[2][1], rotation[2][2], translation_vector_scaled.z,
											  0, 0, 0, 1 });

		/*matrices->insert(matrices->end(), { rotation->entry[0][0], rotation->entry[1][0], rotation->entry[2][0], 0,
												  rotation->entry[0][1], rotation->entry[1][1], rotation->entry[2][1], 0,
												  rotation->entry[0][2], rotation->entry[1][2], rotation->entry[2][2], 0,
												  translation->x, translation->y, translation->z, 1 });*/
	}

	logger::info("assembled matrices");
	m_pHairObject->UpdateBoneMatrices((EI_CommandContextRef)m_pManager, &(matrices->front()), BONE_MATRIX_SIZE * m_numBones);
	delete (matrices);
	logger::info("updated matrices");
	ID3D11DeviceContext* context;
	m_pManager->m_pDevice->GetImmediateContext(&context);
	logger::info("Before simulate call");
	mSimulation.Simulate((EI_CommandContextRef)context, *m_pHairObject);
	logger::info("After simulate call");
	PrintAllD3D11DebugMessages(m_pManager->m_pDevice);
	logger::info("End of TFX Simulate Debug");
	ID3D11DeviceContext* pContext;
	m_pManager->m_pDevice->GetImmediateContext(&pContext);
	m_pHairObject->GetPosTanCollection().TransitionSimToRendering((EI_CommandContextRef)pContext);
	logger::info("simulation complete");
	return true;
	return false;
}

ID3DX11Effect* Hair::create_effect(std::string_view filePath, std::vector<D3D_SHADER_MACRO> defines)
{
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
	ID3DX11Effect* pEffect = ShaderCompiler::CompileAndRegisterEffectShader(path.wstring(), m_pManager->m_pDevice, defines);
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
	m_pManager = pManager;
	pManager->m_pDevice->CreateTexture2D(&desc, NULL, &m_hairTexture);
	pManager->m_pDevice->CreateShaderResourceView(m_hairTexture, nullptr, &m_hairSRV);
	//create input layout for rendering
	//D3D11_INPUT_ELEMENT_DESC inputDesc;
	//pManager->m_pDevice->CreateInputLayout();
	//compile effects
	logger::info("Create strand effect");
	std::vector<D3D_SHADER_MACRO> strand_defines = { { "SU_sRGB_TO_LINEAR", "2.2" }, { "SU_LINEAR_SPACE_LIGHTING", "" }, { "SM_CONST_BIAS", "0.000025" }, { "SU_CRAZY_IF", "[flatten] if" }, { "SU_MAX_LIGHTS", "20" }, { "KBUFFER_SIZE", "4" }, { "SU_LOOP_UNROLL", "[unroll]" }, { "SU_LINEAR_TO_sRGB", "0.4545454545" }, { "SU_3D_API_D3D11", "1" }, { NULL, NULL } };
	m_pStrandEffect = create_effect("data\\Shaders\\TressFX\\oHair.fx"sv, strand_defines);
	printEffectVariables(m_pStrandEffect);
	logger::info("Create quad effect");
	std::vector<D3D_SHADER_MACRO> quad_defines = { { "SU_sRGB_TO_LINEAR", "2.2" }, { "SU_LINEAR_SPACE_LIGHTING", "" }, { "SM_CONST_BIAS", "0.000025" }, { "SU_CRAZY_IF", "[flatten] if" }, { "SU_MAX_LIGHTS", "20" }, { "KBUFFER_SIZE", "4" }, { "SU_LOOP_UNROLL", "[unroll]" }, { "SU_LINEAR_TO_sRGB", "0.4545454545" }, { "SU_3D_API_D3D11", "1" }, { NULL, NULL } };
	m_pQuadEffect = create_effect("data\\Shaders\\TressFX\\qHair.fx"sv, quad_defines);
	printEffectVariables(m_pQuadEffect);
	logger::info("Create simulation effect");
	std::vector<D3D_SHADER_MACRO> macros;
	std::string                   s = std::to_string(AMD_TRESSFX_MAX_NUM_BONES);
	macros.push_back({ "AMD_TRESSFX_MAX_NUM_BONES", s.c_str() });
	m_pTressFXSimEffect = create_effect("data\\Shaders\\TressFX\\cTressFXSimulation.fx"sv);
	printEffectVariables(m_pTressFXSimEffect);
	logger::info("Create collision effect");
	m_pTressFXSDFCollisionEffect = create_effect("data\\Shaders\\TressFX\\cTressFXSDFCollision.fx"sv);
	printEffectVariables(m_pTressFXSDFCollisionEffect);
	logger::info("Create test effect");
	m_pTestEffect = create_effect("data\\Shaders\\TressFX\\bufferTest.fx"sv);
	printEffectVariables(m_pTestEffect);
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

	m_pFullscreenPass = new FullscreenPass(pManager);

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

	EI_LayoutManagerRef simLayoutManager = (EI_LayoutManagerRef&)*m_pTressFXSimEffect;
	CreateSimPosTanLayout2((EI_Device*)pManager->m_pDevice, simLayoutManager);
	logger::info("Created SimPosTanLayout");
	CreateSimLayout2((EI_Device*)pManager->m_pDevice, simLayoutManager);
	logger::info("Created SimLayout");

	EI_LayoutManagerRef applySDFManager = (EI_LayoutManagerRef&)*m_pTressFXSDFCollisionEffect;
	CreateApplySDFLayout2((EI_Device*)pManager->m_pDevice, applySDFManager);
	logger::info("Created ApplySDFLayout");

	EI_LayoutManagerRef sdfCollisionManager = (EI_LayoutManagerRef&)*m_pTressFXSDFCollisionEffect;
	CreateGenerateSDFLayout2((EI_Device*)pManager->m_pDevice, sdfCollisionManager);
	logger::info("Created GenerateSDFLayout");

	logger::info("Initialize simulation");
	mSimulation.Initialize((EI_Device*)pManager->m_pDevice, simLayoutManager);
	logger::info("Initialize SDF collision");
	mSDFCollisionSystem.Initialize((EI_Device*)pManager->m_pDevice, sdfCollisionManager);

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
	pManager->m_pDevice->CreateRasterizerState(&rsState, &m_pWireframeRSState);
}
