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
#include "glm/ext/matrix_common.hpp"

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
	void UpdateLights();
	void Simulate();
	void Draw();
	void DrawShadows();

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
	bool              m_gameLoaded = false; //TODO: remove this hack
	DirectX::XMMATRIX m_cameraWorld;


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
	ID3D11DepthStencilState*  m_pPPLLBuildDepthStencilState;
	ID3D11RasterizerState*    m_pPPLLBuildRasterizerState;
	ID3D11BlendState*         m_pPPLLReadBlendState;
	ID3D11DepthStencilState*  m_pPPLLReadDepthStencilState;
	ID3D11RasterizerState*    m_pPPLLReadRasterizerState;
	ID3D11BlendState*         m_pShadowBlendState;
	ID3D11DepthStencilState*  m_pShadowDepthStencilState;
	ID3D11RasterizerState*    m_pShadowRasterizerState;

	std::vector<DirectX::XMMATRIX> m_lightPositions;

	//lignts
	int       m_nNumLights = 0;
	int       m_nLightShape[SU_MAX_LIGHTS] = { 0 };
	int       m_nLightIndex[SU_MAX_LIGHTS] = { 0 };
	float     m_fLightIntensity[SU_MAX_LIGHTS] = { 0 };
	glm::vec3 m_vLightPosWS[SU_MAX_LIGHTS] = { glm::vec3(0) };
	glm::vec3 m_vLightDirWS[SU_MAX_LIGHTS] = { glm::vec3(0) };
	glm::vec3 m_vLightColor[SU_MAX_LIGHTS] = { glm::vec3(0) };
	glm::vec3 m_vLightConeAngles[SU_MAX_LIGHTS] = { glm::vec3(0) };
	glm::vec3 m_vLightScaleWS[SU_MAX_LIGHTS] = { glm::vec3(0) };
	glm::vec4 m_vLightParams[SU_MAX_LIGHTS] = { glm::vec4(0) };
	glm::vec4 m_vLightOrientationWS[SU_MAX_LIGHTS] = { glm::vec4(0) };

	glm::vec4 m_vSunlightParams = glm::vec4(0);
	glm::vec3 m_vSunlightColor = glm::vec3(0);
	glm::vec3 m_vSunlightDir = glm::vec3(0);

	//global parameter
	float m_gravityMagnitude = 0.09;

		//debug
	ID3D11RasterizerState* m_pWireframeRSState;
	MarkerRender*          m_pMarkerRenderer;

};
