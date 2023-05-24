#pragma once

//#include "../hdtSSEUtils/NetImmerseUtils.h"
//#include "../hdtSSEUtils/FrameworkUtils.h"


#include "IEventListener.h"
#include "HookEvents.h"
#include "RE/M/MenuOpenCloseEvent.h"
#include <mutex>
#include <optional>
#include <RE/M/Moon.h>
#undef max
template <class T>
T max(const T& a, const T& b) restrict(cpu, amp) { return a < b ? b : a; }
namespace hdt
{
	class ActorManager
		: public IEventListener<ArmorAttachEvent>
		, public IEventListener<SkinSingleHeadGeometryEvent>
		, public IEventListener<SkinAllHeadGeometryEvent>
		, public IEventListener<FrameEvent>
		, public IEventListener<ShutdownEvent>
	{
		using IDType = uint32_t;

	public:

		enum class ItemState
		{
			e_NoPhysics,
			e_Inactive,
			e_Active
		};

		// Overall skeleton state, purely for console debug info
		enum class SkeletonState
		{
			// Note order: inactive states must come before e_SkeletonActive, and active states after
			e_InactiveNotInScene,
			e_InactiveUnseenByPlayer,
			e_InactiveTooFar,
			e_SkeletonActive,
			e_ActiveNearPlayer,
			e_ActiveIsPlayer
		};
		int activeSkeletons = 0;

	public:
		struct Skeleton;
	private:
		int maxActiveSkeletons = 10;
		int frameCount = 0;
		float rollingAverage = 0;
		struct Head
		{
			struct HeadPart
			{
				RE::BSGeometry* headPart;
				RE::NiNode* origPartRootNode;
				std::set<std::string> renamedBonesInUse;
			};

			IDType id;
			std::string* prefix;
			RE::BSFaceGenNiNode* headNode;
			RE::BSFadeNode* npcFaceGeomNode;
			std::vector<HeadPart> headParts;
			std::unordered_map<std::string, std::string> renameMap;
			std::unordered_map<std::string, uint8_t> nodeUseCount;
			bool isFullSkinning;
		};

		bool m_shutdown = false;
		std::recursive_mutex m_lock;
		std::vector<Skeleton> m_skeletons;



		Skeleton& getSkeletonData(RE::NiNode* skeleton);

	public:
		struct Skeleton
		{
			RE::NiPointer<RE::TESObjectREFR> skeletonOwner;
			RE::NiNode*                      skeleton;
			RE::NiNode*                      npc;
			Head                             head;
			SkeletonState                    state;

			std::string name();
			void        addArmor(RE::NiNode* armorModel);
			void        attachArmor(RE::NiNode* armorModel, RE::NiAVObject* attachedNode);

			void cleanArmor();
			void cleanHead(bool cleanAll = false);
			void clear();

			// @brief This calculates and sets the distance from skeleton to player, and a value that is the cosinus
			// between the camera orientation vector and the camera to skeleton vector, multiplied by the length
			// of the camera to skeleton vector; that value is very fast to compute as it is a dot product, and it
			// can be directly used for our needs later; the distance is provided squared for performance reasons.
			// @param sourcePosition the position of the camera
			// @param sourceOrientation the orientation of the camera
			void calculateDistanceAndOrientationDifferenceFromSource(RE::NiPoint3 sourcePosition, RE::NiPoint3 sourceOrientation);

			bool                        isPlayerCharacter() const;
			bool                        isInPlayerView();
			bool                        hasPhysics = false;
			std::optional<RE::NiPoint3> position() const;

			// @brief Update windfactor for skeleton
			// @param a_windFactor is a percentage [0,1] with 0 being no wind efect to 1 being full wind effect.
			void updateWindFactor(float a_windFactor);
			// @brief Get windfactor for skeleton
			float getWindFactor();

			// @brief Updates the states and activity of skeletons, their heads parts and armors.
			// @param playerCell The skeletons not in the player cell are automatically inactive.
			// @param deactivate If set to true, the concerned skeleton will be inactive, regardless of other elements.
			bool updateAttachedState(const RE::NiNode* playerCell, bool deactivate);

			// bool deactivate(); // FIXME useless?
			void reloadMeshes();

			void scanHead();
			void processGeometry(RE::BSFaceGenNiNode* head, RE::BSGeometry* geometry);

			static void        doSkeletonMerge(RE::NiNode* dst, RE::NiNode* src, std::string* prefix,
					   std::unordered_map<std::string, std::string>& map);
			static void        doSkeletonClean(RE::NiNode* dst, std::string* prefix);
			static RE::NiNode* cloneNodeTree(RE::NiNode* src, std::string* prefix, std::unordered_map<std::string, std::string>& map);
			static void        renameTree(RE::NiNode* root, std::string* prefix, std::unordered_map<std::string, std::string>& map);

			// @brief This is the squared distance between the skeleton and the camera.
			float m_distanceFromCamera2 = std::numeric_limits<float>::max();

			// @brief This is |camera2SkeletonVector|*cos(angle between that vector and the camera direction).
			float m_cosAngleFromCameraDirectionTimesSkeletonDistance = -1.;

		private:
			bool isActiveInScene() const;
			bool checkPhysics();

			bool  isActive = false;
			float currentWindFactor = 0.f;
			//std::vector<Armor> armors;
		};
		ActorManager();
		~ActorManager();
		Skeleton* m_playerSkeleton;
		static ActorManager* instance();

		static std::string armorPrefix(IDType id);
		static std::string headPrefix(IDType id);

		void onEvent(const ArmorAttachEvent& e) override;

		// @brief On this event, we decide which skeletons will be active for physics this frame.
		void onEvent(const FrameEvent& e) override;

		void onEvent(const RE::MenuOpenCloseEvent&);
		void onEvent(const ShutdownEvent&) override;
		void onEvent(const SkinSingleHeadGeometryEvent&) override;
		void onEvent(const SkinAllHeadGeometryEvent&) override;

#ifdef ANNIVERSARY_EDITION
		bool skeletonNeedsParts(NiNode* skeleton);
#endif
		std::vector<Skeleton>& getSkeletons();//Altered by Dynamic HDT

		bool m_skinNPCFaceParts = true;
		bool m_autoAdjustMaxSkeletons = true; // Whether to dynamically change the maxActive skeletons to maintain min_fps
		int m_maxActiveSkeletons = 20; // The maximum active skeletons; hard limit
		int m_sampleSize = 5; // how many samples (each sample taken every second) for determining average time per activeSkeleton.

		// @brief Depending on this setting, we avoid to calculate the physics of the PC when it is in 1st person view.
		bool m_disable1stPersonViewPhysics = false;

	private:
		void setSkeletonsActive();
	};
}
