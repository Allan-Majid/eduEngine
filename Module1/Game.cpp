
#include <entt/entt.hpp>
#include "glmcommon.hpp"
#include "imgui.h"
#include "Log.hpp"
#include "Game.hpp"
#include "ComponentTransform.hpp"
#include "ComponentLinearVelocity.hpp"
#include "ComponentMesh.hpp"
#include "ComponentPlayerController.hpp"
#include "ComponentNPCController.hpp"
#include "ComponentAnimation.hpp"
#include "ComponentSphereCollider.hpp"
#include "ComponentAABBCollider.hpp"


bool Game::init()
{

	initRenderers();
	initRegistry();
	initEventSystem();
	loadMeshes();
	initWorldMatrices();
	createPlayerEntity();
	createHorseEntity();
	createNPCEntity();
	createQuestTriggerEntities();
	createWallTestEntity();
	logEntitySetup();

	return true;

}

void Game::update(
	float time,
	float deltaTime,
	InputManagerPtr input)
{

	eventQueue.BroadcastAllEvents();

	updateInputAndCamera(deltaTime, input);
	updateSystems(deltaTime, input);
	updateSceneState(time);

}

void Game::render(float time, int windowWidth, int windowHeight)
{
	renderUI();
	updateRenderMatrices(windowWidth, windowHeight);

	forwardRenderer->beginPass(matrices.P, matrices.V, pointlight.pos, pointlight.color, camera.pos);
	renderScene(time);
	drawcallCount = forwardRenderer->endPass();

	if (drawCollisionDebug)
	{
		shapeRenderer->render(matrices.P * matrices.V);
		shapeRenderer->post_render();
	}
}

void Game::renderUI()
{

	renderGameInfoUI();
	renderCustomDebugUI();
	//renderInWorldHorseLabel();
	renderQuestObjectiveWindow();

}

void Game::destroy()
{

}

void Game::updateCamera(
	InputManagerPtr input)
{
	// Fetch mouse and compute movement since last frame
	auto mouse = input->GetMouseState();
	glm::ivec2 mouse_xy{ mouse.x, mouse.y };
	glm::ivec2 mouse_xy_diff{ 0, 0 };
	if (mouse.leftButton && camera.mouse_xy_prev.x >= 0)
		mouse_xy_diff = camera.mouse_xy_prev - mouse_xy;
	camera.mouse_xy_prev = mouse_xy;

	// Update camera rotation from mouse movement
	camera.yaw += mouse_xy_diff.x * camera.sensitivity;
	camera.pitch += mouse_xy_diff.y * camera.sensitivity;
	camera.pitch = glm::clamp(camera.pitch, -glm::radians(89.0f), 0.0f);

	// Update camera position
	const glm::vec4 rotatedPos = glm_aux::R(camera.yaw, camera.pitch) * glm::vec4(0.0f, 0.0f, camera.distance, 1.0f);
	camera.pos = camera.lookAt + glm::vec3(rotatedPos);
}

void Game::updatePlayer(
	float deltaTime,
	InputManagerPtr input)
{
	// Fetch keys relevant for player movement
	using Key = eeng::InputManager::Key;
	bool W = input->IsKeyPressed(Key::W);
	bool A = input->IsKeyPressed(Key::A);
	bool S = input->IsKeyPressed(Key::S);
	bool D = input->IsKeyPressed(Key::D);

	// Compute vectors in the local space of the player
	player.fwd = glm::vec3(glm_aux::R(camera.yaw, glm_aux::vec3_010) * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f));
	player.right = glm::cross(player.fwd, glm_aux::vec3_010);

	// Compute the total movement as a 3D vector
	auto movement =
		player.fwd * player.velocity * deltaTime * ((W ? 1.0f : 0.0f) + (S ? -1.0f : 0.0f)) +
		player.right * player.velocity * deltaTime * ((A ? -1.0f : 0.0f) + (D ? 1.0f : 0.0f));

	// Update player position and forward view ray
	player.pos += movement;
	player.viewRay = glm_aux::Ray{ player.pos + glm::vec3(0.0f, 2.0f, 0.0f), player.fwd };

	// Update camera to track the player
	camera.lookAt += movement;
	camera.pos += movement;

}

void Game::renderGameInfoUI()
{
	ImGui::Begin("Game Info");

	ImGui::Text("Drawcall count %i", drawcallCount);

	if (ImGui::ColorEdit3("Light color", glm::value_ptr(pointlight.color), ImGuiColorEditFlags_NoInputs))
	{
	}

	if (characterMesh)
	{
		ImGui::Separator();
		ImGui::Text("Middle Character (controllable): Single Clip");

		int curAnimIndex = middleCharacterAnimIndex;
		std::string label = (curAnimIndex == -1 ? "Bind pose" : characterMesh->getAnimationName(curAnimIndex));

		if (ImGui::BeginCombo("Clip##middle_animclip", label.c_str()))
		{
			const bool isSelected = (curAnimIndex == -1);

			if (ImGui::Selectable("Bind pose", isSelected))
				curAnimIndex = -1;

			if (isSelected)
				ImGui::SetItemDefaultFocus();

			for (int i = 0; i < characterMesh->getNbrAnimations(); i++)
			{
				const bool isSelected = (curAnimIndex == i);
				const auto label = characterMesh->getAnimationName(i) + "##" + std::to_string(i);

				if (ImGui::Selectable(label.c_str(), isSelected))
					curAnimIndex = i;

				if (isSelected)
					ImGui::SetItemDefaultFocus();
			}

			ImGui::EndCombo();
		}

		middleCharacterAnimIndex = curAnimIndex;
		ImGui::SliderFloat("Animation speed##middle_speed", &characterAnimSpeed, 0.1f, 5.0f);
	}

	ImGui::Separator();
	ImGui::Text("Left Character: 2-Clip Blend");
	ImGui::Text("Idle (1) + Walking (2)");
	ImGui::SliderFloat("Blend factor##left_blend", &leftCharacterAnimBlend, 0.0f, 1.0f);

	ImGui::Separator();
	ImGui::Text("Right Character: Filtered Blend");
	ImGui::Text("Walking (2) + Waving (3)");
	ImGui::Text("Branch root: mixamorig:Spine");
	ImGui::Checkbox("Spine subtree uses waving", &rightCharacterSubtreeUsesWave);

	ImGui::End();
}

void Game::renderCustomDebugUI()
{
	ImGui::Begin("Allan Custom Stuff");

	ImGui::Text("Total Time Elapsed Since Start of Session: %0.2f Seconds", ImGui::GetTime());

	ImGui::Separator();
	ImGui::Text("Latest Event:");
	ImGui::Text(debugEventListener.GetLastMessage().c_str());

	renderHorseEntityUI();
	renderNPCEntityUI();

	ImGui::Separator();
	ImGui::Text("Quest");
	ImGui::Text("Has Food: %s", questSystem.hasFood ? "true" : "false");
	ImGui::Text("Horse Fed: %s", questSystem.horseFed ? "true" : "false");

	if (!questSystem.hasFood)
	{
		ImGui::Text("Objective: Find food for the horse");
	}
	else if (!questSystem.horseFed)
	{
		ImGui::Text("Objective: Bring the food to the horse");
	}
	else
	{
		ImGui::Text("Objective: Quest completed");
	}

	ImGui::Separator();
	ImGui::Checkbox("Draw Collision Debug", &drawCollisionDebug);

	ImGui::End();
}

void Game::renderHorseEntityUI()
{
	if (!ImGui::CollapsingHeader("Player Controlled Horse Entity Stuff"))
		return;

	auto& horseTransform = entity_registry->get<TransformComponent>(horseEntity);

	ImGui::Text(
		"Horse Entity Position: \n X: %f \n Y: %f \n Z: %f \n",
		horseTransform.position.x,
		horseTransform.position.y,
		horseTransform.position.z
	);

	float uniformScale = horseTransform.scale.x;

	if (ImGui::SliderFloat("Horse Scale##horse", &uniformScale, 0.0f, 0.1f, "%0.2f"))
	{
		horseTransform.scale = glm::vec3(uniformScale);
	}

	ImGui::SliderFloat("Horse Y Rotation##horse", &horseTransform.rotation.y, 0.0f, 360.0f, "%1.0f");

	auto& horseAnimationComponent = entity_registry->get<AnimationComponent>(horseEntity);
	renderAnimationControls("Horse", horseAnimationComponent, 10);
}

void Game::renderNPCEntityUI()
{
	if (!ImGui::CollapsingHeader("NPC Controlled Character Stuff"))
		return;

	auto& npcController = entity_registry->get<NPCControllerComponent>(npcEntity);

	auto& npcAnimationComponent = entity_registry->get<AnimationComponent>(npcEntity);
	renderAnimationControls("NPC", npcAnimationComponent, 3);

	ImGui::SliderFloat("NPC Movement Speed##npc", &npcController.movementSpeed, 0.0f, 4.0f, "%0.1f");
}

void Game::renderInWorldHorseLabel()
{
	const auto VP_P_V = matrices.VP * matrices.P * matrices.V;
	auto world_pos = glm::vec3(horseWorldMatrix[3]);

	glm::ivec2 window_coords;

	if (glm_aux::window_coords_from_world_pos(world_pos, VP_P_V, window_coords))
	{
		ImGui::SetNextWindowPos(
			ImVec2{ float(window_coords.x), float(matrices.windowSize.y - window_coords.y) },
			ImGuiCond_Always,
			ImVec2{ 0.0f, 0.0f }
		);

		ImGui::PushStyleColor(ImGuiCol_WindowBg, 0x80000000);
		ImGui::PushStyleColor(ImGuiCol_Text, 0xffffffff);

		ImGuiWindowFlags flags =
			ImGuiWindowFlags_NoDecoration |
			ImGuiWindowFlags_NoInputs |
			ImGuiWindowFlags_AlwaysAutoResize;

		if (ImGui::Begin("window_name", nullptr, flags))
		{
			ImGui::Text("In-world GUI element");
			ImGui::Text("Window pos (%i, %i)", window_coords.x, window_coords.y);
			ImGui::Text("World pos (%1.1f, %1.1f, %1.1f)", world_pos.x, world_pos.y, world_pos.z);
			ImGui::End();
		}

		ImGui::PopStyleColor(2);
	}
}

void Game::renderAnimationControls(const char* labelPrefix, AnimationComponent& animationComponent, int maxAnimationIndex)
{
	std::string id = std::string("##") + labelPrefix;

	ImGui::SliderInt((std::string(labelPrefix) + " Primary Animation" + id).c_str(), &animationComponent.baseAnimation, 0, maxAnimationIndex);
	ImGui::SliderInt((std::string(labelPrefix) + " Secondary Animation" + id).c_str(), &animationComponent.secondaryAnimation, 0, maxAnimationIndex);
	ImGui::SliderFloat((std::string("Blend Factor") + id).c_str(), &animationComponent.blendFactor, 0.0f, 1.0f, "%0.2f");
	ImGui::Checkbox((std::string("Use Speed Control") + id).c_str(), &animationComponent.useSpeedControl);
	ImGui::SliderFloat((std::string(labelPrefix) + " Speed" + id).c_str(), &animationComponent.speed, 0.0f, 1.0f, "%0.2f");
	ImGui::Checkbox((std::string("Use Layering") + id).c_str(), &animationComponent.useLayering);
	ImGui::Checkbox((std::string("Draw Skeleton") + id).c_str(), &animationComponent.drawSkeleton);
}

void Game::initRenderers()
{
	forwardRenderer = std::make_shared<eeng::ForwardRenderer>();
	forwardRenderer->init("shaders/phong_vert.glsl", "shaders/phong_frag.glsl");

	shapeRenderer = std::make_shared<ShapeRendering::ShapeRenderer>();
	shapeRenderer->init();
}

void Game::initRegistry()
{
	entity_registry = std::make_shared<entt::registry>();
}

void Game::initEventSystem()
{
	debugListenerId = eventQueue.RegisterListener([this](const GameEvent& event) { debugEventListener.OnNotify(event); });
	eventQueue.RegisterListener([this](const GameEvent& event) { questSystem.Update(*entity_registry, eventQueue, event, playerEntity, foodTriggerEntity, horseTriggerEntity, horseEntity); });
	eventQueue.RegisterListener([this](const GameEvent& event)
		{
			if (event.type == GameEventType::QuestUpdated)
			{
				questObjectiveText = event.message;
			}
		});
	eventQueue.EnqueueEvent({ GameEventType::DebugMessage, entt::null, entt::null, "Event system initialized" });
}

void Game::loadMeshes()
{
	grassMesh = std::make_shared<eeng::RenderableMesh>();
	grassMesh->load("assets/grass/grass_trees_merged.fbx", false);

	horseMesh = std::make_shared<eeng::RenderableMesh>();
	horseMesh->load("assets/Animals/Horse.fbx", false);

	characterMesh = std::make_shared<eeng::RenderableMesh>();
	characterMesh->load("assets/Amy/Ch46_nonPBR.fbx");
	characterMesh->load("assets/Amy/idle.fbx", true);
	characterMesh->load("assets/Amy/walking.fbx", true);
	characterMesh->load("assets/Amy/waving.fbx", true);
	characterMesh->removeTranslationKeys("mixamorig:Hips");

	playerMesh = std::make_shared<eeng::RenderableMesh>();
	playerMesh->load("assets/Amy/Ch46_nonPBR.fbx");
	playerMesh->load("assets/Amy/idle.fbx", true);
	playerMesh->load("assets/Amy/walking.fbx", true);
	playerMesh->load("assets/Amy/waving.fbx", true);
	playerMesh->removeTranslationKeys("mixamorig:Hips");

	npcMesh = std::make_shared<eeng::RenderableMesh>();
	npcMesh->load("assets/Amy/Ch46_nonPBR.fbx");
	npcMesh->load("assets/Amy/idle.fbx", true);
	npcMesh->load("assets/Amy/walking.fbx", true);
	npcMesh->load("assets/Amy/waving.fbx", true);
	npcMesh->removeTranslationKeys("mixamorig:Hips");
}

void Game::initWorldMatrices()
{
	grassWorldMatrix = glm_aux::TRS({ 0.0f, 0.0f, 0.0f }, 0.0f, { 0, 1, 0 }, { 100.0f, 100.0f, 100.0f });
	horseWorldMatrix = glm_aux::TRS({ 30.0f, 0.0f, -35.0f }, 35.0f, { 0, 1, 0 }, { 0.01f, 0.01f, 0.01f });
}

void Game::createPlayerEntity()
{
	playerEntity = entity_registry->create();

	auto& playerController = entity_registry->emplace<PlayerControllerComponent>(playerEntity);
	playerController.movementSpeed = 10.0f;
	playerController.currentSpeed = 0.0f;
	playerController.acceleration = 8.0f;
	playerController.deceleration = 3.0f;

	auto& playerTransform = entity_registry->emplace<TransformComponent>(playerEntity);
	playerTransform.position = { 0.0f, 0.0f, -20.0f };
	playerTransform.rotation = { 0.0f, 0.0f, 0.0f };
	playerTransform.scale = { 0.03f, 0.03f, 0.03f };

	auto& playerMeshComponent = entity_registry->emplace<MeshComponent>(playerEntity);
	playerMeshComponent.mesh = playerMesh;

	auto& playerVelocity = entity_registry->emplace<LinearVelocityComponent>(playerEntity);
	playerVelocity.velocity = { 0.0f, 0.0f, 0.0f };

	auto& playerAnimation = entity_registry->emplace<AnimationComponent>(playerEntity);
	playerAnimation.baseAnimation = 1;
	playerAnimation.secondaryAnimation = 2;
	playerAnimation.blendFactor = 0.0f;
	playerAnimation.useSpeedControl = true;
	playerAnimation.speed = 0.0f;
	playerAnimation.maxSpeedForFullBlend = 4.0f;
	playerAnimation.useLayering = false;
	playerAnimation.upperBodyRootNode = "mixamorig:Spine";

	auto& playerAABB = entity_registry->emplace<AABBColliderComponent>(playerEntity);
	playerAABB.halfExtents = { 1.0f, 2.2f, 1.0f };
	playerAABB.offset = { 0.0f, 2.2f, 0.0f };
	playerAABB.isTrigger = false;

	auto& playerSphere = entity_registry->emplace<SphereColliderComponent>(playerEntity);
	playerSphere.radius = glm::length(playerAABB.halfExtents);
	playerSphere.offset = playerAABB.offset;
	playerSphere.isTrigger = false;

}

void Game::createHorseEntity()
{
	horseEntity = entity_registry->create();

	auto& horseTransform = entity_registry->emplace<TransformComponent>(horseEntity);
	horseTransform.position = { 0.0f, 0.0f, -35.0f };
	horseTransform.rotation = { 0.0f, 180.0f, 0.0f };
	horseTransform.scale = { 0.01f, 0.01f, 0.01f };

	auto& horseMeshComponent = entity_registry->emplace<MeshComponent>(horseEntity);
	horseMeshComponent.mesh = horseMesh;

	auto& horseVelocity = entity_registry->emplace<LinearVelocityComponent>(horseEntity);
	horseVelocity.velocity = { 0.0f, 0.0f, 0.0f };

	auto& horseAnimation = entity_registry->emplace<AnimationComponent>(horseEntity);
	horseAnimation.baseAnimation = 3;
	horseAnimation.secondaryAnimation = 9;
	horseAnimation.blendFactor = 0.0f;
	horseAnimation.useSpeedControl = true;
	horseAnimation.speed = 0.0f;
	horseAnimation.maxSpeedForFullBlend = 10.0f;
	horseAnimation.useLayering = false;
	horseAnimation.upperBodyRootNode = "";


	auto& horseAABB = entity_registry->emplace<AABBColliderComponent>(horseEntity);
	horseAABB.halfExtents = { 1.5f, 2.5f, 3.2f };
	horseAABB.offset = { 0.0f, 2.4f, -0.6f };
	horseAABB.isTrigger = false;

	auto& horseSphere = entity_registry->emplace<SphereColliderComponent>(horseEntity);
	horseSphere.radius = glm::length(horseAABB.halfExtents);
	horseSphere.offset = horseAABB.offset;
	horseSphere.isTrigger = false;
}

void Game::createWallTestEntity()
{
	wallTestEntity = entity_registry->create();

	auto& wallTransform = entity_registry->emplace<TransformComponent>(wallTestEntity);
	wallTransform.position = { 10.0f, 1.0f, -20.0f };
	wallTransform.rotation = { 0.0f, 0.0f, 0.0f };
	wallTransform.scale = { 1.0f, 1.0f, 1.0f };

	auto& wallAABB = entity_registry->emplace<AABBColliderComponent>(wallTestEntity);
	wallAABB.halfExtents = { 3.0f, 1.0f, 3.0f };
	wallAABB.isTrigger = false;
	wallAABB.isStatic = true;

	auto& wallSphere = entity_registry->emplace<SphereColliderComponent>(wallTestEntity);
	wallSphere.radius = glm::length(wallAABB.halfExtents);
	wallSphere.offset = wallAABB.offset;
	wallSphere.isTrigger = false;
	wallSphere.isStatic = true;
}

void Game::createNPCEntity()
{
	npcEntity = entity_registry->create();

	auto& npcTransform = entity_registry->emplace<TransformComponent>(npcEntity);
	npcTransform.position = { -5.0f, 0.0f, -5.0f };
	npcTransform.rotation = { 0.0f, 0.0f, 0.0f };
	npcTransform.scale = { 0.03f, 0.03f, 0.03f };

	auto& npcMeshComponent = entity_registry->emplace<MeshComponent>(npcEntity);
	npcMeshComponent.mesh = npcMesh;

	auto& npcVelocity = entity_registry->emplace<LinearVelocityComponent>(npcEntity);
	npcVelocity.velocity = { 0.0f, 0.0f, 0.0f };

	auto& npcController = entity_registry->emplace<NPCControllerComponent>(npcEntity);
	npcController.movementSpeed = 2.0f;
	npcController.arriveDistance = 0.5f;
	npcController.waypoints = { { -5.0f, 0.0f, -5.0f }, { -5.0f, 0.0f, 5.0f }, { 5.0f, 0.0f, 5.0f }, { 5.0f, 0.0f, -5.0f } };

	auto& npcAnimationComponent = entity_registry->emplace<AnimationComponent>(npcEntity);
	npcAnimationComponent.baseAnimation = 2;
	npcAnimationComponent.secondaryAnimation = 3;
	npcAnimationComponent.maxSpeedForFullBlend = npcController.movementSpeed;
	npcAnimationComponent.blendFactor = 0.7f;
	npcAnimationComponent.useLayering = true;
	npcAnimationComponent.upperBodyRootNode = "mixamorig:Spine";

	auto& npcAABB = entity_registry->emplace<AABBColliderComponent>(npcEntity);
	npcAABB.halfExtents = { 1.0f, 2.2f, 1.0f };
	npcAABB.offset = { 0.0f, 2.2f, 0.0f };
	npcAABB.isTrigger = false;

	auto& npcSphere = entity_registry->emplace<SphereColliderComponent>(npcEntity);
	npcSphere.radius = glm::length(npcAABB.halfExtents);
	npcSphere.offset = npcAABB.offset;
	npcSphere.isTrigger = false;
}

void Game::logEntitySetup()
{
	auto& horseTransform = entity_registry->get<TransformComponent>(horseEntity);

	eeng::Log("Horse entity has TransformComponent: %s", entity_registry->all_of<TransformComponent>(horseEntity) ? "true" : "false");
	eeng::Log("Horse entity has MeshComponent: %s", entity_registry->all_of<MeshComponent>(horseEntity) ? "true" : "false");
	eeng::Log("Horse ECS Transform position: (%f, %f, %f)", horseTransform.position.x, horseTransform.position.y, horseTransform.position.z);
	eeng::Log("Is horseEntity an Orphan? Answer: %s", entity_registry->orphan(horseEntity) ? "true" : "false");
}

void Game::updateRenderMatrices(int windowWidth, int windowHeight)
{
	matrices.windowSize = glm::ivec2(windowWidth, windowHeight);

	const float aspectRatio = float(windowWidth) / windowHeight;
	matrices.P = glm::perspective(glm::radians(60.0f), aspectRatio, camera.nearPlane, camera.farPlane);
	matrices.V = glm::lookAt(camera.pos, camera.lookAt, camera.up);
	matrices.VP = glm_aux::create_viewport_matrix(0.0f, 0.0f, windowWidth, windowHeight, 0.0f, 1.0f);
}

void Game::renderScene(float time)
{
	renderSystem.Render(*entity_registry, forwardRenderer, shapeRenderer, time, drawCollisionDebug);

	forwardRenderer->renderMesh(grassMesh, grassWorldMatrix);
	grass_aabb = grassMesh->m_model_aabb.post_transform(grassWorldMatrix);
}

void Game::renderDebugShapes()
{
	if (player.viewRay)
	{
		shapeRenderer->push_states(ShapeRendering::Color4u{ 0xff00ff00 });
		shapeRenderer->push_line(player.viewRay.origin, player.viewRay.point_of_contact());
	}
	else
	{
		shapeRenderer->push_states(ShapeRendering::Color4u{ 0xffffffff });
		shapeRenderer->push_line(player.viewRay.origin, player.viewRay.origin + player.viewRay.dir * 100.0f);
	}

	shapeRenderer->pop_states<ShapeRendering::Color4u>();

	shapeRenderer->push_states(ShapeRendering::Color4u{ 0xFFE61A80 });
	shapeRenderer->push_AABB(character_aabb1.min, character_aabb1.max);
	shapeRenderer->push_AABB(character_aabb2.min, character_aabb2.max);
	shapeRenderer->push_AABB(character_aabb3.min, character_aabb3.max);
	shapeRenderer->push_AABB(horse_aabb.min, horse_aabb.max);
	shapeRenderer->push_AABB(grass_aabb.min, grass_aabb.max);
	shapeRenderer->pop_states<ShapeRendering::Color4u>();

	auto colliderView = entity_registry->view<TransformComponent, AABBColliderComponent>();

	shapeRenderer->push_states(ShapeRendering::Color4u{ 0xFF00FFFF });

	for (auto entity : colliderView)
	{
		auto& transform = colliderView.get<TransformComponent>(entity);
		auto& aabb = colliderView.get<AABBColliderComponent>(entity);

		glm::vec3 center = transform.position + aabb.offset;

		glm::vec3 min = center - aabb.halfExtents;
		glm::vec3 max = center + aabb.halfExtents;

		shapeRenderer->push_AABB(min, max);
	}

	auto sphereView = entity_registry->view<TransformComponent, SphereColliderComponent>();

	shapeRenderer->push_states(ShapeRendering::Color4u{ 0xFFFF0000 });

	for (auto entity : sphereView)
	{
		auto& transform = sphereView.get<TransformComponent>(entity);
		auto& sphere = sphereView.get<SphereColliderComponent>(entity);

		shapeRenderer->push_states(glm_aux::TS(transform.position + sphere.offset, glm::vec3(1.0f)));

		shapeRenderer->push_sphere_wireframe(sphere.radius, sphere.radius);

		shapeRenderer->pop_states<glm::mat4>();
	}

	shapeRenderer->pop_states<ShapeRendering::Color4u>();
}

void Game::updateSceneState(float time)
{
	pointlight.pos = glm::vec3(glm_aux::R(time * 0.1f, { 0.0f, 1.0f, 0.0f }) * glm::vec4(100.0f, 100.0f, 100.0f, 1.0f));

	characterWorldMatrix1 = glm_aux::TRS(player.pos, 0.0f, { 0, 1, 0 }, { 0.03f, 0.03f, 0.03f });
	characterWorldMatrix2 = glm_aux::TRS({ -3, 0, 0 }, time * glm::radians(50.0f), { 0, 1, 0 }, { 0.03f, 0.03f, 0.03f });
	characterWorldMatrix3 = glm_aux::TRS({ 3, 0, 0 }, time * glm::radians(50.0f), { 0, 1, 0 }, { 0.03f, 0.03f, 0.03f });

	glm_aux::intersect_ray_AABB(player.viewRay, character_aabb2.min, character_aabb2.max);
	glm_aux::intersect_ray_AABB(player.viewRay, character_aabb3.min, character_aabb3.max);
	glm_aux::intersect_ray_AABB(player.viewRay, horse_aabb.min, horse_aabb.max);

	if (player.viewRay)
	{
		eeng::Log("Ray intersects at (%f, %f, %f)", player.viewRay.point_of_contact().x, player.viewRay.point_of_contact().y, player.viewRay.point_of_contact().z);
	}
}

void Game::updateSystems(float deltaTime, InputManagerPtr input)
{
	playerControllerSystem.Update(*entity_registry, deltaTime, camera.yaw, input);
	npcControllerSystem.Update(*entity_registry);
	movementSystem.Update(*entity_registry, deltaTime);
	collisionSystem.Update(*entity_registry, eventQueue);
	animationSystem.Update(*entity_registry, deltaTime);
	questSystem.UpdateQuestProgress(*entity_registry, eventQueue, deltaTime, input, playerEntity, horseEntity);
}

void Game::updateInputAndCamera(float deltaTime, InputManagerPtr input)
{
	//updateCamera(input);
	//updatePlayer(deltaTime, input);
	updatePlayerCamera(input);
}

void Game::updatePlayerCamera(InputManagerPtr input)
{
	if (!entity_registry->valid(playerEntity) || !entity_registry->all_of<TransformComponent>(playerEntity))
	{
		return;
	}

	auto& playerTransform = entity_registry->get<TransformComponent>(playerEntity);

	auto mouse = input->GetMouseState();
	glm::ivec2 mouseXY{ mouse.x, mouse.y };
	glm::ivec2 mouseXYDiff{ 0, 0 };

	if (mouse.leftButton && camera.mouse_xy_prev.x >= 0)
	{
		mouseXYDiff = camera.mouse_xy_prev - mouseXY;
	}

	camera.mouse_xy_prev = mouseXY;

	camera.yaw += mouseXYDiff.x * camera.sensitivity;
	camera.pitch += mouseXYDiff.y * camera.sensitivity;
	camera.pitch = glm::clamp(camera.pitch, -glm::radians(89.0f), glm::radians(20.0f));

	glm::vec3 playerPosition = playerTransform.position;
	glm::vec3 cameraTarget = playerPosition + glm::vec3(0.0f, 4.5f, 0.0f);
	camera.distance = 8.0f;

	const glm::vec4 rotatedOffset = glm_aux::R(camera.yaw, camera.pitch) * glm::vec4(0.0f, 0.0f, camera.distance, 1.0f);

	camera.lookAt = cameraTarget;
	camera.pos = cameraTarget + glm::vec3(rotatedOffset);
}

void Game::createQuestTriggerEntities()
{
	foodTriggerEntity = entity_registry->create();

	auto& foodTransform = entity_registry->emplace<TransformComponent>(foodTriggerEntity);
	foodTransform.position = { 18.0f, 0.01f, -8.0f };
	foodTransform.rotation = { 0.0f, 0.0f, 0.0f };
	foodTransform.scale = { 2.0f, 5.0f, 2.0f };

	auto& foodMesh = entity_registry->emplace<MeshComponent>(foodTriggerEntity);
	foodMesh.mesh = grassMesh;

	auto& foodAABB = entity_registry->emplace<AABBColliderComponent>(foodTriggerEntity);
	foodAABB.halfExtents = { 2.0f, 1.0f, 2.0f };
	foodAABB.offset = { 0.0f, 1.0f, 0.0f };
	foodAABB.isTrigger = true;

	auto& foodSphere = entity_registry->emplace<SphereColliderComponent>(foodTriggerEntity);
	foodSphere.radius = glm::length(foodAABB.halfExtents);
	foodSphere.offset = foodAABB.offset;
	foodSphere.isTrigger = true;

	horseTriggerEntity = entity_registry->create();

	auto& horseTriggerTransform = entity_registry->emplace<TransformComponent>(horseTriggerEntity);
	horseTriggerTransform.position = { 0.0f, 0.0f, -35.0f };
	horseTriggerTransform.rotation = { 0.0f, 0.0f, 0.0f };
	horseTriggerTransform.scale = { 1.0f, 1.0f, 1.0f };

	auto& horseTriggerAABB = entity_registry->emplace<AABBColliderComponent>(horseTriggerEntity);
	horseTriggerAABB.halfExtents = { 3.0f, 2.0f, 3.0f };
	horseTriggerAABB.offset = { 0.0f, 2.0f, 0.0f };
	horseTriggerAABB.isTrigger = true;

	auto& horseTriggerSphere = entity_registry->emplace<SphereColliderComponent>(horseTriggerEntity);
	horseTriggerSphere.radius = glm::length(horseTriggerAABB.halfExtents);
	horseTriggerSphere.offset = horseTriggerAABB.offset;
	horseTriggerSphere.isTrigger = true;
}

void Game::renderQuestObjectiveWindow()
{
	ImGui::SetNextWindowPos(ImVec2{ 500.0f, 20.0f }, ImGuiCond_Always, ImVec2{ 0.0f, 0.0f });

	ImGui::PushStyleColor(ImGuiCol_WindowBg, 0x80000000);
	ImGui::PushStyleColor(ImGuiCol_Text, 0xffffffff);

	ImGuiWindowFlags flags =
		ImGuiWindowFlags_NoDecoration |
		ImGuiWindowFlags_NoInputs |
		ImGuiWindowFlags_AlwaysAutoResize;

	if (ImGui::Begin("quest_objective_window", nullptr, flags))
	{
		ImGui::Text("Quest Objective");
		ImGui::Separator();
		ImGui::TextWrapped("%s", questObjectiveText.c_str());
		ImGui::End();
	}

	ImGui::PopStyleColor(2);
}


