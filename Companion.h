#pragma once
#include "GameObject.h"
#include "Observer.h"
#include "CompanionBehavoiur.h"
#include "CompanionContext.h"
#include "CompanionSteeringBehavior.h"

#include <DreamEngine/utilities/CountTimer.h>
#include <DreamEngine/graphics/ModelInstance.h>
#include <DreamEngine/math/Vector.h>
#include <vector>
#include <utility> 

class Player; 
class EnemyPool;
class GroundEnemy;
class FlyingEnemy;

class Companion: public GameObject, public Observer
{
public:
	Companion();
	~Companion();

	void Init();

	void Update(float aDeltaTime) override;
	
	void Render(DE::GraphicsEngine& aGraphicsEngine) override;
	void RenderVFX(DreamEngine::GraphicsStateStack& aGraphicsStateStack);

	void OnCollision(GameObject* anOtherGameObject, eCollisionLayer aCollisionLayer) override {anOtherGameObject; aCollisionLayer;}
	void Receive(const Message& aMessage)override;

	void SetPlayer(std::shared_ptr<Player> aPlayer);
	void SetModelInstance(std::shared_ptr<DreamEngine::ModelInstance>& aModelInstance);
		
	void SetTargetedEnemyPos(std::vector<std::shared_ptr<FlyingEnemy>> aEnemyFlyingPos, std::vector<std::shared_ptr<GroundEnemy>> aEnemyGroundPos);

	void AddHealingStationPos(DreamEngine::Vector3f aHealingStationPos);
	DreamEngine::Vector3f CalculateClosesHealingStation(); 
	bool Near(DreamEngine::Vector3f aPos, DreamEngine::Vector3f aTargetPos, float aLenght);

private:
	CompanionBehavior myBehavior; 
	CompanionSteeringBehavior* mySteeringBehavior;
	std::shared_ptr<DreamEngine::ModelInstance> myModelInstance; 
	std::shared_ptr<Player> myPlayer;
	std::vector<DreamEngine::Vector3f> myHealingStationPos;

	CompanionMovementRead myReadState;  // Read-only state
	CompanionContext myContext;

	DreamEngine::Vector3f myRotation;
	DreamEngine::Vector3f myTargetRotation;
	DreamEngine::Transform myTargetEnemyTransform;
	bool myFoundEnemy;
};

