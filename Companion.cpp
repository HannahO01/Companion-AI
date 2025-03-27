#include "Companion.h"
#include "Message.h"
#include "Player.h"
#include "FlyingEnemy.h"
#include "GroundEnemy.h"
#include "MainSingleton.h"
#include "EnemyPool.h"
#include "RigidBodyComponent.h"
#include <DreamEngine/windows/settings.h>
#include <DreamEngine/graphics/TextureManager.h>
#include <DreamEngine/graphics/ModelDrawer.h>
#include <PhysX\PxPhysicsAPI.h> 

Companion::Companion()
{
	myRotation = 0.f;
	myFoundEnemy = false;

	// fixing collision
	AddComponent<RigidBodyComponent>();
	auto* physXScene = MainSingleton::GetInstance()->GetPhysXScene();
	physx::PxShape* shape = DE::Engine::GetPhysXPhysics()->createShape(physx::PxSphereGeometry(25.0f), *MainSingleton::GetInstance()->GetPhysXMaterials()[0]);

	auto filter = MainSingleton::GetInstance()->GetCollisionFiltering();
	filter.setupFiltering(shape, filter.Companion, filter.Environment | filter.Enemy);

	physx::PxRigidDynamic* body = DE::Engine::GetPhysXPhysics()->createRigidDynamic(
		physx::PxTransform(myTransform.GetPosition().x, myTransform.GetPosition().y, myTransform.GetPosition().z));

	body->attachShape(*shape);
	physx::PxRigidBodyExt::updateMassAndInertia(*body, 50.0f);

	body->setRigidBodyFlag(physx::PxRigidBodyFlag::eKINEMATIC, false);

	physx::PxTransform currentPose = body->getGlobalPose();
	body->setName("Companion");
	physXScene->addActor(*body);
	GetComponent<RigidBodyComponent>()->SetBody(body);

	physx::PxTransform updatedPose(physx::PxVec3(myTransform.GetPosition().x, myTransform.GetPosition().y, myTransform.GetPosition().z), currentPose.q);
	body->setGlobalPose(updatedPose);
}

Companion::~Companion()
{
	MainSingleton::GetInstance()->GetPostMaster().Unsubscribe(eMessageType::CompanionFetch, this);
	MainSingleton::GetInstance()->GetPostMaster().Unsubscribe(eMessageType::CompanionTurret, this);
	MainSingleton::GetInstance()->GetPostMaster().Unsubscribe(eMessageType::CompanionStartIntro, this);
	MainSingleton::GetInstance()->GetPostMaster().Unsubscribe(eMessageType::PlayerDied, this);
}

void Companion::Init()
{
	myBehavior.Init(myModelInstance);
	myContext.closesHealingStation = CalculateClosesHealingStation();

	mySteeringBehavior = new CompanionSteeringBehavior;
	mySteeringBehavior->Init(myTransform); 

	MainSingleton::GetInstance()->GetPostMaster().Subscribe(eMessageType::CompanionFetch, this);
	MainSingleton::GetInstance()->GetPostMaster().Subscribe(eMessageType::CompanionTurret, this);
	MainSingleton::GetInstance()->GetPostMaster().Subscribe(eMessageType::CompanionStartIntro, this);
	MainSingleton::GetInstance()->GetPostMaster().Subscribe(eMessageType::PlayerDied, this);

	physx::PxRigidDynamic* body = static_cast<physx::PxRigidDynamic*>(GetComponent<RigidBodyComponent>()->GetBody());
	physx::PxTransform currentPose = body->getGlobalPose();
	physx::PxTransform updatedPose(physx::PxVec3(myTransform.GetPosition().x, myTransform.GetPosition().y, myTransform.GetPosition().z), currentPose.q);
	body->setGlobalPose(updatedPose);
}

void Companion::Update(float aDeltaTime)
{
	myModelInstance = myBehavior.context.modelInstance;

	myContext.transform = *GetTransform();
	myContext.playerPos = myPlayer->GetTransform()->GetPosition();
	myContext.closesHealingStation = CalculateClosesHealingStation();
	myContext.enemyPosition = myTargetEnemyTransform.GetPosition();
	myContext.enemyTransform = &myTargetEnemyTransform;

	myBehavior.SetContext(myContext);

	DreamEngine::Vector3f steeringForce;
	DreamEngine::Vector3f target = myBehavior.Update(aDeltaTime); 

	if(myBehavior.GetOrder() == CompanionBehavior::Orders::FollowPlayer)
	{
		if(Near(myTransform.GetPosition(), target, 1000.f))
		{
			mySteeringBehavior->ChoseClosesBilateral(myPlayer->GetTransform()->GetPosition());
			auto offsetPos = myPlayer->GetTransform()->GetPosition() + mySteeringBehavior->SetOffsetToPlayer(myPlayer->GetTransform()->GetPosition());
			steeringForce = mySteeringBehavior->Update(aDeltaTime, myTransform.GetPosition(), offsetPos);
		}
		else
		{
			steeringForce = mySteeringBehavior->Update(aDeltaTime, myTransform.GetPosition(), target);
		}
		if(steeringForce.Length() == 0.0f)
		{
			if(myBehavior.context.seesEnemy)
			{
				myTargetRotation = myTargetEnemyTransform.GetPosition() - GetTransform()->GetPosition();
				myRotation = mySteeringBehavior->RotateToThisOverTime(myTargetRotation, aDeltaTime, 5.f, myRotation);
			}
			else
			{
				myTargetRotation = myPlayer->GetTransform()->GetPosition() - GetTransform()->GetPosition();
				myRotation = mySteeringBehavior->RotateToThisOverTime(myTargetRotation, aDeltaTime, 5.f, myRotation);
			}
		}
		else
		{
			if(myBehavior.context.seesEnemy)
			{
				myTargetRotation = myTargetEnemyTransform.GetPosition() - GetTransform()->GetPosition();
				myRotation = mySteeringBehavior->RotateToThis(myTargetRotation);
			}
			else
				myRotation = mySteeringBehavior->RotateToVelocity();
		}
	}
	if(myBehavior.GetOrder() == CompanionBehavior::Orders::Turret)
	{
		steeringForce = mySteeringBehavior->Update(aDeltaTime, myTransform.GetPosition(), target);
		
		if(steeringForce.Length() == 0.0f)
		{
			myTargetRotation = myTargetEnemyTransform.GetPosition() - GetTransform()->GetPosition();
			myRotation = mySteeringBehavior->RotateToThisOverTime(myTargetRotation, aDeltaTime, 5.f, myRotation);
		}
		else
			myRotation = mySteeringBehavior->RotateToVelocity();

	}
	if(myBehavior.GetOrder() == CompanionBehavior::Orders::Fetch)
	{
		steeringForce = mySteeringBehavior->Update(aDeltaTime, myTransform.GetPosition(), target);

		myRotation = mySteeringBehavior->RotateToVelocity();
	}
	if(myBehavior.GetOrder() == CompanionBehavior::Orders::Intro)
	{
		if(myBehavior.context.hasWokenUp)
		{
			steeringForce = mySteeringBehavior->Update(aDeltaTime, myTransform.GetPosition(), target);

			myTargetRotation = myPlayer->GetTransform()->GetPosition() - GetTransform()->GetPosition();
			myRotation = mySteeringBehavior->RotateToThisOverTime(myTargetRotation, aDeltaTime, 5.f, myRotation);
		}
		else
			steeringForce = 0.0f;
	}


	physx::PxRigidDynamic* body = static_cast<physx::PxRigidDynamic*>(GetComponent<RigidBodyComponent>()->GetBody());
	physx::PxVec3 velocity(steeringForce.x, steeringForce.y, steeringForce.z); // Adjust the multiplier as needed
	body->setLinearVelocity(velocity);

	// Update the transform position to match the physics body
	if(!MainSingleton::GetInstance()->GetGameToPause())
	{
		physx::PxRigidDynamic* body = static_cast<physx::PxRigidDynamic*>(GetComponent<RigidBodyComponent>()->GetBody());
		physx::PxTransform currentPose = body->getGlobalPose();

		if(steeringForce.Length() == 0.0f)
		{
			currentPose.p.y = myTransform.GetPosition().y;
			body->setActorFlag(physx::PxActorFlag::eDISABLE_GRAVITY, true);
		}
		else
			body->setActorFlag(physx::PxActorFlag::eDISABLE_GRAVITY, false);

		myTransform.SetPosition(DreamEngine::Vector3f(currentPose.p.x, (currentPose.p.y), currentPose.p.z));
		myModelInstance->SetTransform(*GetTransform());
		myModelInstance->SetRotation(myRotation);
		myTransform.SetRotation(myRotation);
	}
}

void Companion::Render(DE::GraphicsEngine & aGraphicsEngine)
{
	myBehavior.Render(aGraphicsEngine);
	aGraphicsEngine.GetModelDrawer().DrawGBCalc(*myModelInstance.get());
}

void Companion::RenderVFX(DreamEngine::GraphicsStateStack& aGraphicsStateStack)
{
	aGraphicsStateStack;
}

void Companion::Receive(const Message & aMessage)
{
	if(aMessage.messageType == eMessageType::CompanionFetch)
	{
		myBehavior.SetOrder(CompanionBehavior::Orders::Fetch);
	}
	else if(aMessage.messageType == eMessageType::CompanionTurret)
	{
		myBehavior.SetOrder(CompanionBehavior::Orders::Turret);
	}
	else if(aMessage.messageType == eMessageType::CompanionStartIntro)
	{
		myBehavior.context.hasWokenUp = true;
	}
	else if(aMessage.messageType == eMessageType::PlayerRespawned)
	{//this message is not being used atm but will fix later
		myTransform.SetPosition(myPlayer->GetTransform()->GetPosition()); 
		myModelInstance->SetTransform(*GetTransform());
	}
}

void Companion::SetPlayer(std::shared_ptr<Player> aPlayer)
{
	myPlayer = aPlayer; 
}

void Companion::SetModelInstance(std::shared_ptr<DreamEngine::ModelInstance>& aModelInstance)
{
	myModelInstance = aModelInstance;
}

void Companion::SetTargetedEnemyPos(std::vector<std::shared_ptr<FlyingEnemy>> aEnemyFlyingPos, std::vector<std::shared_ptr<GroundEnemy>> aEnemyGroundPos)
{
	if(!myBehavior.context.shootTimer.ReachedThreshold())
		return;

	float dist = 0;
	int index = 0;
	DreamEngine::Transform transform;
	myContext.seesEnemy = false;

	for(int i = 0; i < aEnemyFlyingPos.size(); i++)
	{
		DreamEngine::Transform enemyTransform = *aEnemyFlyingPos[i]->GetTransform();
		if(!aEnemyFlyingPos[i]->IsAlive()) continue;
		if(dist == 0 || dist > (GetTransform()->GetPosition() - enemyTransform.GetPosition()).Length())
		{
			dist = (GetTransform()->GetPosition() - enemyTransform.GetPosition()).Length();
			index = i;
			transform = enemyTransform;
			myContext.seesEnemy = true;
		}
	}
	for(int i = 0; i < aEnemyGroundPos.size(); i++)
	{
		DreamEngine::Transform enemyTransform = *aEnemyGroundPos[i]->GetTransform();
		if(!aEnemyGroundPos[i]->IsAlive()) continue;
		if(dist == 0 || dist > (GetTransform()->GetPosition() - enemyTransform.GetPosition()).Length())
		{
			dist = (GetTransform()->GetPosition() - enemyTransform.GetPosition()).Length();
			index = i;
			transform = enemyTransform;
			myContext.seesEnemy = true;
		}
	}

	if(dist > myBehavior.context.shootingLength)
	{
		myContext.seesEnemy = false;
		return;
	}

	myTargetEnemyTransform = transform;
}

void Companion::AddHealingStationPos(DreamEngine::Vector3f aHealingStationPos)
{
	myHealingStationPos.push_back(aHealingStationPos); 
}

DreamEngine::Vector3f Companion::CalculateClosesHealingStation()
{
	if(myHealingStationPos.empty())
		return DreamEngine::Vector3f();

	float shortesDist = 0;
	int index = 0;

	for(int i = 0; i < myHealingStationPos.size(); i++)
	{
		float dist = (myHealingStationPos[i] - GetTransform()->GetPosition()).Length();
		if(shortesDist == 0 || shortesDist > dist)
		{
			shortesDist = dist;
			index = i;
		}
	}

	return myHealingStationPos[index];
}

bool Companion::Near(DreamEngine::Vector3f aPos, DreamEngine::Vector3f aTargetPos, float aLenght)
{
	float distans = (aPos - aTargetPos).Length();
	if(distans < aLenght)
	{
		return true;
	}
	return false;
}
