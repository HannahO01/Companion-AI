#pragma once
#include "BehaviourTree.h"
#include "CompanionContext.h"
#include "MainSingleton.h"
#include "CompanionTreeNodes.h"

#include <DreamEngine/graphics/ModelInstance.h>
#include <DreamEngine/graphics/GraphicsEngine.h>
#include <DreamEngine/math/Vector.h>
#include <DreamEngine/math/Matrix.h>
#include <memory>
#include <vector>
#include <utility> 

class CompanionBehavior
{
public:
	enum class Orders { FollowPlayer, Fetch, Turret, Intro };

	CompanionBehavior();
	~CompanionBehavior();

	void Init(std::shared_ptr<DreamEngine::ModelInstance> aModel);
	DreamEngine::Vector3f Update(float aDeltaTime);
	void Render(DE::GraphicsEngine& aGraphicsEngine);

	void SetContext(const CompanionContext& someStateToRead);

	Orders GetOrder() { return myOrder; }
	void SetOrder(Orders aOrder) { myOrder = aOrder; }

	void InitAudio();
	void PlayRandomSound();

	void SetTexture();
	void Texture(const wchar_t* aColorPath,
		const wchar_t* aNormalPath,
		const wchar_t* aMaterialPath,
		const wchar_t* aEmessivePath);

	int GetRandomInt(int min, int max)
	{
		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_real_distribution<> dis(min, max);
		auto result = (int)dis(gen);
		return result;
	}

	CompanionContext context;

private:
	Orders myOrder = Orders::Intro;
	std::shared_ptr<BehaviourTree> myBehaviourTree;
	std::vector<eAudioEvent> myAudios;
};