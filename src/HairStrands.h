#pragma once
#include "TressFX/AMD_TressFX.h"
#include "MarkerRender.h"
#include "ShaderCompiler.h"
#include "TressFX/TressFXHairObject.h"
#include "TressFX/TressFXSDFCollision.h"
#include "TressFX/TressFXSimulation.h"
#include <d3d11.h>
#include <nlohmann/json.hpp>
#include "DX11EngineInterfaceImpl.h"

typedef float BoneMatrix[4][4];
#define BONE_MATRIX_SIZE 16 * sizeof(float)
class TressFXPPLL;
class HairStrands
{
public:
	HairStrands(EI_Scene* scene, int skinNumber, int renderIndex, std::string tfxFilePath, std::string tfxboneFilePath, int numFollowHairsPerGuideHair, float tipSeparationFactor, std::string name, std::vector<std::string> boneNames, nlohmann::json data, std::filesystem::path configPath, float initialOffsets[4], std::string userEditorID);
	~HairStrands();
	void UpdateOffsets(float x, float y, float z, float scale);
	void UpdateVariables(float gravityMagnitude);
	//lol
	void SetRenderingAndSimParameters(float fiberRadius, float fiberSpacing, float fiberRatio, float kd, float ks1, float ex1, float ks2, float ex2, int localConstraintsIterations, int lengthConstraintsIterations, float localConstraintsStiffness, float globalConstraintsStiffness, float globalConstraintsRange, float damping, float vspAmount, float vspAccelThreshold, float hairOpacity, float hairShadowAlpha, bool thinTip);
	bool GetUpdatedBones(EI_CommandContext context);
	void TransitionRenderingToSim(EI_CommandContext& context);
	void TransitionSimToRendering(EI_CommandContext& context);
	void DrawDebugMarkers();
	void RegisterBones();
	void UpdateBones();
	void ExportOffsets(float x, float y, float z, float scale);
	void ExportParameters();
	void Reload();
	TressFXHairObject* GetTressFXHandle() { return m_pHairObject.get(); }

	std::string        m_configPath;
	nlohmann::json     m_config;
	std::string      m_hairName;
	TressFXAsset*	  m_pHairAsset;
	std::string       m_userEditorID;

	size_t          m_numBones = 0;
	std::string     m_boneNames[AMD_TRESSFX_MAX_NUM_BONES];
	RE::NiAVObject* m_pBones[AMD_TRESSFX_MAX_NUM_BONES];
	RE::NiTransform m_boneTransforms[AMD_TRESSFX_MAX_NUM_BONES];
	std::vector<AMD::float4x4> m_boneMatrices;
	//tressfx uses both formats for some reason?
	std::vector<DirectX::XMMATRIX> m_boneMatricesXMMATRIX;

private:
	void                      initialize(std::filesystem::path texturePath);
	//ID3D11Texture2D*          m_hairTexture;
	ID3D11Resource*           m_hairTexture;
	ID3D11ShaderResourceView* m_hairSRV;
	EI_Resource*              m_hairEIResource;
	std::unique_ptr<TressFXHairObject> m_pHairObject;
	EI_Scene*                          m_pScene = nullptr;
	int                                m_skinNumber;
	int                                m_renderIndex;
	float                     m_currentOffsets[4];

	//rendering and sim parameters
	float m_fiberRadius = 0.002f;
	float m_fiberSpacing = 0.1f;
	float m_fiberRatio = 0.5f;
	float m_kd = 0.07f;
	float m_ks1 = 0.17f;
	float m_ex1 = 14.4f;
	float m_ks2 = 0.72f;
	float m_ex2 = 11.8f;
	int m_localConstraintsIterations = 3;
	int m_lengthConstraintsIterations = 3;
	float m_localConstraintsStiffness = 0.9f;
	float m_globalConstraintsStiffness = 0.4f;
	float m_globalConstraintsRange = 0.3f;
	float m_damping = 0.06f;
	float m_vspAmount = 0.75f;
	float m_vspAccelThreshold = 1.2f;
	float m_hairOpacity = 0.63f;
	float m_hairShadowAlpha = 0.35f;
	bool  m_thinTip = true;

	//if we have obtained bones
	bool m_gotSkeleton = false;
};
