#pragma once
#include "Utilities.h"
// 18
class BSFaceGenMorphData : public RE::NiRefObject
{
public:
	void* unk10;	// 10


	MEMBER_FN_PREFIX(BSFaceGenMorphData);
	DEFINE_MEMBER_FN(ApplyMorph, unsigned short, 0x003D7860, const char** morphName, RE::NiAVObject* faceTrishape, float relative, unsigned short unk2);
};
// 20
class RE::BSFaceGenModel : public RE::NiRefObject
{
public:
	struct Data10
	{
		unsigned long				unk00;		// 00
		unsigned long				pad04;		// 04
		RE::NiAVObject* unk08;	// 08
		RE::NiAVObject* unk10;	// 10
		void* unk18;	// 18
		BSFaceGenMorphData * unk20;	// 20
	};

	Data10* unk10;	// 10
	unsigned long	unk18;		// 18
	unsigned long	pad1C;		// 1C

	MEMBER_FN_PREFIX(BSFaceGenModel);
	DEFINE_MEMBER_FN(ctor, void, 0x003D4070);
	DEFINE_MEMBER_FN(CopyFrom, void, 0x003D4150, BSFaceGenModel* other);
	DEFINE_MEMBER_FN(SetModelData, bool, 0x003D47C0, const char* meshPath, void* unk1, unsigned short unk2);
	DEFINE_MEMBER_FN(ApplyMorph, unsigned short, 0x003D4630, RE::BSFixedString* morphName, RE::TESModelTri* triModel, RE::NiAVObject** headNode, float relative, unsigned short unk1);
};
