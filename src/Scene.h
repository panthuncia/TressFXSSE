#pragma once

class EI_Scene
{
public:
	struct State
	{
		float  time;
	};
	EI_Scene() {}
	~EI_Scene() {}

	int GetBoneIdByName(int skinIndex, const char* boneName);

	DirectX::XMMATRIX m_viewXMMatrix;
	DirectX::XMMATRIX m_projXMMatrix;
	D3D11_VIEWPORT    m_currentViewport;

	std::unordered_map<int, std::vector<std::string>> skinIDBonesMap;
};
