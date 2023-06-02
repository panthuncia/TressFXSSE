#pragma once
#include <d3d11.h>
#include "TressFXHairObject.h"
#include "AMD_TressFX.h"
#include "TressFXSimulation.h"
#include "TressFXSDFCollision.h"
#include "SkyrimGPUInterface.h"
#include "ShaderCompiler.h"
typedef float BoneMatrix[4][4];
#define BONE_MATRIX_SIZE 16 * sizeof(float)
class TressFXPPLL;
class Hair
{
public:
	Hair(AMD::TressFXAsset* asset, SkyrimGPUResourceManager* pManager, ID3D11DeviceContext* context, EI_StringHash name);
	void UpdateVariables();
	void Draw();
	void Simulate();
	SkyrimGPUResourceManager* m_pManager;

	//static
	static inline std::unordered_map<std::string, Hair*> hairs;
	static inline D3D11_VIEWPORT currentViewport;

private:
	void initialize(SkyrimGPUResourceManager* pManager);
	ID3DX11Effect* create_effect(std::string_view filePath, std::vector<D3D_SHADER_MACRO> defines = std::vector<D3D_SHADER_MACRO>());
	ID3D11Texture2D* m_hairTexture;
	ID3D11ShaderResourceView* m_hairSRV;
	EI_Resource* m_hairEIResource;
	TressFXHairObject* m_pHairObject;
	ID3DX11Effect* m_pStrandEffect;
	ID3DX11Effect* m_pQuadEffect;
	ID3DX11Effect* m_pTressFXSimEffect;
	ID3DX11Effect* m_pTressFXSDFCollisionEffect;
	EI_PSO* m_pBuildPSO;
	EI_PSO* m_pReadPSO;
	FullscreenPass* m_pFullscreenPass;
	TressFXPPLL* m_pPPLL = NULL;
	TressFXSimulation         mSimulation;
	TressFXSDFCollisionSystem mSDFCollisionSystem;
	int m_nPPLLNodes;
};
