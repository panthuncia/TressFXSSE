#include "ActorManager.h"

#include "ActorManager.h"

#include <cinttypes>
#include "Offsets.h"


#include "HookEvents.h"
#include "NetImmerseUtils.h"
#include "RE/RTTI.h"
#include "RE/M/MenuOpenCloseEvent.h"
#include "RE/N/NiCloningProcess.h"
#include "RE/P/PlayerCharacter.h"
#include "RE/A/Actor.h"
#include "BSFaceGenModel.h"
#include "NiStreamEx.h"
#pragma warning(push)
#pragma warning(disable: 4100)
#pragma warning(disable: 4189)
#pragma warning(disable: 4244)
#define noop
namespace hdt
{
	ActorManager::ActorManager()
	{

	}

	ActorManager::~ActorManager()
	{
	}

	ActorManager* ActorManager::instance()
	{
		static ActorManager s;
		return &s;
	}

	std::string ActorManager::armorPrefix(ActorManager::IDType id)
	{
		char buffer[128];
		sprintf_s(buffer, "hdtSSEPhysics_AutoRename_Armor_%08X ", id);
		return std::string(buffer);
	}

	std::string ActorManager::headPrefix(ActorManager::IDType id)
	{
		char buffer[128];
		sprintf_s(buffer, "hdtSSEPhysics_AutoRename_Head_%08X ", id);
		return std::string(buffer);
	}

	inline bool isFirstPersonSkeleton(RE::NiNode* npc)
	{
		if (!npc) return false;
		return findNode(npc, "Camera1st [Cam1]") ? true : false;
	}

	RE::NiNode* getNpcNode(RE::NiNode* skeleton)
	{
		// TODO: replace this with a generic skeleton fixing configuration option
		// hardcode an exception for lurker skeletons because they are made incorrectly
		auto shouldFix = false;
		if (skeleton->GetUserData() != NULL && skeleton->GetUserData()->GetBaseObject())
		{
			//auto npcForm = DYNAMIC_CAST(skeleton->GetUserData()->data.objectReference, RE::TESForm, RE::TESNPC);
			auto npcForm = skyrim_cast<RE::TESNPC*>(skeleton->GetUserData()->GetBaseObject());
			if (npcForm && npcForm->race
				&& !strcmp(npcForm->race->skeletonModels[0].GetModel(), "Actors\\DLC02\\BenthicLurker\\Character Assets\\skeleton.nif"))
				shouldFix = true;
		}
		return findNode(skeleton, shouldFix ? "NPC Root [Root]" : "NPC");
	}

	// TODO Shouldn't there be an ArmorDetachEvent?
	void ActorManager::onEvent(const ArmorAttachEvent& e)
	{
	}

	// @brief This happens on a closing RaceSex menu, and on 'smp reset'.
	void ActorManager::onEvent(const RE::MenuOpenCloseEvent&)
	{
		// The ActorManager members are protected from parallel events by ActorManager.m_lock.
		std::lock_guard<decltype(m_lock)> l(m_lock);
		if (m_shutdown) return;

		setSkeletonsActive();

		for (auto& i : m_skeletons)
			noop;//i.reloadMeshes();
	}

	void ActorManager::onEvent(const FrameEvent& e)
	{
		// Other events have to be managed. The FrameEvent is the only event that we can drop,
		// we always have one later where we'll be able to manage the passed time.
		// We drop this execution when the lock is already taken; in that case, we would execute the code later.
		// It is better to drop it now, and let the next frame manage it.
		// Moreover, dropping a locked part of the code allows to reduce the total wait times.
		// Finally, some skse mods issue FrameEvents, this mechanism manages the case where they issue too many.
		std::unique_lock<decltype(m_lock)> lock(m_lock, std::try_to_lock);
		if (!lock.owns_lock()) return;

		setSkeletonsActive();
	}

	//NiAVObject* Actor::CalculateLOS_1405FD2C0(Actor *aActor, NiPoint3 *aTargetPosition, NiPoint3 *aRayHitPosition, float aViewCone)
	//Used to ray cast from the actor. Will return nonNull if it hits something with position at aTargetPosition.
	//Pass in 2pi to aViewCone to ignore LOS of actor.
	/*typedef RE::NiAVObject* (*_Actor_CalculateLOS)(RE::Actor* aActor, RE::NiPoint3* aTargetPosition, RE::NiPoint3* aRayHitPosition, float aViewCone);
	RelocAddr<_Actor_CalculateLOS> Actor_CalculateLOS(offset::Actor_CalculateLOS);*/

	// @brief This function is called by different events, with different locking needs, and is therefore extracted from the events.
	void ActorManager::setSkeletonsActive()
	{
		if (m_shutdown) return;

		// We get the player character and its cell.
		// TODO Isn't there a more performing way to find the PC?? A singleton? And if it's the right way, why isn't it in utils functions?
		const auto& playerCharacter = std::find_if(m_skeletons.begin(), m_skeletons.end(), [](Skeleton& s) { return s.isPlayerCharacter(); });
		auto playerCell = (playerCharacter != m_skeletons.end() && playerCharacter->skeleton->parent) ? playerCharacter->skeleton->parent->parent : nullptr;

		// We get the camera, its position and orientation.
		// TODO Can this be reconciled between VR and AE/SE?
#ifndef SKYRIMVR
		const auto cameraNode = RE::PlayerCamera::GetSingleton()->cameraRoot;
#else
		// Camera info taken from Shizof's cpbc under MIT. https://www.nexusmods.com/skyrimspecialedition/mods/21224?tab=files
		if (!(*g_thePlayer)->loadedState)
			return;
		const auto cameraNode = (*g_thePlayer)->loadedState->node;
#endif
		const auto cameraTransform = cameraNode->world;
		const auto cameraPosition = cameraTransform.translate;
		const auto cameraOrientation = cameraTransform.rotate * RE::NiPoint3(0., 1., 0.); // The camera matrix is relative to the world.

		std::for_each(m_skeletons.begin(), m_skeletons.end(), [&](Skeleton& skel)
			{
				skel.calculateDistanceAndOrientationDifferenceFromSource(cameraPosition, cameraOrientation);
			});

		for (auto& i : m_skeletons)
		{
			//i.cleanArmor();
			i.cleanHead();
		}

	}

	void ActorManager::onEvent(const ShutdownEvent&)
	{
		m_shutdown = true;
		std::lock_guard<decltype(m_lock)> l(m_lock);

		m_skeletons.clear();
	}

	void ActorManager::onEvent(const SkinSingleHeadGeometryEvent& e)
	{
		// This case never happens to a lurker skeleton, thus we don't need to test.
		auto npc = findNode(e.skeleton, "NPC");
		if (!npc) return;
		std::lock_guard<decltype(m_lock)> l(m_lock);
		if (m_shutdown) return;
		auto& skeleton = getSkeletonData(e.skeleton);
		skeleton.npc = getNpcNode(e.skeleton);
		//logger::info("4, headNode is {}, geometry is {}", e.headNode, e.geometry);
		//skeleton.processGeometry(e.headNode, e.geometry);
		auto headPartIter = std::find_if(skeleton.head.headParts.begin(), skeleton.head.headParts.end(),
			[e](const Head::HeadPart& p)
			{
				return p.headPart == e.geometry;
			});
		if (headPartIter != skeleton.head.headParts.end())
		{
			if (headPartIter->origPartRootNode)
			{

#ifdef _DEBUG
				logger::info("renaming nodes in original part {} back", headPartIter->origPartRootNode->name);
#endif // _DEBUG

				for (auto& entry : skeleton.head.renameMap)
				{
					// This case never happens to a lurker skeleton, thus we don't need to test.
					auto node = findNode(headPartIter->origPartRootNode, entry.second.c_str());
					if (node)
					{
#ifdef _DEBUG
						logger::info("rename node %s -> %s", entry.second.c_str(), entry.first.c_str());
#endif // _DEBUG
						setNiNodeName(node, entry.first.c_str());
					}
				}
			}
			headPartIter->origPartRootNode = nullptr;
		}
		//if (!skeleton.head.isFullSkinning)
			//skeleton.scanHead();
	}

	void ActorManager::onEvent(const SkinAllHeadGeometryEvent& e)
	{
		logger::info("ActorManager SkinAllHeadGeometryEvent recieved!");
		// This case never happens to a lurker skeleton, thus we don't need to test.
		auto npc = findNode(e.skeleton, "NPC");
		if (!npc) return;
		std::lock_guard<decltype(m_lock)> l(m_lock);
		if (m_shutdown) return;

		auto& skeleton = getSkeletonData(e.skeleton);
		skeleton.npc = npc;
		if (e.skeleton->GetUserData())
			skeleton.skeletonOwner = RE::NiPointer(e.skeleton->GetUserData());
		if (e.skeleton->GetUserData()->formID == 20) {
			logger::info("Found player skeleton");
			m_playerSkeleton = &skeleton;
		}
		if (e.hasSkinned)
		{
			//skeleton.scanHead();
			skeleton.head.isFullSkinning = false;
			if (skeleton.head.npcFaceGeomNode)
			{
				logger::info("npc face geom no longer needed, clearing ref");
				skeleton.head.npcFaceGeomNode = nullptr;
			}
		}
		else
		{
			skeleton.head.isFullSkinning = true;
		}
	}

	std::vector<ActorManager::Skeleton>& ActorManager::getSkeletons()
	{
		return m_skeletons;
	}

#ifdef ANNIVERSARY_EDITION
	bool ActorManager::skeletonNeedsParts(RE::NiNode* skeleton)
	{
		return !isFirstPersonSkeleton(skeleton);
		/*
		auto iter = std::find_if(m_skeletons.begin(), m_skeletons.end(), [=](Skeleton& i)
		{
			return i.skeleton == skeleton;
		});
		if (iter != m_skeletons.end())
		{
			return (iter->head.headNode == 0);
		}
		*/
	}
#endif
	ActorManager::Skeleton& ActorManager::getSkeletonData(RE::NiNode* skeleton)
	{
		auto iter = std::find_if(m_skeletons.begin(), m_skeletons.end(), [=](Skeleton& i)
			{
				return i.skeleton == skeleton;
			});
		if (iter != m_skeletons.end())
		{
			return *iter;
		}
		if (!isFirstPersonSkeleton(skeleton))
		{
			auto ownerIter = std::find_if(m_skeletons.begin(), m_skeletons.end(), [=](Skeleton& i)
				{
					return !isFirstPersonSkeleton(i.skeleton) && i.skeletonOwner && skeleton->GetUserData() && i.skeletonOwner ==
					RE::NiPointer(skeleton->GetUserData());
				});
			if (ownerIter != m_skeletons.end())
			{
#ifdef _DEBUG
				logger::info("new skeleton found for formid %08x", skeleton->GetUserData()->formID);
#endif // _DEBUG
				ownerIter->cleanHead(true);
			}
		}
		m_skeletons.push_back(Skeleton());
		m_skeletons.back().skeleton = skeleton;
		return m_skeletons.back();
	}

	void ActorManager::Skeleton::doSkeletonMerge(RE::NiNode* dst, RE::NiNode* src, std::string* prefix,
		std::unordered_map<std::string, std::string>& map)
	{
		for (int i = 0; i < src->GetChildren().capacity(); ++i)
		{
			auto srcChild = castNiNode(src->GetChildren().begin()[i].get());
			if (!srcChild) continue;

			if (!(srcChild->name == NULL))
			{
				doSkeletonMerge(dst, srcChild, prefix, map);
				continue;
			}

			// FIXME: This was previously only in doHeadSkeletonMerge.
			// But surely non-head skeletons wouldn't have this anyway?
			if (!strcmp(srcChild->name.c_str(), "BSFaceGenRE::NiNodeSkinned"))
			{
#ifdef _DEBUG
				logger::info("skipping facegen RE::NiNode in skeleton merge");
#endif // _DEBUG
				continue;
			}

			// TODO check it's not a lurker skeleton
			auto dstChild = findNode(dst, srcChild->name);
			if (dstChild)
			{
				doSkeletonMerge(dstChild, srcChild, prefix, map);
			}
			else
			{
				dst->AttachChild(cloneNodeTree(srcChild, prefix, map), false);
			}
		}
	}

	RE::NiNode* ActorManager::Skeleton::cloneNodeTree(RE::NiNode* src, std::string* prefix, std::unordered_map<std::string, std::string>& map)
	{
		RE::NiCloningProcess c;
		auto ret = static_cast<RE::NiNode*>(src->CreateClone(c));
		src->ProcessClone(c);

		// FIXME: cloneHeadNodeTree just did this for ret, not both. Don't know if that matters. Armor parts need it on both.
		renameTree(src, prefix, map);
		renameTree(ret, prefix, map);

		return ret;
	}

	void ActorManager::Skeleton::renameTree(RE::NiNode* root, std::string* prefix, std::unordered_map<std::string, std::string>& map)
	{
		if (root->name != NULL)
		{
			std::string newName(prefix->c_str(), prefix->size());
			newName += root->name;
#ifdef _DEBUG
			if (map.insert(std::make_pair<std::string, std::string>(std::string(root->name), newName.c_str())).second)
				logger::info("Rename Bone %s -> %s", root->name, newName.c_str());
#else
			map.insert(std::make_pair<std::string, std::string>(std::string(root->name), newName.c_str()));
#endif // _DEBUG

			setNiNodeName(root, newName.c_str());
		}

		for (int i = 0; i < root->GetChildren().capacity(); ++i)
		{
			auto child = castNiNode(root->GetChildren().begin()[i].get());
			if (child)
				renameTree(child, prefix, map);
		}
	}

	void ActorManager::Skeleton::doSkeletonClean(RE::NiNode* dst, std::string* prefix)
	{
		for (int i = dst->GetChildren().capacity() - 1; i >= 0; --i)
		{
			auto child = castNiNode(dst->GetChildren().begin()[i].get());
			if (!child) continue;

			if (child->name != NULL && !strncmp(child->name.c_str(), prefix->c_str(), prefix->size()))
			{
				dst->DetachChildAt(i++);
			}
			else
			{
				doSkeletonClean(child, prefix);
			}
		}
	}

	// returns the name of the skeleton owner
	std::string ActorManager::Skeleton::name()
	{
		if (skeleton->GetUserData() && skeleton->GetUserData()->GetBaseObject()) {
			auto bname = skyrim_cast<RE::TESNPC*>(skeleton->GetUserData()->GetBaseObject());;
			if (bname)
				return bname->GetName();
		}
		return "";
	}

	void ActorManager::Skeleton::cleanHead(bool cleanAll)
	{
		for (auto& headPart : head.headParts)
		{
			if (!headPart.headPart->parent || cleanAll)
			{
#ifdef _DEBUG
				if (cleanAll)
					logger::info("cleaning headpart %s due to clean all", headPart.headPart->name);
				else
					logger::info("headpart %s disconnected", headPart.headPart->name);
#endif // _DEBUG

				auto renameIt = this->head.renameMap.begin();

				while (renameIt != this->head.renameMap.end())
				{
					bool erase = false;

					if (headPart.renamedBonesInUse.count(renameIt->first) != 0)
					{
						auto findNode = this->head.nodeUseCount.find(renameIt->first);
						if (findNode != this->head.nodeUseCount.end())
						{
							findNode->second -= 1;
#ifdef _DEBUG
							logger::info("decrementing use count by 1, it is now %d", findNode->second);
#endif // _DEBUG
							if (findNode->second <= 0)
							{
#ifdef _DEBUG
								logger::info("node no longer in use, cleaning from skeleton");
#endif // _DEBUG
								auto removeObj = findObject(npc, renameIt->second.c_str());
								if (removeObj)
								{
#ifdef _DEBUG
									logger::info("found node %s, removing", removeObj->name);
#endif // _DEBUG
									auto parent = removeObj->parent;
									if (parent)
									{
										parent->DetachChild(removeObj);
										removeObj->DecRefCount();
									}
								}
								this->head.nodeUseCount.erase(findNode);
								erase = true;
							}
						}
					}

					if (erase)
						renameIt = this->head.renameMap.erase(renameIt);
					else
						++renameIt;
				}

				headPart.headPart = nullptr;
				headPart.origPartRootNode = nullptr;
				//headPart.clearPhysics();
				headPart.renamedBonesInUse.clear();
			}
		}

		head.headParts.erase(std::remove_if(head.headParts.begin(), head.headParts.end(),
			[](Head::HeadPart& i) { return !i.headPart; }), head.headParts.end());
	}

	void ActorManager::Skeleton::clear()
	{
		//std::for_each(armors.begin(), armors.end(), [](Armor& armor) { armor.clearPhysics(); });
		//SkyrimPhysicsWorld::get()->removeSystemByNode(npc);
		cleanHead();
		head.headParts.clear();
		head.headNode = nullptr;
		//armors.clear();
	}

	void ActorManager::Skeleton::calculateDistanceAndOrientationDifferenceFromSource(RE::NiPoint3 sourcePosition, RE::NiPoint3 sourceOrientation)
	{
		if (isPlayerCharacter())
		{
			m_distanceFromCamera2 = 0.f;
			return;
		}

		auto pos = position();
		if (!pos.has_value())
		{
			m_distanceFromCamera2 = std::numeric_limits<float>::max();
			return;
		}

		// We calculate the vector between camera and the skeleton feets.
		const auto camera2SkeletonVector = pos.value() - sourcePosition;
		// This is the distance (squared) between the camera and the skeleton feets.
		m_distanceFromCamera2 = camera2SkeletonVector.x * camera2SkeletonVector.x + camera2SkeletonVector.y * camera2SkeletonVector.y + camera2SkeletonVector.z * camera2SkeletonVector.z;
		// This is |camera2SkeletonVector|*cos(angle between both vectors)
		m_cosAngleFromCameraDirectionTimesSkeletonDistance = camera2SkeletonVector.x * sourceOrientation.x + camera2SkeletonVector.y * sourceOrientation.y + camera2SkeletonVector.z * sourceOrientation.z;
	}


	bool ActorManager::Skeleton::isActiveInScene() const
	{
		// TODO: do this better
		// When entering/exiting an interior, NPCs are detached from the scene but not unloaded, so we need to check two levels up.
		// This properly removes exterior cell armors from the physics world when entering an interior, and vice versa.
		return skeleton->parent && skeleton->parent->parent && skeleton->parent->parent->parent;
	}

	bool ActorManager::Skeleton::isPlayerCharacter() const
	{
		constexpr uint32_t playerFormID = 0x14;
		return skeletonOwner.get()->formID == RE::PlayerCharacter::GetSingleton()->formID || (skeleton->GetUserData() && skeleton->GetUserData()->formID == playerFormID);
	}

	bool ActorManager::Skeleton::isInPlayerView()
	{
		// This function is called only when the skeleton isn't the player character.
		// This might change in the future; in that case this test will have to be enabled.
		//if (isPlayerCharacter())
		//	return true;

		// We don't enable the skeletons behind the camera or on its side.
		if (m_cosAngleFromCameraDirectionTimesSkeletonDistance <= 0)
			return false;

		// We enable only the skeletons that the PC sees.
		bool unk1 = 0;
		//return HasLOS((*g_thePlayer), skeleton->m_owner, &unk1);
		return RE::PlayerCharacter::GetSingleton()->HasLineOfSight(skeleton->GetUserData(), unk1);
	}

	std::optional<RE::NiPoint3> ActorManager::Skeleton::position() const
	{
		if (npc)
		{
			// This works for lurker skeletons.
			auto rootNode = findNode(npc, "NPC Root [Root]");
			if (rootNode) return std::optional<RE::NiPoint3>(rootNode->world.translate);
		}
		return std::optional<RE::NiPoint3>();
	}
}

#pragma warning(pop)
