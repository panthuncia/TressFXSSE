#pragma once

#include "RE/N/NiObject.h"
#include "EventDispatcherImpl.h"
#include "RE/N/NiNode.h"

namespace hdt
{
	struct SkinAllHeadGeometryEvent
	{
		RE::NiNode* skeleton = nullptr;
		//RE::BSFaceGenNiNode* headNode = nullptr;
		bool hasSkinned = false;
	};

	struct SkinSingleHeadGeometryEvent
	{
		RE::NiNode* skeleton = nullptr;
		RE::BSFaceGenNiNode* headNode = nullptr;
		RE::BSGeometry* geometry = nullptr;
	};

	struct ArmorAttachEvent
	{
		RE::NiNode* armorModel = nullptr;
		RE::NiNode* skeleton = nullptr;
		RE::NiAVObject* attachedNode = nullptr;
		bool hasAttached = false;
	};

	struct FrameEvent
	{
		bool gamePaused;
	};

	struct ShutdownEvent
	{
	};

	extern EventDispatcherImpl<FrameEvent> g_frameEventDispatcher;
	extern EventDispatcherImpl<ShutdownEvent> g_shutdownEventDispatcher;
	extern EventDispatcherImpl<ArmorAttachEvent> g_armorAttachEventDispatcher;
	extern EventDispatcherImpl<SkinAllHeadGeometryEvent> g_skinAllHeadGeometryEventDispatcher;
	extern EventDispatcherImpl<SkinSingleHeadGeometryEvent> g_skinSingleHeadGeometryEventDispatcher;
}
