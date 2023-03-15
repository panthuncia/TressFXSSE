#pragma once

#include "IEventDispatcher.h"

#include <unordered_set>
#include <mutex>

namespace hdt
{
	template <class Event = void>
	class EventDispatcherImpl : public IEventDispatcher<Event>
	{
	public:
		EventDispatcherImpl()
		{
		}

		~EventDispatcherImpl()
		{
		}

		void addListener(IEventListener<Event>*) override;
		void removeListener(IEventListener<Event>*) override;
		void dispatch(const Event&) override;

	protected:
		std::recursive_mutex m_lock;
		std::unordered_set<IEventListener<Event>*> m_listeners;
		std::vector<IEventListener<Event>*> m_caches;
		bool m_cacheDirt = false;
	};

	template <class Event>
	void EventDispatcherImpl<Event>::addListener(IEventListener<Event>* listener)
	{
		std::lock_guard<decltype(m_lock)> l(m_lock);
		m_listeners.insert(listener);
		m_cacheDirt = true;
	}

	template <class Event>
	void EventDispatcherImpl<Event>::removeListener(IEventListener<Event>* listener)
	{
		std::lock_guard<decltype(m_lock)> l(m_lock);
		m_listeners.erase(listener);
		m_cacheDirt = true;
	}

	template <class Event>
	void EventDispatcherImpl<Event>::dispatch(const Event& event)
	{
		std::lock_guard<decltype(m_lock)> l(m_lock);
		if (m_cacheDirt)
		{
			m_caches.clear();
			for (auto& i : m_listeners)
				m_caches.push_back(i);
			m_cacheDirt = false;
		}

		for (auto i : m_caches)
			i->onEvent(event);
	}

	template <>
	class EventDispatcherImpl<void> : public IEventDispatcher<void>
	{
	public:
		EventDispatcherImpl()
		{
		}

		~EventDispatcherImpl()
		{
		}

		void addListener(IEventListener<void>*) override;
		void removeListener(IEventListener<void>*) override;
		void dispatch() override;

	protected:
		std::recursive_mutex m_lock;
		std::unordered_set<IEventListener<void>*> m_listeners;
		std::vector<IEventListener<void>*> m_caches;
		bool m_cacheDirt = false;
	};

	inline void EventDispatcherImpl<void>::dispatch()
	{
		std::lock_guard<decltype(m_lock)> l(m_lock);
		if (m_cacheDirt)
		{
			m_caches.clear();
			for (auto& i : m_listeners)
				m_caches.push_back(i);
			m_cacheDirt = false;
		}

		for (auto i : m_caches)
			i->onEvent();
	}
}
