#include "CompanionSteeringBehavior.h"
#include "MainSingleton.h"

#include <algorithm>
#include <cmath>

#include <DreamEngine/debugging/LineDrawer.h>
#include <DreamEngine/debugging/DebugDrawer.h>
#include <DreamEngine/debugging/LinePrimitive.h>

#include <DreamEngine/graphics/ModelDrawer.h>
#include <DreamEngine/windows/settings.h> 
#include <DreamEngine/graphics/ModelInstance.h>

namespace
{
	// Constants
	constexpr float minDistance = 25.0f; 
	constexpr float extraFleeLength = 400.f; 
	constexpr float roatationUpOffset = 0.011f; 
	constexpr float sideOffset = 150.f; 
	constexpr float forwardOffset = 300.f; 
}

CompanionSteeringBehavior::CompanionSteeringBehavior()
{
	myRayLength = 200.f;
	myClosestCollision = 2000.0f;
    myMaxSpeed = 2000.f;
    myVelocity = 0.0f;
	mySlowingRadius = 1000.0f;
    mySeekWeight = 0.0f;
    myFleeWeight = 0.0f;
	myArivalWeight = 0.0f;
}

void CompanionSteeringBehavior::Init(DreamEngine::Transform aTransform)
{
    myTransform = aTransform;
}

DE::Vector3f CompanionSteeringBehavior::Update(float aDeltaTime, DE::Transform aTransform, DE::Vector3f aTarget)
{
	auto lenght = (aTarget - aTransform.GetPosition()).Length();
	if(lenght <= minDistance)
		return 0.0f;

    myTransform = aTransform;
    myTarget = aTarget;

	CalculateWeights();

	DreamEngine::Vector3f seekForce = SeekForce() * mySeekWeight;
	DreamEngine::Vector3f fleeForce = FleeForce() * myFleeWeight;
	DreamEngine::Vector3f arrivalForce = ArrivalForce(myTarget) * myArivalWeight;

	myVelocity += (seekForce + fleeForce + arrivalForce) * aDeltaTime;
	myVelocity = Truncate(myVelocity, myMaxSpeed);

	return myVelocity;
}


DE::Vector3f CompanionSteeringBehavior::ArrivalForce(const DreamEngine::Vector3f aDirection)
{
	auto desiredVelocity = aDirection - myTransform.GetPosition();
	float distance = (desiredVelocity).Length();

	if(distance < mySlowingRadius)
	{
		float scale = (distance / mySlowingRadius);
		desiredVelocity = desiredVelocity.GetNormalized() * (myMaxSpeed * scale);
	}
	else
		desiredVelocity = desiredVelocity.GetNormalized() * myMaxSpeed;

	return desiredVelocity - myVelocity;
}

DE::Vector3f CompanionSteeringBehavior::SeekForce()
{
	DreamEngine::Vector3f toTarget = myTarget - myTransform.GetPosition();
	DreamEngine::Vector3f desiredVelocity = toTarget.GetNormalized() * myMaxSpeed;
	mySeekForce = desiredVelocity - myVelocity;

	return mySeekForce;
}

DE::Vector3f CompanionSteeringBehavior::FleeForce()
{
	std::vector<eRayDir> collisionDirections = DirectionAvoidance();
	DreamEngine::Vector3f fleeDirection;

	for(eRayDir dir : collisionDirections)
	{
		if(dir != eRayDir::count)
		{
			auto matrix = myTransform.GetMatrix();

			switch(dir)
			{
			case eRayDir::Forward:
			{
				DE::Vector3f directionR = (matrix.GetForward() + matrix.GetRight()).GetNormalized();
				DE::Vector3f directionL = (matrix.GetForward() + matrix.GetRight() * -1.0f).GetNormalized();

				if(CollisionCheck(myTransform.GetPosition(), directionR, extraFleeLength) && CollisionCheck(myTransform.GetPosition(), directionL, extraFleeLength) && myClosestCollision < 70.f)
				{
					fleeDirection = matrix.GetForward() * -1.0f;
					fleeDirection = matrix.GetUp();
					break;
				}
				else
				{
					if(CollisionCheck(myTransform.GetPosition(), directionR, extraFleeLength))	//checking a bit futher away 
					{
						fleeDirection = matrix.GetRight() * -1.0f;
						fleeDirection += matrix.GetForward() * -1.0f;
						break;
					}
					if(CollisionCheck(myTransform.GetPosition(), directionL, extraFleeLength))
					{
						fleeDirection += matrix.GetRight();
						fleeDirection += matrix.GetForward() * -1.0f;
						break;
					}
				}
				break;
			}
			case eRayDir::Back:
			fleeDirection += matrix.GetForward();
			break;
			case eRayDir::Up:
			fleeDirection += matrix.GetUp() * -1.0f;
			break;
			case eRayDir::Down:
			fleeDirection += matrix.GetUp();
			break;
			case eRayDir::Right:
			fleeDirection += matrix.GetRight() * -1.0f;
			break;
			case eRayDir::Left:
			fleeDirection += matrix.GetRight();
			break;
			}
		}
		else
			myFleeForce = 0.0f;
	}

	fleeDirection = fleeDirection.GetNormalized();

	DreamEngine::Vector3f desiredVelocity = fleeDirection * myMaxSpeed;
	myFleeForce = desiredVelocity - myVelocity;

	return myFleeForce;
}

bool CompanionSteeringBehavior::CollisionCheck(const DreamEngine::Vector3f aPosition, const DreamEngine::Vector3f aDirection, float anAdditionalLength)
{
	physx::PxVec3 origin = physx::PxVec3(aPosition.x, aPosition.y, aPosition.z); 
	physx::PxVec3 direction = physx::PxVec3(aDirection.x, aDirection.y, aDirection.z); 

	auto collisionFiltering = MainSingleton::GetInstance()->GetCollisionFiltering();
	physx::PxQueryFilterData queryFilterData;
	queryFilterData.data.word0 = collisionFiltering.Environment;

	physx::PxRaycastBufferN<64> hitInfo;
	if(MainSingleton::GetInstance()->GetPhysXScene()->raycast(
		origin, direction, (myRayLength + anAdditionalLength), hitInfo, physx::PxHitFlag::eDEFAULT, queryFilterData))
	{
		for(physx::PxU32 j = 0; j < hitInfo.nbTouches; ++j)
		{
			const physx::PxRaycastHit& hit = hitInfo.touches[j];
			if(hit.actor->getName(), "Companion" == 0) continue; // Ignore own body
			myCollisionDist = hit.distance;

			return true;
		}
	}

	myCollisionDist = myRayLength;
	return false;
}

std::vector<eRayDir> CompanionSteeringBehavior::DirectionAvoidance()
{
	struct RayInfo
	{
		eRayDir direction;
		float closestCollision;
		DE::Vector3f position;
	};

	DE::Matrix4x4f matrix = myTransform.GetMatrix();
	std::vector<RayInfo> rayInfoList;

	for(int i = 0; i < static_cast<int>(eRayDir::count); ++i)
	{
		eRayDir currentDir = static_cast<eRayDir>(i);
		DE::Vector3f direction;
		DE::Vector3f position = myTransform.GetPosition();

		// Determine the direction vector based on the current ray direction
		switch(currentDir)
		{
		case eRayDir::Forward:
		direction = matrix.GetForward().GetNormalized();
		break;
		case eRayDir::Back:
		direction = (matrix.GetForward() * -1.0f).GetNormalized();
		break;
		case eRayDir::Right:
		direction = matrix.GetRight().GetNormalized();
		break;
		case eRayDir::Left:
		direction = (matrix.GetRight() * -1.0f).GetNormalized();
		break;
		default:
		continue;
		}

		// Check for collisions in the current direction
		if(CollisionCheck(position, direction, 0.0f))
		{
			rayInfoList.push_back({currentDir, myCollisionDist, position});
		}
	}

	float closestDist = myRayLength;
	myClosestCollision = myRayLength;
	std::vector<eRayDir> resultDirections;

	if(rayInfoList.size() == 2)
	{
		for(const auto& info : rayInfoList)
		{
			if(closestDist == 0.0f || closestDist > info.closestCollision)
			{
				closestDist = info.closestCollision;
			}
		}

		myClosestCollision = closestDist;
		resultDirections.push_back(rayInfoList[0].direction);
		resultDirections.push_back(rayInfoList[1].direction);
	}
	else if(!rayInfoList.empty())
	{
		RayInfo closestRayInfo = rayInfoList[0];
		for(const auto& info : rayInfoList)
		{
			if(closestDist == 0.0f || closestDist > info.closestCollision)
			{
				closestDist = info.closestCollision;
				closestRayInfo = info;
			}
		}

		myClosestCollision = closestDist;
		resultDirections.push_back(closestRayInfo.direction);
	}

	return resultDirections;
}

DE::Vector3f CompanionSteeringBehavior::Truncate(const DreamEngine::Vector3f aDirection, float aSpeed)
{
	float length = aDirection.Length();
	if(length > aSpeed)
	{
		return aDirection.GetNormalized() * aDirection;
	}
	return aDirection;
}

DE::Vector3f CompanionSteeringBehavior::RotateToThisOverTime(DreamEngine::Vector3f aPoint, float aDeltaTime, float aRotationSpeed, DreamEngine::Vector3f aCurrentRotation)
{
	auto myCurrentDir = aPoint.GetNormalized();

	DreamEngine::Vector3f up = DreamEngine::Vector3f(0, 1, 0) + myCurrentDir * 0.011f;
	up = up.GetNormalized();

	DreamEngine::Vector3f forward = {0.f, 1.f, 0.f};
	forward = (forward - forward.Dot(up) * up).GetNormalized();
	forward *= -1.f;

	DreamEngine::Vector3f right = up.Cross(forward).GetNormalized();

	DE::Quatf q = DE::Quatf::CreateFromOrthonormalBasisVectors(right, up, forward);
	DE::Vector3f rotation = q.GetEulerAnglesDegrees();

	DE::Vector3f delta = rotation - aCurrentRotation;

	while(delta.x > 180) delta.x -= 360;
	while(delta.x < -180) delta.x += 360;
	while(delta.y > 180) delta.y -= 360;
	while(delta.y < -180) delta.y += 360;
	while(delta.z > 180) delta.z -= 360;
	while(delta.z < -180) delta.z += 360;

	DE::Vector3f newRotation = aCurrentRotation + delta * aRotationSpeed * aDeltaTime;

	return newRotation;
}

DE::Vector3f CompanionSteeringBehavior::RotateToThis(DreamEngine::Vector3f aPoint)
{
	auto myCurrentDir = aPoint.GetNormalized();

	DreamEngine::Vector3f up = DreamEngine::Vector3f(0, 1, 0) + myCurrentDir * 0.011f;
	up = up.GetNormalized();

	DreamEngine::Vector3f forward = {0.f, 1.f, 0.f};
	forward = (forward - forward.Dot(up) * up).GetNormalized();
	forward *= -1.f;

	DreamEngine::Vector3f right = up.Cross(forward).GetNormalized();

	DE::Quatf q = DE::Quatf::CreateFromOrthonormalBasisVectors(right, up, forward);
	DE::Vector3f rotation = q.GetEulerAnglesDegrees();

	return rotation;
}

DE::Vector3f CompanionSteeringBehavior::RotateToVelocity()
{
	auto myCurrentDir = myVelocity.GetNormalized();

	DreamEngine::Vector3f up = DreamEngine::Vector3f(0, 1, 0) + myCurrentDir * 0.011f;
	up = up.GetNormalized();

	DreamEngine::Vector3f forward = {0.f, 1.f, 0.f};
	forward = (forward - forward.Dot(up) * up).GetNormalized();
	forward *= -1.f;

	DreamEngine::Vector3f right = up.Cross(forward).GetNormalized();

	DE::Quatf q = DE::Quatf::CreateFromOrthonormalBasisVectors(right, up, forward);
	DE::Vector3f rotation = q.GetEulerAnglesDegrees();

	return rotation;
}

DE::Vector3f CompanionSteeringBehavior::SetOffsetToPlayer(DE::Vector3f aPlayerPos)
{
	DreamEngine::Vector3f offset;
	DreamEngine::Vector3f collidiongOffset = {0.0f,0.0f,0.0f};

	switch(myBilateral)
	{
	case Bilateral::Left: 
	{
		auto cam = MainSingleton::GetInstance()->GetActiveCamera()->GetTransform().GetMatrix();
		offset = offset + (cam.GetForward().GetNormalized() * forwardOffset);
		offset = offset + (cam.GetRight().GetNormalized() * -sideOffset);
		offset.y += myRayLength;
		offset += collidiongOffset;

		break;
	}
	case Bilateral::Right:
	{
		auto cam = MainSingleton::GetInstance()->GetActiveCamera()->GetTransform().GetMatrix();
		offset = offset + (cam.GetForward().GetNormalized() * forwardOffset);
		offset = offset + (cam.GetRight().GetNormalized() * sideOffset);
		offset.y += myRayLength;
		offset += collidiongOffset;

		break;
	}
	case Bilateral::Reset:
	{
		offset = {0.0f,0.0f,0.0f};
		break;
	}
	default:
	break;
	}

	auto test = aPlayerPos.y + myRayLength;

	if(offset.y < myRayLength)
	{
		offset.y = myRayLength;
	}

	return offset;
}

Bilateral CompanionSteeringBehavior::ChoseClosesBilateral(DE::Vector3f aPlayerPos)
{
	myBilateral = Bilateral::Left;
	auto offset = SetOffsetToPlayer(aPlayerPos);
	auto offsetPosLeft = aPlayerPos + offset;
	float distL = (myTransform.GetPosition() - offsetPosLeft).Length();

	myBilateral = Bilateral::Right;
	offset = SetOffsetToPlayer(aPlayerPos);
	auto offsetPosRight = aPlayerPos + offset;
	float distR = (myTransform.GetPosition() - offsetPosRight).Length();

	distL < distR ? myBilateral = Bilateral::Left : myBilateral = Bilateral::Right;

	return myBilateral;
}

void CompanionSteeringBehavior::CalculateWeights()
{
	myFleeWeight = max(0.0f, 1.0f - (myClosestCollision / myRayLength));
	myFleeWeight = TruncateToOneDecimal(myFleeWeight);

	float distanceToTarget = (myTarget - myTransform.GetPosition()).Length();
	myArivalWeight = max(0.0f, 1.0f - (distanceToTarget - minDistance) / mySlowingRadius);
	myArivalWeight = TruncateToOneDecimal(myArivalWeight);

	mySeekWeight = max(0.0f, 1.0f - myFleeWeight - myArivalWeight - myPredictWeight);
	mySeekWeight = TruncateToOneDecimal(mySeekWeight);
}
