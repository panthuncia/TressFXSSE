#include "Scene.h"
#include "Menu.h"
int EI_Scene::GetBoneIdByName(int skinIndex, const char* boneName) {
	std::vector<std::string> bones = skinIDBoneNamesMap[skinIndex];
	for (int i = 0; i < bones.size(); i++) {
		if (bones[i] == boneName) {
			return i;
		}
	}
	logger::error("encountered incorrect bone name! {}", boneName);
	return -1;
}
std::vector<DirectX::XMMATRIX>& EI_Scene::GetWorldSpaceSkeletonMats(int m_skinNumber) {
	return skinIDBoneTransformsMap[m_skinNumber];
}
