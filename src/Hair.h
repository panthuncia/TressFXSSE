#pragma once
#include "AMD_TressFX.h"
#include "MarkerRender.h"
#include "ShaderCompiler.h"
#include "SkyrimGPUInterface.h"
#include "TressFXHairObject.h"
#include "TressFXSDFCollision.h"
#include "TressFXSimulation.h"
#include <d3d11.h>
typedef float BoneMatrix[4][4];
#define BONE_MATRIX_SIZE 16 * sizeof(float)
class TressFXPPLL;
class Hair
{
public:
	Hair(AMD::TressFXAsset* asset, SkyrimGPUResourceManager* pManager, ID3D11DeviceContext* context, EI_StringHash name, std::vector<std::string> boneNames);
	~Hair();
	void UpdateOffsets(float x, float y, float z, float scale);
	void UpdateVariables();
	void Draw(ID3D11DeviceContext* pContext, EI_PSO* pPSO);
	bool Simulate(SkyrimGPUResourceManager* pManager, TressFXSimulation* pSimulation);
	void TransitionRenderingToSim(ID3D11DeviceContext* pContext);
	void DrawDebugMarkers();
	void UpdateBones();

	void Reload();

	EI_StringHash      m_hairName;
	AMD::TressFXAsset* m_hairAsset;

	size_t          m_numBones = 0;
	std::string     m_boneNames[AMD_TRESSFX_MAX_NUM_BONES];
	RE::NiAVObject* m_pBones[AMD_TRESSFX_MAX_NUM_BONES];
	RE::NiTransform m_boneTransforms[AMD_TRESSFX_MAX_NUM_BONES];

private:
	void                      initialize(SkyrimGPUResourceManager* pManager);
	ID3D11Texture2D*          m_hairTexture;
	ID3D11ShaderResourceView* m_hairSRV;
	EI_Resource*              m_hairEIResource;
	TressFXHairObject*        m_pHairObject;

	bool m_gotSkeleton = false;
};
