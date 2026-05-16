#include "SystemQuest.hpp"
#include "ComponentAnimation.hpp"



void QuestSystem::Update(entt::registry& registry, const GameEvent& event, entt::entity playerEntity, entt::entity foodTriggerEntity, entt::entity horseTriggerEntity, entt::entity horseEntity)
{
	if (event.type != GameEventType::TriggerEntered)
	{
		return;
	}

	bool playerHitFoodTrigger = (event.entityA == playerEntity && event.entityB == foodTriggerEntity) || (event.entityA == foodTriggerEntity && event.entityB == playerEntity);
	bool playerHitHorseTrigger = (event.entityA == playerEntity && event.entityB == horseTriggerEntity) || (event.entityA == horseTriggerEntity && event.entityB == playerEntity);

	if (playerHitFoodTrigger && !hasFood)
	{
		hasFood = true;
		questMessage = "Food collected. Bring it to the horse.";
		return;
	}

	if (playerHitHorseTrigger && hasFood && !horseFed)
	{
		playerNearHorse = true;
		questMessage = "Press F to feed the horse.";
	}

}

void QuestSystem::UpdateQuestProgress(entt::registry& registry, float deltaTime, std::shared_ptr<eeng::InputManager> inputManager, entt::entity playerEntity, entt::entity horseEntity)
{
	using Key = eeng::InputManager::Key;

	if (playerNearHorse && hasFood && !horseFed && !feedingStarted && inputManager->IsKeyPressed(Key::F))
	{
		feedingStarted = true;
		feedingTimer = 0.0f;
		questMessage = "Feeding horse...";

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
			questMessage = "Quest completed. Horse fed.";

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