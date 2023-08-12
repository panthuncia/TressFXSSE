#pragma once
#include "TressFX/TressFXAsset.h"
#include "TressFX/AMD_Types.h"
#include "TressFX/AMD_TressFX.h"
#include "TressFX/TressFXCommon.h"
#include "TressFX/TressFXSDFCollision.h"
#include "TressFX/TressFXSettings.h"
#include "Hair.h"
class CollisionMesh;
class TressFXPPLL;
class TressFXShortCut;
class CollisionMesh;
class Simulation;
class EI_Scene;
typedef HairStrands Hair;
// in-memory scene
struct TressFXObject
{
	std::unique_ptr<HairStrands> hairStrands;
	TressFXSimulationSettings    simulationSettings;
	TressFXRenderingSettings     renderingSettings;
	std::string                  name;
};

struct TressFXScene
{
	std::vector<TressFXObject>                  objects;
	std::vector<std::unique_ptr<CollisionMesh>> collisionMeshes;

	TressFXUniformBuffer<TressFXViewParams> viewConstantBuffer;
	std::unique_ptr<EI_BindSet>             viewBindSet;

	TressFXUniformBuffer<TressFXViewParams> shadowViewConstantBuffer;
	std::unique_ptr<EI_BindSet>             shadowViewBindSet;

	TressFXUniformBuffer<TressFXLightParams> lightConstantBuffer;
	std::unique_ptr<EI_BindSet>              lightBindSet;

	std::unique_ptr<EI_Scene> scene;
};

class SkyrimTressFX {
public:
	SkyrimTressFX();
	~SkyrimTressFX();

	void Draw();
	void RecreateSizeDependentResources();

	TressFXScene m_activeScene;
	
	std::unique_ptr<TressFXPPLL>     m_pPPLL;
	std::unique_ptr<TressFXShortCut> m_pShortCut;
	std::unique_ptr<Simulation>      m_pSimulation;

	enum OITMethod
	{
		OIT_METHOD_PPLL,
		OIT_METHOD_SHORTCUT
	};
	OITMethod m_eOITMethod;
	int       m_nScreenWidth;
	int       m_nScreenHeight;
	int       m_nPPLLNodes;

	void InitializeLayouts();
	void DestroyLayouts();
	void SetOITMethod(OITMethod method);
	void DestroyOITResources(OITMethod method);

	float  m_time;       // WallClock in seconds.
	float  m_deltaTime;  // The elapsed time in milliseconds since the previous frame.
	double m_lastFrameTime;
	bool   m_PauseAnimation = false;
	bool   m_PauseSimulation = false;
	bool   m_drawHair = true;
	bool   m_drawModel = true;
	bool   m_drawCollisionMesh = false;
	bool   m_drawMarchingCubes = false;
	bool   m_generateSDF = true;
	bool   m_collisionResponse = true;
	bool   m_useDepthApproximation = true;

private:
	void DrawHair();
};
