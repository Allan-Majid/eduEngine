#pragma once

#include <entt/entt.hpp>
#include "GameEvent.hpp"
#include "InputManager.hpp"
#include "EventQueue.hpp"

struct QuestSystem
{
	bool hasFood = false;
	bool horseFed = false;
	bool playerNearHorse = false;
	bool feedingStarted = false;
	float feedingTimer = 0.0f;
	float feedingDuration = 2.0f;

	std::string questMessage = "Find food for the horse";

	void Update(entt::registry& registry, EventQueue& eventQueue, const GameEvent& event, entt::entity playerEntity, entt::entity foodTriggerEntity, entt::entity horseTriggerEntity, entt::entity horseEntity);
	void UpdateQuestProgress(entt::registry& registry, EventQueue& eventQueue, float deltaTime, std::shared_ptr<eeng::InputManager> inputManager, entt::entity playerEntity, entt::entity horseEntity);
	void SendQuestUpdate(EventQueue& eventQueue);
};