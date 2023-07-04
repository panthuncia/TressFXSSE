#pragma once
#include "AMD_TressFX.h"
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
	Hair(AMD::TressFXAsset* asset, SkyrimGPUResourceManager* pManager, ID3D11DeviceContext* context, EI_StringHash name);
	void                      UpdateVariables(RE::ThirdPersonState* tps);
	void                      Draw();
	bool                      Simulate();
	void                      RunTestEffect();
	SkyrimGPUResourceManager* m_pManager;

	//static
	static inline std::unordered_map<std::string, Hair*> hairs;
	static inline D3D11_VIEWPORT                         currentViewport;

private:
	static const int                 m_numBones = 8;

	void                      initialize(SkyrimGPUResourceManager* pManager);
	ID3DX11Effect*            create_effect(std::string_view filePath, std::vector<D3D_SHADER_MACRO> defines = std::vector<D3D_SHADER_MACRO>());
	ID3D11Texture2D*          m_hairTexture;
	ID3D11ShaderResourceView* m_hairSRV;
	EI_Resource*              m_hairEIResource;
	ID3D11InputLayout*        renderInputLayout;
	TressFXHairObject*        m_pHairObject;
	ID3DX11Effect*            m_pStrandEffect;
	ID3DX11Effect*            m_pQuadEffect;
	ID3DX11Effect*            m_pTressFXSimEffect;
	ID3DX11Effect*            m_pTressFXSDFCollisionEffect;
	ID3DX11Effect*            m_pTestEffect;
	EI_PSO*                   m_pBuildPSO;
	EI_PSO*                   m_pReadPSO;
	FullscreenPass*           m_pFullscreenPass;
	TressFXPPLL*              m_pPPLL = NULL;
	TressFXSimulation         mSimulation;
	TressFXSDFCollisionSystem mSDFCollisionSystem;
	int                       m_nPPLLNodes;
	ID3D11RasterizerState*    m_pWireframeRSState;
	bool                      m_gotSkeleton = false;
	RE::NiNode*               m_bones[m_numBones];
};
