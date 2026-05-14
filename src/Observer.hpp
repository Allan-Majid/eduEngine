#pragma once

#include "GameEvent.hpp"
#include <vector>
#include <algorithm>

class IObserver
{
public:
	virtual ~IObserver() = default;
	virtual void OnNotify(const GameEvent& event) = 0;
};

class Subject
{
public:
	void AddObserver(IObserver* observer)
	{
		observers.push_back(observer);
	}

	void RemoveObserver(IObserver* observer)
	{
		observers.erase(
			std::remove(observers.begin(), observers.end(), observer),
			observers.end()
		);
	}

protected:
	void Notify(const GameEvent& event)
	{
		for (IObserver* observer : observers)
		{
			if (observer)
			{
				observer->OnNotify(event);
			}
		}
	}

private:
	std::vector<IObserver*> observers;
};