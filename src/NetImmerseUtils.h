#pragma once
#pragma once

//#include <IPrefix.h>
//#include "Ref.h"
#include "RE/N/NiCloningProcess.h"
#include "RE/N/NiGeometry.h"
#include "RE/N/NiExtraData.h"

namespace hdt
{
	/*inline void setBSFixedString(const char** ref, const char* value)
	{
		auto tmp = reinterpret_cast<RE::BSFixedString*>(ref);
		CALL_MEMBER_FN(tmp, Set)(value);
	}*/

	inline void setNiNodeName(RE::NiNode* node, const char* name)
	{
		//setBSFixedString(&node->name, name);
		node->name = name;
	}

	inline RE::NiNode* castNiNode(RE::NiAVObject* obj) { return obj ? obj->AsNode() : nullptr; }
	inline RE::BSTriShape* castBSTriShape(RE::NiAVObject* obj) { return obj ? obj->AsTriShape() : nullptr; }
	inline RE::BSDynamicTriShape* castBSDynamicTriShape(RE::NiAVObject* obj) { return obj ? obj->AsDynamicTriShape() : nullptr; }

	RE::NiNode* addParentToNode(RE::NiNode* node, const char* name);

	RE::NiAVObject* findObject(RE::NiAVObject* obj, const RE::BSFixedString& name);
	RE::NiNode* findNode(RE::NiNode* obj, const RE::BSFixedString& name);

	inline float length(const RE::NiPoint3& a) { return sqrt(a.x * a.x + a.y * a.y + a.z * a.z); }
	inline float distance(const RE::NiPoint3& a, const RE::NiPoint3& b) { return length(a - b); }

	namespace ref
	{
		inline void retain(RE::NiRefObject* object) { object->IncRefCount(); }
		inline void release(RE::NiRefObject* object) { object->DecRefCount(); }
	}

	std::string readAllFile(const char* path);
	std::string readAllFile2(const char* path);

	void updateTransformUpDown(RE::NiAVObject* obj, bool dirty);
}

