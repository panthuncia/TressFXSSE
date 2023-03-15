#include "NetImmerseUtils.h"
#include <fstream>
namespace hdt
{
	RE::NiNode* addParentToNode(RE::NiNode* node, const char* name)
	{
		auto parent = node->parent;
		auto newParent = RE::NiNode::Create(1);
		node->IncRefCount();
		if (parent)
		{
			parent->DetachChild(node);
			parent->AttachChild(newParent, false);
		}
		newParent->AttachChild(node, false);
		setNiNodeName(newParent, name);
		node->DecRefCount();
		return newParent;
	}

	RE::NiAVObject* findObject(RE::NiAVObject* obj, const RE::BSFixedString& name)
	{
		return obj->GetObjectByName(name);
	}

	RE::NiNode* findNode(RE::NiNode* obj, const RE::BSFixedString& name)
	{
		auto ret = obj->GetObjectByName(name);
		return ret ? ret->AsNode() : nullptr;
	}

	std::string readAllFile(const char* path)
	{
		RE::BSResourceNiBinaryStream fin(path);
		if (!fin.good()) return "";

		size_t readed;
		char buffer[4096];
		std::string ret;
		do {
			readed = fin.read(buffer, sizeof(buffer));
			ret.append(buffer, readed);
		} while (readed == sizeof(buffer));
		return ret;
	}

	std::string readAllFile2(const char* path)
	{
		std::ifstream fin(path, std::ios::binary);
		if (!fin.is_open()) return "";

		fin.seekg(0, std::ios::end);
		auto size = fin.tellg();
		fin.seekg(0, std::ios::beg);
		std::string ret;
		ret.resize(size);
		fin.read(&ret[0], size);
		return ret;
	}

	void updateTransformUpDown(RE::NiAVObject* obj, bool dirty)
	{
		if (!obj) return;

		RE::NiUpdateData ctx =
		{ 0.f,
			dirty ? RE::NiUpdateData::Flag::kDirty : RE::NiUpdateData::Flag::kNone
		};

		obj->UpdateWorldData(&ctx);

		auto node = castNiNode(obj);

		if (node)
		{
			for (int i = 0; i < node->GetChildren().capacity(); ++i)
			{
				auto child = node->GetChildren().begin()[i];
				if (child) updateTransformUpDown(child.get(), dirty);
			}
		}
	}
}
