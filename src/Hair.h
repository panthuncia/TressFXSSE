#pragma once
#include <d3d11.h>
#include "TressFXHairObject.h"
#include "AMD_TressFX.h"
#include "SkyrimGPUInterface.h"
#include "ShaderCompiler.h"
class TressFXPPLL;
class Hair
{
public:
	Hair(AMD::TressFXAsset* asset, SkyrimGPUResourceManager* pManager, ID3D11DeviceContext* context, EI_StringHash name);
	void draw();
	SkyrimGPUResourceManager* m_pManager;

	//static
	static inline std::unordered_map<std::string, Hair*> hairs;
private:
	void initialize(SkyrimGPUResourceManager* pManager);
	ID3DX11Effect* create_effect(std::string_view filePath);
	ID3D11Texture2D* m_hairTexture;
	ID3D11ShaderResourceView* m_hairSRV;
	EI_Resource* m_hairEIResource;
	TressFXHairObject* m_hairObject;
	ID3DX11Effect* m_pStrandEffect;
	ID3DX11Effect* m_pQuadEffect;
	EI_PSO* m_pBuildPSO;
	EI_PSO* m_pReadPSO;
	TressFXPPLL* m_pPPLL;
	int m_nPPLLNodes;
};
