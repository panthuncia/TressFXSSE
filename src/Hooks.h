#pragma once
#include "Detours.h"
void hookFacegen();
void hookGameLoop();
void hookMainDraw();
namespace BoneHooks
{
	void Install();

	class UpdateHooks
	{
	public:
		static void Hook() {
			REL::Relocation<uintptr_t> hook1{ RELOCATION_ID(35565, 36564) };  // 5B2FF0, 5D9F50, main update
			auto&                      trampoline = SKSE::GetTrampoline();
			_Nullsub = trampoline.write_call<5>(hook1.address() + RELOCATION_OFFSET(0x748, 0xC26), Nullsub);  
		}

	private:
		static void Nullsub();
		static inline REL::Relocation<decltype(Nullsub)> _Nullsub;
	};
}
