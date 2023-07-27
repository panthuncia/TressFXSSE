#pragma once
#include "AMD_TressFX.h"
#include "MarkerRender.h"
#include "ShaderCompiler.h"
#include "SkyrimGPUInterface.h"
#include "TressFXHairObject.h"
#include "TressFXSDFCollision.h"
#include "TressFXSimulation.h"
#include <d3d11.h>
#include <nlohmann/json.hpp>

typedef float BoneMatrix[4][4];
#define BONE_MATRIX_SIZE 16 * sizeof(float)
class TressFXPPLL;
class Hair
{
public:
	Hair(AMD::TressFXAsset* asset, SkyrimGPUResourceManager* pManager, ID3D11DeviceContext* context, EI_StringHash name, std::vector<std::string> boneNames, std::filesystem::path texturePath);
	~Hair();
	void UpdateOffsets(float x, float y, float z, float scale);
	void UpdateVariables(float gravityMagnitude);
	//lol
	void SetRenderingAndSimParameters(float fiberRadius, float fiberSpacing, float fiberRatio, float kd, float ks1, float ex1, float ks2, float ex2, int localConstraintsIterations, int lengthConstraintsIterations, float localConstraintsStiffness, float globalConstraintsStiffness, float globalConstraintsRange, float damping, float vspAmount, float vspAccelThreshold, float hairOpacity, float hairShadowAlpha, bool thinTip);
	void Draw(ID3D11DeviceContext* pContext, EI_PSO* pPSO);
	bool Simulate(SkyrimGPUResourceManager* pManager, TressFXSimulation* pSimulation);
	void TransitionRenderingToSim(ID3D11DeviceContext* pContext);
	void DrawDebugMarkers();
	void UpdateBones();
	void ExportOffsets(float x, float y, float z, float scale);
	void ExportParameters();
	void Reload();

	std::string        m_configPath;
	nlohmann::json               m_config;
	EI_StringHash      m_hairName;
	AMD::TressFXAsset* m_hairAsset;

	size_t          m_numBones = 0;
	std::string     m_boneNames[AMD_TRESSFX_MAX_NUM_BONES];
	RE::NiAVObject* m_pBones[AMD_TRESSFX_MAX_NUM_BONES];
	RE::NiTransform m_boneTransforms[AMD_TRESSFX_MAX_NUM_BONES];

private:
	void                      initialize(SkyrimGPUResourceManager* pManager, std::filesystem::path texturePath);
	//ID3D11Texture2D*          m_hairTexture;
	ID3D11Resource*           m_hairTexture;
	ID3D11ShaderResourceView* m_hairSRV;
	EI_Resource*              m_hairEIResource;
	TressFXHairObject*        m_pHairObject;
	float                     m_currentOffsets[4];

	//rendering and sim parameters
	float m_fiberRadius = 0.210;
	float m_fiberSpacing = 0.1;
	float m_fiberRatio = 0.1;
	float m_kd = 0.07;
	float m_ks1 = 0.17;
	float m_ex1 = 14.4;
	float m_ks2 = 0.72;
	float m_ex2 = 11.8;
	int m_localConstraintsIterations = 3;
	int m_lengthConstraintsIterations = 3;
	float m_localConstraintsStiffness = 0.9;
	float m_globalConstraintsStiffness = 0.4;
	float m_globalConstraintsRange = 0.3;
	float m_damping = 0.06;
	float m_vspAmount = 0.75;
	float m_vspAccelThreshold = 1.2;
	float m_hairOpacity = 0.63;
	float m_hairShadowAlpha = 0.35;
	bool  m_thinTip = true;

	//if we have obtained bones
	bool m_gotSkeleton = false;
};
