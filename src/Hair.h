#pragma once
#include <d3d11.h>
#include "TressFXHairObject.h"
#include "AMD_TressFX.h"
#include "SkyrimGPUInterface.h"
#include "ShaderCompiler.h"
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
	ID3D11Texture2D* hairTexture;
	ID3D11ShaderResourceView* hairSRV;
	EI_Resource* hairEIResource;
	TressFXHairObject* hairObject;
	ID3DX11Effect* pStrandEffect;
	EI_PSO* m_pBuildPSO;
	EI_PSO* m_pReadPSO;
};
