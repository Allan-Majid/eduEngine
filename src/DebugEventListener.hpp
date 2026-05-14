#pragma once

#include "Observer.hpp"
#include <string>

class DebugEventListener : public IObserver
{
public:
	void OnNotify(const GameEvent& event) override
	{
		lastMessage = event.message;
	}

	const std::string& GetLastMessage() const
	{
		return lastMessage;
	}

private:
	std::string lastMessage = "No events yet";
};
