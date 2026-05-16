#pragma once

#include <entt/entt.hpp>
#include "GameEvent.hpp"


struct QuestSystem
{
	bool hasFood = false;
	bool horseFed = false;

	std::string questMessage = "Find food for the horse";

	void Update(entt::registry& registry, const GameEvent& event, entt::entity playerEntity, entt::entity foodTriggerEntity, entt::entity horseTriggerEntity, entt::entity horseEntity);
};