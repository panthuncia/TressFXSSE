#pragma once
#include "TressFX/TressFXCommon.h"
class EI_Scene
{
public:
	struct State
	{
		float time;
	};
	EI_Scene() {}
	~EI_Scene() {}
	int                             GetBoneIdByName(int skinIndex, const char* boneName);
	std::vector<DirectX::XMMATRIX>& GetWorldSpaceSkeletonMats(int m_skinNumber);
	AMD::float4x4                   GetMV() const { return m_viewMatrix; }
	AMD::float4x4                   GetMVP() const { return m_viewProjMatrix; }
	AMD::float4                     GetCameraPos() { return m_cameraPos; };

	AMD::float4x4 m_viewMatrix;
	AMD::float4x4 m_projMatrix;
	AMD::float4x4 m_viewProjMatrix;
	D3D11_VIEWPORT    m_currentViewport;
	AMD::float4       m_cameraPos;

	std::unordered_map<int, std::vector<DirectX::XMMATRIX>> skinIDBoneTransformsMap;
	std::unordered_map<int, std::vector<std::string>>       skinIDBoneNamesMap;
};
