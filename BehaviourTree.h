#pragma once
#include "Node.h"
#include <vector>
#include <assert.h>

class BehaviourTree: public Node
{
public:
	BehaviourTree(): myRootNode(nullptr) {}
	BehaviourTree(const std::shared_ptr<Node>& aRootNode) : BehaviourTree() { myRootNode = aRootNode; }

	Status Update() { return myRootNode->Tick(); }
	void SetRoot(const std::shared_ptr<Node>& aRootNode) { myRootNode = aRootNode; }

private:
	std::shared_ptr<Node> myRootNode = nullptr;
};

class Composite: public Node
{
public:
	Composite() = default;
	virtual ~Composite() {}

	void AddChild(std::shared_ptr<Node> anAddedChild) { myChildren.push_back(anAddedChild); }

protected:
	std::vector<std::shared_ptr<Node>> myChildren;
	std::vector<std::shared_ptr<Node>>::iterator myIterator;
};

template <class Parent>
class CompositeBuilder
{
public:
	CompositeBuilder(Parent* aParent, Composite* aComposite): myParent(aParent), myComposite(aComposite) {}

	template <class NodeType, typename... Arguments>
	CompositeBuilder<Parent> Leaf(Arguments... someArguments)
	{
		auto child = std::make_shared<NodeType>((someArguments)...);
		myComposite->AddChild(child);
		return *this;
	}

	template <class CompositeType, typename... Arguments>
	CompositeBuilder<CompositeBuilder<Parent>> Composites(Arguments... someArguments)
	{
		auto child = std::make_shared<CompositeType>((someArguments)...);
		myComposite->AddChild(child);
		return CompositeBuilder<CompositeBuilder<Parent>>(this, (CompositeType*)child.get());
	}

	Parent& End()
	{
		return *myParent;
	}

private:
	Parent* myParent;
	Composite* myComposite;
};

class Builder
{
public:
	template <class NodeType, typename... Arguments>
	Builder Leaf(Arguments... someArguments)
	{
		myRoot = std::make_shared<NodeType>((someArguments)...);
		return *this;
	}

	template <class CompositeType, typename... Arguments>
	CompositeBuilder<Builder> Composites(Arguments... someArguments)
	{
		myRoot = std::make_shared<CompositeType>((someArguments)...);
		return CompositeBuilder<Builder>(this, (CompositeType*)myRoot.get());
	}

	std::shared_ptr<Node> Build()
	{
		assert(myRoot != nullptr && "The Behavior Tree is empty!");
		auto tree = std::make_shared<BehaviourTree>();
		tree->SetRoot(myRoot);
		return tree;
	}

private:
	std::shared_ptr<Node> myRoot;
};

class Sequence: public Composite
{
public:
	void Init() override
	{
		myIterator = myChildren.begin();
	}
	Status Update()override
	{
		assert(!myChildren.empty() && "Composite has no children");

		while(myIterator != myChildren.end())
		{
			auto status = (*myIterator)->Tick();

			if(status != Status::Success)
			{
				return status;
			}
			myIterator++;
		}
		return Status::Success;
	}
private:
};

class Selector: public Composite
{
public:
	void Init() override
	{
		myIterator = myChildren.begin();
	}
	Status Update()override
	{
		assert(!myChildren.empty() && "Composite has no children");

		while(myIterator != myChildren.end())
		{
			auto status = (*myIterator)->Tick();

			if(status != Status::Failure)
			{
				return status;
			}
			myIterator++;
		}
		return Status::Failure;
	}
private:
};

class Leaf: public Node
{
public:
	Leaf() = default;
	virtual ~Leaf() {}
	virtual Status Update() = 0;
private:
};

