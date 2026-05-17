#pragma once

#include <entt/entt.hpp>
#include <string>

enum class GameEventType
{
	None,
	DebugMessage,
	CollisionStarted,
	TriggerEntered,
	TriggerExited,
	QuestFoodFound,
	QuestHorseReached,
	QuestHorseFed
};

struct GameEvent
{
	GameEventType type = GameEventType::None;
	entt::entity entityA = entt::null;
	entt::entity entityB = entt::null;
	std::string message;
};
