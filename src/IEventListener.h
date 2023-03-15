#pragma once
#pragma once

namespace hdt
{
	template <class Event = void>
	class IEventListener
	{
	public:
		virtual void onEvent(const Event&) = 0;
	};

	template <>
	class IEventListener<void>
	{
	public:
		virtual void onEvent() = 0;
	};
}
