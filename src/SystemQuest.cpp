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
	}
}