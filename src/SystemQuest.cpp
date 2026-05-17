#include "SystemQuest.hpp"
#include "ComponentAnimation.hpp"

void QuestSystem::Update(entt::registry& registry, EventQueue& eventQueue, const GameEvent& event, entt::entity playerEntity, entt::entity foodTriggerEntity, entt::entity horseTriggerEntity, entt::entity horseEntity)
{
	bool playerHitFoodTrigger = event.entityA == foodTriggerEntity && event.entityB == playerEntity;
	bool playerHitHorseTrigger = event.entityA == horseTriggerEntity && event.entityB == playerEntity;

	if (event.type == GameEventType::TriggerEntered)
	{
		if (playerHitFoodTrigger && !hasFood)
		{
			hasFood = true;
			questMessage = "Food collected. Bring it to the horse.";
			SendQuestUpdate(eventQueue);
			return;
		}

		if (playerHitHorseTrigger && hasFood && !horseFed && !feedingStarted)
		{
			playerNearHorse = true;
			questMessage = "Press F to feed the horse.";
			SendQuestUpdate(eventQueue);
			return;
		}
	}

	if (event.type == GameEventType::TriggerExited)
	{
		if (playerHitHorseTrigger && hasFood && !horseFed && !feedingStarted)
		{
			playerNearHorse = false;
			questMessage = "Bring the food to the horse.";
			SendQuestUpdate(eventQueue);
			return;
		}
	}
}

void QuestSystem::UpdateQuestProgress(entt::registry& registry, EventQueue& eventQueue, float deltaTime, std::shared_ptr<eeng::InputManager> inputManager, entt::entity playerEntity, entt::entity horseEntity)
{
	using Key = eeng::InputManager::Key;

	if (playerNearHorse && hasFood && !horseFed && !feedingStarted && inputManager->IsKeyPressed(Key::F))
	{
		feedingStarted = true;
		feedingTimer = 0.0f;
		questMessage = "Feeding horse...";
		SendQuestUpdate(eventQueue);

		if (auto* playerAnimation = registry.try_get<AnimationComponent>(playerEntity))
		{
			playerAnimation->baseAnimation = 3;
			playerAnimation->secondaryAnimation = 3;
			playerAnimation->blendFactor = 0.0f;
			playerAnimation->useSpeedControl = false;
			playerAnimation->useLayering = false;
			playerAnimation->time = 0.0f;
			playerAnimation->playOnce = true;
			playerAnimation->freezeAtEnd = true;
			playerAnimation->animationFinished = false;
			playerAnimation->animationDuration = feedingDuration;
		}
	}

	if (feedingStarted && !horseFed)
	{
		feedingTimer += deltaTime;

		if (feedingTimer >= feedingDuration)
		{
			horseFed = true;
			playerNearHorse = false;
			questMessage = "Quest completed. Horse fed.";
			SendQuestUpdate(eventQueue);

			if (auto* horseAnimation = registry.try_get<AnimationComponent>(horseEntity))
			{
				horseAnimation->baseAnimation = 3;
				horseAnimation->secondaryAnimation = 2;
				horseAnimation->blendFactor = 1.0f;
				horseAnimation->useSpeedControl = false;
				horseAnimation->useLayering = false;
				horseAnimation->time = 0.0f;
				horseAnimation->playOnce = true;
				horseAnimation->freezeAtEnd = true;
				horseAnimation->animationFinished = false;
				horseAnimation->animationDuration = 0.9f;
			}

			if (auto* playerAnimation = registry.try_get<AnimationComponent>(playerEntity))
			{
				playerAnimation->baseAnimation = 1;
				playerAnimation->secondaryAnimation = 2;
				playerAnimation->blendFactor = 0.0f;
				playerAnimation->useSpeedControl = true;
				playerAnimation->useLayering = false;
				playerAnimation->playOnce = false;
				playerAnimation->freezeAtEnd = false;
				playerAnimation->animationFinished = false;
				playerAnimation->time = 0.0f;
			}
		}
	}
}

void QuestSystem::SendQuestUpdate(EventQueue& eventQueue)
{
	eventQueue.EnqueueEvent({
		GameEventType::QuestUpdated,
		entt::null,
		entt::null,
		questMessage
		});
}