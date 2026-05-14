#pragma once// EventQueue.hpp
#pragma once

#include "GameEvent.hpp"

#include <functional>
#include <vector>
#include <queue>
#include <cstdint>

using EventListener = std::function<void(const GameEvent&)>;

class EventQueue
{
public:
	std::uint8_t RegisterListener(EventListener listener)
	{
		listeners.push_back({ nextListenerId, listener });
		return nextListenerId++;
	}

	void DeregisterListener(std::uint8_t listenerId)
	{
		for (auto iterator = listeners.begin(); iterator != listeners.end(); ++iterator)
		{
			if (iterator->first == listenerId)
			{
				listeners.erase(iterator);
				return;
			}
		}
	}

	void EnqueueEvent(const GameEvent& event)
	{
		queuedEvents.push(event);
	}

	void BroadcastAllEvents()
	{
		while (!queuedEvents.empty())
		{
			GameEvent event = queuedEvents.front();
			queuedEvents.pop();

			for (auto& listener : listeners)
			{
				listener.second(event);
			}
		}
	}

private:
	std::vector<std::pair<std::uint8_t, EventListener>> listeners;
	std::queue<GameEvent> queuedEvents;
	std::uint8_t nextListenerId = 0;
};
