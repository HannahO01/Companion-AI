#pragma once
#include <DreamEngine/math/Matrix4x4.h>
#include <DreamEngine/math/Vector.h>
#include <DreamEngine/math/Transform.h>
#include <DreamEngine/utilities/CountTimer.h>
#include <DreamEngine/graphics/ModelInstance.h>

struct CompanionMovementRead
{ // Read only
	DreamEngine::Transform transform;
	DreamEngine::Vector3f playerPosition;
	DreamEngine::Vector3f healingStationPosition;
	DreamEngine::Vector3f targetedEnemyPosition;
};

struct CompanionMovementContext
{
	CU::CountupTimer directionCooldown;

	DreamEngine::Transform transform;
	DreamEngine::Vector3f offset;
	DreamEngine::Vector3f targetPosition;
	DreamEngine::Vector3f playerPos;
	DreamEngine::Vector3f closesHealingStation;
	DreamEngine::Vector3f enemyPosition;
	DreamEngine::Vector3f turretPosition = {0.0f,0.0f,0.0f};
	DreamEngine::Vector3f velocity = {0.0f,0.0f,0.0f};
	DreamEngine::Vector3f lastTargetPos = {0.0f,0.0f,0.0f};

	float deltaTime;
	float maxVelocity;
	float maxForce;
	float rayLength;
	float maxSpeed;
	float speed;
	float mass;
	float dotProduct;

	bool hasPickedUp;
	bool approaching;
	bool colliding;
};

class ProjectilePool;
struct CompanionContext
{
	std::shared_ptr<DreamEngine::ModelInstance> modelInstance;
	std::shared_ptr<DreamEngine::ModelInstance> modelInstanceHealthPack;
	DreamEngine::Transform transform;
	DreamEngine::Transform* enemyTransform;

	CU::CountupTimer turretTimer;
	CU::CountupTimer shootTimer;
	CU::CountupTimer turretCooldown;
	CU::CountupTimer healCooldown;
	CU::CountupTimer conversationTimer;

	DreamEngine::Vector3f targetPosition = 0.0f;
	DreamEngine::Vector3f playerPos = 0.0f;
	DreamEngine::Vector3f closesHealingStation = 0.0f;
	DreamEngine::Vector3f enemyPosition = 0.0f;
	DreamEngine::Vector3f turretPosition = 0.0f;
	DreamEngine::Vector3f introPosition = 0.0f;

	float rayLength = 200.f;
	float shootingLength = 1000.f;

	bool hasPickedUp;
	bool noShooting = true;
	bool seesEnemy = false;
	bool hasSentCoolDownMSG = true;
	bool hasHealingCoolDown = false;
	bool hasWokenUp = false;
	bool everyOtherHealing = false;

	ProjectilePool* projectilePool = nullptr;
};