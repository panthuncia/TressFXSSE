#pragma once
#include "HairStrands.h"
#include "TressFX/AMD_TressFX.h"
#include "TressFX/AMD_Types.h"
#include "TressFX/TressFXAsset.h"
#include "TressFX/TressFXCommon.h"
#include "TressFX/TressFXSDFCollision.h"
#include "TressFX/TressFXSettings.h"
class CollisionMesh;
class TressFXPPLL;
class TressFXShortCut;
class CollisionMesh;
class Simulation;
class EI_Scene;

enum state
{
	draw_shadows = 0,
	draw_strands = 1,
	done_drawing = 2
};

struct TressFXSSEData
{
	std::string              m_userEditorID;
	std::filesystem::path    m_configPath;
	nlohmann::json           m_configData;
	float                    m_initialOffsets[4];
	std::vector<std::string> boneNames;
};

struct TressFXObject
{
	std::unique_ptr<HairStrands> hairStrands;
	TressFXSimulationSettings    simulationSettings;
	TressFXRenderingSettings     renderingSettings;
	std::string                  name;
};
// in-memory scene
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
// scene description - no live objects
struct TressFXObjectDescription
{
	std::string               name;
	std::string               tfxFilePath;
	std::string               tfxBoneFilePath;
	std::string               hairObjectName;
	int                       numFollowHairs;
	float                     tipSeparationFactor;
	int                       mesh;
	TressFXSimulationSettings initialSimulationSettings;
	TressFXRenderingSettings  initialRenderingSettings;
	TressFXSSEData            tressfxSSEData;
};

struct TressFXCollisionMeshDescription
{
	std::string name;
	std::string tfxMeshFilePath;
	int         numCellsInXAxis;
	float       collisionMargin;
	int         mesh;
	std::string followBone;
};

struct TressFXSceneDescription
{
	std::vector<TressFXObjectDescription>        objects;
	std::vector<TressFXCollisionMeshDescription> collisionMeshes;

	std::string gltfFilePath;
	std::string gltfFileName;
	std::string gltfBonePrefix;

	float startOffset;
};

class SkyrimTressFX
{
public:
	static SkyrimTressFX* GetSingleton()
	{
		static SkyrimTressFX tfx;
		return &tfx;
	}
	~SkyrimTressFX();

	void OnCreate();
	void Draw();
	void RecreateSizeDependentResources();

	TressFXScene m_activeScene;

	std::unique_ptr<TressFXPPLL>     m_pPPLL;
	std::unique_ptr<TressFXShortCut> m_pShortCut;
	std::unique_ptr<Simulation>      m_pSimulation;

	std::unique_ptr<EI_RenderTargetSet> m_pDebugRenderTargetSet = nullptr;

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
	void ReloadAllHairs();
	void UpdateLights();
	void Simulate(double fTime, bool bUpdateCollMesh, bool bSDFCollisionResponse);
	void Update();
	bool m_doReload = false;

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

	state m_currentState = state::draw_strands;

private:
	void                    DrawHair();
	void                    DrawCollisionMesh();
	void                    GenerateMarchingCubes();
	void                    DrawSDF();
	void                    LoadScene();
	void                    UpdateSimulationParameters();
	void                    UpdateRenderingParameters();
	TressFXSceneDescription LoadTFXUserFiles();
};
