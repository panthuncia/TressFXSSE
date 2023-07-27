#pragma once
#include "AMD_TressFX.h"
#include "ShaderCompiler.h"
#include "SkyrimGPUInterface.h"
#include "TressFXHairObject.h"
#include "TressFXSDFCollision.h"
#include "TressFXSimulation.h"
#include "MarkerRender.h"
#include <d3d11.h>
#include "Hair.h"

#define SU_MAX_LIGHTS 20
class TressFXPPLL;
class PPLLObject
{
public:
	~PPLLObject();

	static PPLLObject* GetSingleton()
	{
		static PPLLObject ppll;
		return &ppll;
	}
	void Initialize();
	void UpdateVariables();
	void Simulate();
	void Draw();

	void ReloadAllHairs();
	bool m_doReload = false;

	std::unordered_map<std::string, Hair*> m_hairs;
	D3D11_VIEWPORT    m_currentViewport;
	
	//for testing
	DirectX::XMMATRIX m_gameViewMatrix;
	DirectX::XMMATRIX m_gameProjMatrix;
	DirectX::XMMATRIX m_gameViewProjMatrix;
	DirectX::XMMATRIX m_gameProjMatrixUnjittered;

	DirectX::XMMATRIX m_viewXMMatrix;
	DirectX::XMMATRIX m_projXMMatrix;
	ID3DX11Effect*    m_pStrandEffect;
	ID3DX11Effect*    m_pQuadEffect;


private:
	PPLLObject();
	ID3DX11Effect*            m_pTressFXSimEffect;
	ID3DX11Effect*            m_pTressFXSDFCollisionEffect;
	EI_PSO*                   m_pBuildPSO;
	EI_PSO*                   m_pReadPSO;
	FullscreenPass*           m_pFullscreenPass;
	TressFXPPLL*              m_pPPLL = NULL;
	TressFXSimulation         m_Simulation;
	TressFXSDFCollisionSystem m_SDFCollisionSystem;
	int                       m_nPPLLNodes;
	SkyrimGPUResourceManager* m_pManager;
	ID3D11BlendState*         m_pPPLLBuildBlendState;
	ID3D11BlendState*         m_pPPLLReadBlendState;
	ID3D11DepthStencilState*  m_pPPLLReadDepthStencilState;
	ID3D11RasterizerState*    m_pPPLLReadRasterizerState;

		//debug
	ID3D11RasterizerState* m_pWireframeRSState;
	MarkerRender*          m_pMarkerRenderer;

};
