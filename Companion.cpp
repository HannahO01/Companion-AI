#include "Companion.h"
#include "Message.h"
#include "Player.h"
#include "FlyingEnemy.h"
#include "GroundEnemy.h"
#include "MainSingleton.h"
#include "EnemyPool.h"
#include "RigidBodyComponent.h"
#include "DreamEngine/graphics/PointLight.h" 
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
	MainSingleton::GetInstance()->GetPostMaster().Unsubscribe(eMessageType::PlayerRespawned, this);
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
	MainSingleton::GetInstance()->GetPostMaster().Subscribe(eMessageType::PlayerRespawned, this);

	physx::PxRigidDynamic* body = static_cast<physx::PxRigidDynamic*>(GetComponent<RigidBodyComponent>()->GetBody());
	physx::PxTransform currentPose = body->getGlobalPose();
	physx::PxTransform updatedPose(physx::PxVec3(myTransform.GetPosition().x, myTransform.GetPosition().y, myTransform.GetPosition().z), currentPose.q);
	body->setGlobalPose(updatedPose);
}

void Companion::Update(float aDeltaTime)
{
	if (MainSingleton::GetInstance()->GetGameToPause())
		return;
	
	PrepareBehaviorContext();
	
	DreamEngine::Vector3f target = myBehavior.Update(aDeltaTime);
	DreamEngine::Vector3f steeringForce = SetSteering(aDeltaTime, target);
	
	UpdateRotation(aDeltaTime, steeringForce);
	UpdatePhysics(steeringForce);

	myModelInstance->SetTransform(*GetTransform());

	UpdatePointLight();
	
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
		if (myBehavior.GetOrder() != CompanionBehavior::Orders::FollowPlayer) { return; }
		myBehavior.SetOrder(CompanionBehavior::Orders::Fetch);
	}
	else if(aMessage.messageType == eMessageType::CompanionTurret)
	{
		if (myBehavior.GetOrder() != CompanionBehavior::Orders::FollowPlayer) { return; }
		myBehavior.SetOrder(CompanionBehavior::Orders::Turret);
	}
	else if(aMessage.messageType == eMessageType::CompanionStartIntro)
	{
		myBehavior.context.hasWokenUp = true;
	}
	else if (aMessage.messageType == eMessageType::PlayerRespawned)
	{
		GetTransform()->SetPosition(myPlayer->GetTransform()->GetPosition()); 
		myModelInstance->SetTransform(*GetTransform()); 
		
		physx::PxRigidDynamic* body = static_cast<physx::PxRigidDynamic*>(GetComponent<RigidBodyComponent>()->GetBody());
		physx::PxTransform currentPose = body->getGlobalPose();
		physx::PxTransform updatedPose(physx::PxVec3(myTransform.GetPosition().x, myTransform.GetPosition().y, myTransform.GetPosition().z), currentPose.q);
		body->setGlobalPose(updatedPose);

		MainSingleton::GetInstance()->GetAudioManager().StopAudio(eAudioEvent::CompanionRevive);
		MainSingleton::GetInstance()->GetAudioManager().PlayAudio(eAudioEvent::CompanionRevive, myTransform.GetPosition());
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

void Companion::SetPointLight(std::shared_ptr<DE::PointLight> aPointLightAbove, std::shared_ptr<DE::PointLight> aPointLightInside)
{
	myPointLightAbove = aPointLightAbove;
	myPointLightInside = aPointLightInside;
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
		float dist = (myHealingStationPos[i], GetTransform()->GetPosition()).Length();
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

void Companion::PrepareBehaviorContext()
{
	myModelInstance = myBehavior.context.modelInstance;

	myContext.transform = *GetTransform();
	myContext.playerPos = myPlayer->GetTransform()->GetPosition();
	myContext.closesHealingStation = CalculateClosesHealingStation();
	myContext.enemyPosition = myTargetEnemyTransform.GetPosition();
	myContext.enemyTransform = &myTargetEnemyTransform;

	myBehavior.SetContext(myContext);
}

DreamEngine::Vector3f Companion::SetSteering(float aDeltaTime, const DreamEngine::Vector3f& target)
{
	switch (myBehavior.GetOrder())
	{
	case CompanionBehavior::Orders::Fetch:
	case CompanionBehavior::Orders::Turret:
		return mySteeringBehavior->Update(aDeltaTime, myTransform.GetPosition(), target);

	case CompanionBehavior::Orders::FollowPlayer:
		return SetFollowPlayerSteering(aDeltaTime, target);

	case CompanionBehavior::Orders::Intro:
		return myBehavior.context.hasWokenUp
			? mySteeringBehavior->Update(aDeltaTime, myTransform.GetPosition(), target)
			: DreamEngine::Vector3f(0.0f);

	default:
		return DreamEngine::Vector3f(0.0f);
	}
}

DreamEngine::Vector3f Companion::SetFollowPlayerSteering(float aDeltaTime, const DreamEngine::Vector3f& target)
{
	if (Near(myTransform.GetPosition(), target, 1000.f))
	{
		mySteeringBehavior->ChoseClosesBilateral(myPlayer->GetTransform()->GetPosition());

		auto offsetPos = myPlayer->GetTransform()->GetPosition() +
			mySteeringBehavior->SetOffsetToPlayer(myPlayer->GetTransform()->GetPosition());

		return mySteeringBehavior->Update(aDeltaTime, myTransform.GetPosition(), offsetPos);
	}
	return mySteeringBehavior->Update(aDeltaTime, myTransform.GetPosition(), target);
}

void Companion::UpdatePointLight()
{
	switch (myBehavior.GetOrder())
	{
	case CompanionBehavior::Orders::Fetch: 
		myPointLightInside->SetColor({ 176.0f / 255.0f, 250.0f / 255.0f, 155.0f / 255.0f });
		break;
	case CompanionBehavior::Orders::Turret:
		myPointLightInside->SetColor({ 250.0f / 255.0f ,155.0f / 255.0f ,155.0f / 255.0f });
		break;
	case CompanionBehavior::Orders::FollowPlayer:
		myPointLightInside->SetColor({ 250.0f / 255.0f ,191.0f / 255.0f ,155.0f / 255.0f });
		myPointLightAbove->SetColor({ 1.0f,1.0f,1.0f });
		break;
	case CompanionBehavior::Orders::Intro:
		return;
	}

	DE::Vector3f pos = GetTransform()->GetPosition();

	myPointLightInside->SetPosition(pos);
	myPointLightInside->myObjPtr->SetLocation(pos);

	pos += GetTransform()->GetMatrix().GetForward().GetNormalized() * 100.0f;
	pos += GetTransform()->GetMatrix().GetUp().GetNormalized() * 50.0f;

	myPointLightAbove->SetPosition(pos);
	myPointLightAbove->myObjPtr->SetLocation(pos);
}

void Companion::UpdateRotation(float aDeltaTime, const DreamEngine::Vector3f& steeringForce)
{
	if (steeringForce.Length() == 0.0f)
		HandleStationaryRotation(aDeltaTime);
	else
		HandleMovingRotation();

	myModelInstance->SetRotation(myRotation);
	myTransform.SetRotation(myRotation);
}

void Companion::HandleStationaryRotation(float aDeltaTime)
{
	myTargetRotation = myBehavior.context.seesEnemy
		? myTargetEnemyTransform.GetPosition() - GetTransform()->GetPosition()
		: myPlayer->GetTransform()->GetPosition() - GetTransform()->GetPosition();

	myRotation = mySteeringBehavior->RotateToThisOverTime(myTargetRotation, aDeltaTime, 5.f, myRotation);
}

void Companion::HandleMovingRotation()
{
	if (myBehavior.context.seesEnemy)
	{
		myTargetRotation = myTargetEnemyTransform.GetPosition() - GetTransform()->GetPosition();
		myRotation = mySteeringBehavior->RotateToThis(myTargetRotation);
	}
	else
		myRotation = mySteeringBehavior->RotateToVelocity();
}

void Companion::UpdatePhysics(const DreamEngine::Vector3f& steeringForce)
{
	physx::PxRigidDynamic* body = static_cast<physx::PxRigidDynamic*>(GetComponent<RigidBodyComponent>()->GetBody());

	// Set velocity
	physx::PxVec3 velocity(steeringForce.x, steeringForce.y, steeringForce.z);
	body->setLinearVelocity(velocity);

	// Update position and handle gravity
	physx::PxTransform currentPose = body->getGlobalPose();

	if (steeringForce.Length() == 0.0f)
	{
		currentPose.p.y = myTransform.GetPosition().y;
		body->setActorFlag(physx::PxActorFlag::eDISABLE_GRAVITY, true);
	}
	else
		body->setActorFlag(physx::PxActorFlag::eDISABLE_GRAVITY, false);

	myTransform.SetPosition(DreamEngine::Vector3f(currentPose.p.x, currentPose.p.y, currentPose.p.z));
}
