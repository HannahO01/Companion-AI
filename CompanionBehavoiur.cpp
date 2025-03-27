#include "CompanionBehavoiur.h"
#include "Node.h"
#include "ProjectilePool.h"

#include <iostream>
#include <algorithm>

#include <DreamEngine/windows/settings.h>
#include <DreamEngine/graphics/TextureManager.h>
#include <DreamEngine/graphics/ModelDrawer.h>
#include <DreamEngine/graphics/ModelFactory.h>
#include <DreamEngine/windows/settings.h>

namespace
{
	// Constants
	constexpr float turretCooldown = 20.0f;
	constexpr float turretDuration = 10.0f;
	constexpr float shootCooldown = 1.0f;
	constexpr float HealCooldown = 15.0f;
	constexpr float conversationInterval = 50.0f;
	constexpr float PickupDistance = 100.0f;
	constexpr float DropDistance = 100.0f;
	constexpr float introHeightOffset = 130.0f;
	constexpr float introCompletionDistance = 25.0f;
	constexpr float healtPackOffset = 30.0f;
}

CompanionBehavior::CompanionBehavior()
{
	myBehaviourTree = std::make_shared<BehaviourTree>(
		Builder()
		.Composites<Selector>()
			.Composites<HaveNoOrder>(this)
				.Leaf<Intro>(this)
				.Composites<FollowPlayer>(this)
					.Leaf<ShootEnemy>(this)
				.End()		// Close FollowPlayer
			.End()			// Close HaveNoOrder

			.Composites<HaveOrder>(this)
				.Composites<Fetch>(this)
					.Composites<PickUp>(this)
					.End() // close pickup
					.Composites<DropOff>(this)
					.End()	// close Drop off
				.End()		//close fetch

				.Composites<Turret>(this)
					.Leaf<ShootEnemy>(this)
				.End()		// close turret

			.End()			// close have order
		.End()				//close root
	.Build() 
	);
}

CompanionBehavior::~CompanionBehavior()
{}

void CompanionBehavior::Init(std::shared_ptr<DreamEngine::ModelInstance> aModel)
{
	myBehaviourTree->Init();
	InitAudio();

	context.modelInstance = aModel;
	context.turretTimer.SetThresholdValue(turretDuration); 
	context.turretCooldown.SetThresholdValue(turretCooldown);
	context.shootTimer.SetThresholdValue(shootCooldown); 
	context.healCooldown.SetThresholdValue(HealCooldown);
	context.conversationTimer.SetThresholdValue(conversationInterval);

	context.hasPickedUp = false;
	context.noShooting = false;

	context.modelInstanceHealthPack = std::make_shared<DreamEngine::ModelInstance>(
		DreamEngine::ModelFactory::GetInstance().GetModelInstance(L"3D/SM_P_Healthpack.fbx"));	

	context.projectilePool = new ProjectilePool(1, false);
}

DreamEngine::Vector3f CompanionBehavior::Update(float aDeltaTime)
{
	myBehaviourTree->Update();

	context.turretTimer.Update(aDeltaTime);
	context.turretCooldown.Update(aDeltaTime);
	context.shootTimer.Update(aDeltaTime);
	context.healCooldown.Update(aDeltaTime);
	context.conversationTimer.Update(aDeltaTime);

	if(context.projectilePool)
		context.projectilePool->Update(aDeltaTime);
	
	SetTexture();


	if(MainSingleton::GetInstance()->GetInputManager().IsKeyDown(DreamEngine::eKeyCode::H))
	{
		context.noShooting = !context.noShooting;
	}

	if(context.turretCooldown.ReachedThreshold() && !context.hasSentCoolDownMSG)
	{
		bool activationMessageData = true;
		MainSingleton::GetInstance()->GetPostMaster().TriggerMessage({ &activationMessageData, eMessageType::CompanionTurretCooldownToggle });

		context.hasSentCoolDownMSG = true;
	}
	if(context.healCooldown.ReachedThreshold() && context.hasHealingCoolDown)
	{
		bool activationMessageData = true;
		MainSingleton::GetInstance()->GetPostMaster().TriggerMessage({ &activationMessageData, eMessageType::CompanionHealthCooldownToggle });

		context.hasHealingCoolDown = false;
	}	
	if(context.conversationTimer.ReachedThreshold())
	{
		PlayRandomSound();
		context.conversationTimer.Reset();
	}

	return context.targetPosition;
}

void CompanionBehavior::Render(DE::GraphicsEngine& aGraphicsEngine)
{
	if(context.hasPickedUp)
		aGraphicsEngine.GetModelDrawer().DrawGBCalc(*context.modelInstanceHealthPack.get());

	if(context.projectilePool)
		context.projectilePool->Render(aGraphicsEngine);
}

void CompanionBehavior::SetContext(const CompanionContext& someStateToRead)
{
	context.transform = someStateToRead.transform;
	context.playerPos = someStateToRead.playerPos;
	context.closesHealingStation = someStateToRead.closesHealingStation;
	context.enemyPosition = someStateToRead.enemyPosition;
	context.enemyTransform = someStateToRead.enemyTransform;
	context.seesEnemy = someStateToRead.seesEnemy;
}

void CompanionBehavior::InitAudio()
{
	myAudios.push_back(eAudioEvent::CompanionVL1);
	myAudios.push_back(eAudioEvent::CompanionVL2);
	myAudios.push_back(eAudioEvent::CompanionVL3);
	myAudios.push_back(eAudioEvent::CompanionVL4);
	myAudios.push_back(eAudioEvent::CompanionVL5);
	myAudios.push_back(eAudioEvent::CompanionVL6);
	myAudios.push_back(eAudioEvent::CompanionVL7);
	myAudios.push_back(eAudioEvent::CompanionVL8);
	myAudios.push_back(eAudioEvent::CompanionVL9);
	myAudios.push_back(eAudioEvent::CompanionVL10);
	myAudios.push_back(eAudioEvent::CompanionVL11);
	myAudios.push_back(eAudioEvent::CompanionVL12);
	myAudios.push_back(eAudioEvent::CompanionVL13);
	myAudios.push_back(eAudioEvent::CompanionVL14);
	myAudios.push_back(eAudioEvent::CompanionVL15);
	myAudios.push_back(eAudioEvent::CompanionVL16);
	myAudios.push_back(eAudioEvent::CompanionVL17);
	myAudios.push_back(eAudioEvent::CompanionVL18);
	myAudios.push_back(eAudioEvent::CompanionVL19);
	myAudios.push_back(eAudioEvent::CompanionVL20);
	myAudios.push_back(eAudioEvent::CompanionVL21);
	myAudios.push_back(eAudioEvent::CompanionVL22);
	myAudios.push_back(eAudioEvent::CompanionVL23);
	myAudios.push_back(eAudioEvent::CompanionVL24);
}

void CompanionBehavior::PlayRandomSound()
{
	int soundNr = GetRandomInt(0, (int)myAudios.size() - 1); 

	MainSingleton::GetInstance()->GetAudioManager().StopAudio(myAudios[soundNr]);
	MainSingleton::GetInstance()->GetAudioManager().PlayAudio(myAudios[soundNr]);
}

void CompanionBehavior::SetTexture()
{
	std::wstring nameC;
	std::wstring nameN;
	std::wstring nameM;
	std::wstring nameFX;

	switch(GetOrder())
	{
	case CompanionBehavior::Orders::Fetch:
	{
		nameC = DreamEngine::Settings::ResolveAssetPathW(L"3D/T_CH_CompanionHappy_c.dds");
		nameN = DreamEngine::Settings::ResolveAssetPathW(L"3D/T_CH_CompanionHappy_n.dds");
		nameM = DreamEngine::Settings::ResolveAssetPathW(L"3D/T_CH_CompanionHappy_m.dds");
		nameFX = DreamEngine::Settings::ResolveAssetPathW(L"3D/T_CH_CompanionHappy_fx.dds");
		Texture(nameC.c_str(), nameN.c_str(), nameM.c_str(), nameFX.c_str());

		break;
	}
	case CompanionBehavior::Orders::FollowPlayer:
	{
		nameC = DreamEngine::Settings::ResolveAssetPathW(L"3D/T_CH_Companion_c.dds");
		nameN = DreamEngine::Settings::ResolveAssetPathW(L"3D/T_CH_Companion_n.dds");
		nameM = DreamEngine::Settings::ResolveAssetPathW(L"3D/T_CH_Companion_m.dds");
		nameFX = DreamEngine::Settings::ResolveAssetPathW(L"3D/T_CH_Companion_fx.dds");
		Texture(nameC.c_str(), nameN.c_str(), nameM.c_str(), nameFX.c_str());
		break;
	}
	case CompanionBehavior::Orders::Turret:
	{
		nameC = DreamEngine::Settings::ResolveAssetPathW(L"3D/T_CH_CompanionAngry_c.dds");
		nameN = DreamEngine::Settings::ResolveAssetPathW(L"3D/T_CH_CompanionAngry_n.dds");
		nameM = DreamEngine::Settings::ResolveAssetPathW(L"3D/T_CH_CompanionAngry_m.dds");
		nameFX = DreamEngine::Settings::ResolveAssetPathW(L"3D/T_CH_CompanionAngry_fx.dds");
		Texture(nameC.c_str(), nameN.c_str(), nameM.c_str(), nameFX.c_str());
		break;
	}
	default:
	break;
	}
}

void CompanionBehavior::Texture(const wchar_t* aColorPath, const wchar_t* aNormalPath, const wchar_t* aMaterialPath, const wchar_t* aEmessivePath)
{
	DreamEngine::Engine& engine = *DreamEngine::Engine::GetInstance();
	DreamEngine::Texture* textureC = nullptr;
	DreamEngine::Texture* textureN = nullptr;
	DreamEngine::Texture* textureM = nullptr;
	DreamEngine::Texture* textureFX = nullptr;

	textureC = engine.GetTextureManager().GetTexture(aColorPath, true);
	textureN = engine.GetTextureManager().GetTexture(aNormalPath, false);
	textureM = engine.GetTextureManager().GetTexture(aMaterialPath, false);
	textureFX = engine.GetTextureManager().GetTexture(aEmessivePath, false);

	for(size_t i = 0; i < context.modelInstance->GetModel()->GetMeshCount(); i++)
	{
		context.modelInstance->SetTexture(i, 0, textureC);	// 0 = Colour
		context.modelInstance->SetTexture(i, 1, textureN); // 1 = normal
		context.modelInstance->SetTexture(i, 2, textureM); // 2 = material
		context.modelInstance->SetTexture(i, 3, textureFX); // 3 = FX 
	}
}

Node::Status HaveNoOrder::Update()
{
	//when no succsed
	if(myController->GetOrder() == CompanionBehavior::Orders::Fetch ||
	   myController->GetOrder() == CompanionBehavior::Orders::Turret)
		return Status::Failure;

	bool running = false;

	for(size_t i = 0; i < myChildren.size(); i++)
	{
		myChildren[i]->Update();
		if(myChildren[i]->CheckStatus(Status::Running))
		{
			running = true;
		}
	}

	return running ? Status::Running : Status::Success;
}

Node::Status HaveOrder::Update()
{
	if (myController->GetOrder() == CompanionBehavior::Orders::Turret) 
	{
		myChildren[1]->Update(); 
	}
	else if(myController->GetOrder() == CompanionBehavior::Orders::Fetch)
	{
		myChildren[0]->Update(); 
	}

	return myController->GetOrder() == CompanionBehavior::Orders::FollowPlayer ? Status::Failure : Status::Success;
}

Node::Status FollowPlayer::Update()
{
	if(myController->GetOrder() != CompanionBehavior::Orders::FollowPlayer)
		return Status::Failure;

	for(size_t i = 0; i < myChildren.size(); i++)
	{
		myChildren[i]->Update();
	}

	myController->SetOrder(CompanionBehavior::Orders::FollowPlayer);
	myController->context.targetPosition = myController->context.playerPos;

	return Status::Running;
}

Node::Status Fetch::Update()
{
	if(!myController->context.hasPickedUp)
	{
		myChildren[0]->Update();
		return Status::Running;
	}
	else
	{
		myChildren[1]->Update();
		return Status::Running;
	}

	return Status::Success;
}

Node::Status Turret::Update()
{
	if(!myController->context.turretCooldown.ReachedThreshold())	//on cooldown
	{
		myController->SetOrder(CompanionBehavior::Orders::FollowPlayer);
		return Status::Failure;
	}

	if(myController->context.turretPosition.Length() == 0)
	{
		myController->context.turretPosition = myController->context.playerPos;
		myController->context.turretPosition.y += myController->context.rayLength;
		myController->context.turretTimer.Reset();

		bool activationMessageData = true;
		MainSingleton::GetInstance()->GetPostMaster().TriggerMessage({ &activationMessageData, eMessageType::CompanionTurretActive });

		MainSingleton::GetInstance()->GetAudioManager().StopAudio(eAudioEvent::CompanionVL1);
		MainSingleton::GetInstance()->GetAudioManager().PlayAudio(eAudioEvent::CompanionVL1);

		//sending message to HUD
		bool deactivationMessageData = false;
		MainSingleton::GetInstance()->GetPostMaster().TriggerMessage({&deactivationMessageData, eMessageType::CompanionTurretCooldownToggle});
		//sending message to projectile
		MainSingleton::GetInstance()->GetPostMaster().TriggerMessage({&deactivationMessageData, eMessageType::CompanionTurretActive});
	}

	if(myController->context.turretTimer.ReachedThreshold())	//is done
	{
		myController->SetOrder(CompanionBehavior::Orders::FollowPlayer);

		myController->context.turretPosition = 0.0f;
		myController->context.turretCooldown.Reset();
		myController->context.hasSentCoolDownMSG = false;

		return Status::Success;
	}

	myChildren[0]->Update();

	myController->SetOrder(CompanionBehavior::Orders::Turret);
	myController->context.targetPosition = myController->context.turretPosition;
	return Status::Running;
}

Node::Status PickUp::Update()
{
	DreamEngine::Vector3f Hpos = myController->context.closesHealingStation;
	Hpos.y += myController->context.rayLength;

	DreamEngine::Vector3f Cpos = myController->context.transform.GetPosition();
	DreamEngine::Vector3f target = Hpos - Cpos;

	float dist = (target).Length();
	if(dist < PickupDistance)
	{
		myController->context.hasPickedUp = true;
		return Status::Success;
	}

	myController->SetOrder(CompanionBehavior::Orders::Fetch);
	myController->context.targetPosition = Hpos;
	return Status::Running;
}

Node::Status DropOff::Update()
{
	DreamEngine::Vector3f Ppos = myController->context.playerPos;
	Ppos.y += myController->context.rayLength;
	DreamEngine::Vector3f Cpos = myController->context.transform.GetPosition();
	DreamEngine::Vector3f target = Ppos - Cpos;
	float dist = (target).Length();

	// updating health pack transform //
	auto transformH = myController->context.transform;
	DE::Vector3f posH = transformH.GetPosition();
	posH.y -= healtPackOffset;
	transformH.SetPosition(posH);
	myController->context.modelInstanceHealthPack->SetTransform(transformH);

	if(dist < DropDistance)
	{
		MainSingleton::GetInstance()->GetPostMaster().TriggerMessage({ nullptr, eMessageType::PlayerTriggerHeal });

		bool deactivationMessageData = false;
		MainSingleton::GetInstance()->GetPostMaster().TriggerMessage({ &deactivationMessageData, eMessageType::CompanionHealthCooldownToggle });

		myController->context.everyOtherHealing = !myController->context.everyOtherHealing;
		if(myController->context.everyOtherHealing) 
		{
			MainSingleton::GetInstance()->GetAudioManager().StopAudio(eAudioEvent::CompanionHealing1);
			MainSingleton::GetInstance()->GetAudioManager().PlayAudio(eAudioEvent::CompanionHealing1);
		}
		else
		{
			MainSingleton::GetInstance()->GetAudioManager().StopAudio(eAudioEvent::CompanionHealing2);
			MainSingleton::GetInstance()->GetAudioManager().PlayAudio(eAudioEvent::CompanionHealing2);
		}

		myController->context.hasHealingCoolDown = true;
		myController->context.healCooldown.Reset();

		myController->context.hasPickedUp = false;
		myController->SetOrder(CompanionBehavior::Orders::FollowPlayer);

		return Status::Success;
	}

	myController->SetOrder(CompanionBehavior::Orders::Fetch);
	myController->context.targetPosition = Ppos;
	return Status::Running;
}

Node::Status ShootEnemy::Update()
{
	if(!myController->context.shootTimer.ReachedThreshold() || 
	   myController->context.noShooting == true ||
	   !myController->context.seesEnemy)
		return Status::Running;

	myController->context.shootTimer.Reset();
	
	DE::Vector3f enemyPosition = myController->context.enemyPosition;
	DE::Vector3f companionPosition = myController->context.transform.GetPosition();
	DE::Vector3f dirToEnemy = DE::Vector3f(enemyPosition - companionPosition);

	myController->context.projectilePool->GetProjectile(
		companionPosition, dirToEnemy.GetNormalized(), myController->context.enemyTransform);

	MainSingleton::GetInstance()->GetAudioManager().StopAudio(eAudioEvent::CompanionShoot);
	MainSingleton::GetInstance()->GetAudioManager().PlayAudio(eAudioEvent::CompanionShoot);

	return Status::Success;
}

Node::Status Intro::Update()
{
	if(myController->GetOrder() == CompanionBehavior::Orders::FollowPlayer || !myController->context.hasWokenUp)	// add this when thea is finished with cam movement!
		return Status::Failure;

	if(myController->context.introPosition.Length() == 0.0f)
	{
		DE::Vector3f pos = myController->context.transform.GetPosition();
		pos.y += introHeightOffset;
		myController->context.introPosition = pos;

		MainSingleton::GetInstance()->GetAudioManager().StopAudio(eAudioEvent::CompanionIntroduction);
		MainSingleton::GetInstance()->GetAudioManager().PlayAudio(eAudioEvent::CompanionIntroduction);
	}

	float lenght = (myController->context.introPosition - myController->context.transform.GetPosition()).Length();
	if(lenght < introCompletionDistance && lenght != 0.0f)
	{
		myController->SetOrder(CompanionBehavior::Orders::FollowPlayer);
		return Status::Success;
	}
	else
	{
		myController->context.targetPosition = myController->context.introPosition;
		return Status::Running;
	}
}
