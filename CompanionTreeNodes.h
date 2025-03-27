#pragma once
#include "BehaviourTree.h" 

class CompanionBehavior;

class HaveNoOrder: public Selector
{
public:
	HaveNoOrder(CompanionBehavior* aCompanionBehavior): myController(aCompanionBehavior) {}
	Status Update() override;
private:
	CompanionBehavior* myController;
};

class HaveOrder: public Selector
{
public:
	HaveOrder(CompanionBehavior* aCompanionBehavior): myController(aCompanionBehavior) {}
	Status Update() override;
private:
	CompanionBehavior* myController;
};

class FollowPlayer: public Sequence
{
public:
	FollowPlayer(CompanionBehavior* aCompanionBehavior): myController(aCompanionBehavior) {}
	Status Update() override;
private:
	CompanionBehavior* myController;
};

class Fetch: public Selector
{
public:
	Fetch(CompanionBehavior* aCompanionBehavior): myController(aCompanionBehavior) {}
	Status Update() override;
private:
	CompanionBehavior* myController;
};

class Turret: public Sequence
{
public:
	Turret(CompanionBehavior* aCompanionBehavior): myController(aCompanionBehavior) {}
	Status Update() override;
private:
	CompanionBehavior* myController;
};

class PickUp: public Selector
{
public:
	PickUp(CompanionBehavior* aCompanionBehavior): myController(aCompanionBehavior) {}
	Status Update() override;
private:
	CompanionBehavior* myController;
};

class DropOff: public Selector
{
public:
	DropOff(CompanionBehavior* aCompanionBehavior): myController(aCompanionBehavior) {}
	Status Update() override;
private:
	CompanionBehavior* myController;
};

class ShootEnemy: public Leaf
{
public:
	ShootEnemy(CompanionBehavior* aCompanionBehavior): myController(aCompanionBehavior) {}
	Status Update() override;
private:
	CompanionBehavior* myController;
};

class Intro: public Leaf
{
public:
	Intro(CompanionBehavior* aCompanionBehavior): myController(aCompanionBehavior) {}
	Status Update() override;
private:
	CompanionBehavior* myController;
};
