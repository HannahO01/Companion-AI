#pragma once
#include <DreamEngine\math\Vector3.h>
#include <DreamEngine/math/Transform.h>
#include <DreamEngine/graphics/GraphicsEngine.h>

enum class Bilateral { Left, Right, Reset };
enum class eRayDir { Forward, Back, Up, Down, Right, Left, count };

class CompanionSteeringBehavior
{
public:
	CompanionSteeringBehavior();

	void Init(DreamEngine::Transform aTransform);
	DE::Vector3f Update(float aDeltaTime, DE::Transform aTransform, DE::Vector3f aTarget);

	// Steering forces
	DE::Vector3f ArrivalForce(const DreamEngine::Vector3f aDirection);
	DE::Vector3f SeekForce();
	DE::Vector3f FleeForce();

	// Rotation methods
	DE::Vector3f RotateToThisOverTime(DreamEngine::Vector3f aPoint, float aDeltaTime, float aRotationSpeed, DreamEngine::Vector3f aCurrentRotation);
	DE::Vector3f RotateToThis(DreamEngine::Vector3f aPoint);
	DE::Vector3f RotateToVelocity();

	// Position/offset methods
	DE::Vector3f SetOffsetToPlayer(DE::Vector3f aPlayerPos);
	Bilateral ChoseClosesBilateral(DE::Vector3f aPlayerPos);

private:
	bool CollisionCheck(const DreamEngine::Vector3f aPosition, const DreamEngine::Vector3f aDirection, float anAdditionalLenght);
	std::vector<eRayDir> DirectionAvoidance();

	void CalculateWeights();
	DE::Vector3f Truncate(const DreamEngine::Vector3f aDirection, float aSpeed);

	float TruncateToOneDecimal(float value) { return static_cast<int>(value * 10) / 10.0f; }

private:
	Bilateral myBilateral;
	DreamEngine::Transform myTransform;
	DE::Vector3f myTarget;
	DE::Vector3f myVelocity;
	DE::Vector3f mySeekForce;
	DE::Vector3f myFleeForce;
	DE::Vector3f myPredictForce;

	float myRayLength;
	float myMaxSpeed;
	float mySlowingRadius;
	float myClosestCollision;
	float myCollisionDist;
	float mySeekWeight = 0.0f;
	float myFleeWeight = 0.0f;
	float myArivalWeight = 0.0f;
	float myPredictWeight = 0.0f;
};
