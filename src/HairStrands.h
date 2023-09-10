#pragma once
#include "DX11EngineInterfaceImpl.h"
#include "MarkerRender.h"
#include "ShaderCompiler.h"
#include "TressFX/AMD_TressFX.h"
#include "TressFX/TressFXHairObject.h"
#include "TressFX/TressFXSDFCollision.h"
#include "TressFX/TressFXSimulation.h"
#include <d3d11.h>
#include <nlohmann/json.hpp>

typedef float BoneMatrix[4][4];
#define BONE_MATRIX_SIZE 16 * sizeof(float)
class TressFXPPLL;
class HairStrands
{
public:
	HairStrands(EI_Scene* scene, int skinNumber, int renderIndex, std::string tfxFilePath, std::string tfxboneFilePath, int numFollowHairsPerGuideHair, float tipSeparationFactor, std::string name, std::vector<std::string> boneNames, nlohmann::json data, std::filesystem::path configPath, float initialOffsets[4], std::string userEditorID, TressFXRenderingSettings initialRenderingSettings);
	~HairStrands();
	void               UpdateOffsets(float x, float y, float z, float scale);
	void               UpdateVariables(float gravityMagnitude);
	bool               GetUpdatedBones(EI_CommandContext context);
	void               TransitionRenderingToSim(EI_CommandContext& context);
	void               TransitionSimToRendering(EI_CommandContext& context);
	void               DrawDebugMarkers();
	void               RegisterBones();
	void               UpdateBones();
	void               ExportOffsets(float x, float y, float z, float scale);
	void               ExportParameters(TressFXRenderingSettings renderSettings, TressFXSimulationSettings simSettings);
	void               Reload();
	TressFXHairObject* GetTressFXHandle() { return m_pHairObject.get(); }

	std::string              m_configPath;
	nlohmann::json           m_config;
	std::string              m_hairName;
	TressFXAsset*            m_pHairAsset;
	std::string              m_userEditorID;
	TressFXRenderingSettings m_initialRenderingSettings;

	size_t                     m_numBones = 0;
	std::string                m_boneNames[AMD_TRESSFX_MAX_NUM_BONES];
	RE::NiAVObject*            m_pBones[AMD_TRESSFX_MAX_NUM_BONES];
	RE::NiTransform            m_boneTransforms[AMD_TRESSFX_MAX_NUM_BONES];
	std::vector<AMD::float4x4> m_boneMatrices;
	//tressfx uses both formats for some reason?
	std::vector<DirectX::XMMATRIX> m_boneMatricesXMMATRIX;

	float m_currentOffsets[4] = { 0, 0, 0, 1 };

private:
	void initialize(std::filesystem::path texturePath);
	//ID3D11Texture2D*          m_hairTexture;
	ID3D11Resource*                    m_hairTexture;
	ID3D11ShaderResourceView*          m_hairSRV;
	EI_Resource*                       m_hairEIResource;
	std::unique_ptr<TressFXHairObject> m_pHairObject;
	EI_Scene*                          m_pScene = nullptr;
	int                                m_skinNumber;
	int                                m_renderIndex;

	//if we have obtained bones
	bool m_gotSkeleton = false;
};
