#include "Scene.h"
#include "Menu.h"
int EI_Scene::GetBoneIdByName(int skinIndex, const char* boneName) {
	std::vector<std::string> bones = skinIDBonesMap[skinIndex];
	for (int i = 0; i < bones.size(); i++) {
		if (bones[i] == boneName) {
			return i;
		}
	}
}
