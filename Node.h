#pragma once
#include <memory>

class Node
{
public:
    enum class Status
    {
        Invalid,
        Success,
        Failure,
        Running,
    };
    virtual ~Node() = default;
    virtual Status Update() = 0;
    virtual void Init() {}
    virtual void Terminate(Status aStatus) { aStatus; }

    bool CheckStatus(Status aStatus) { return myStatus == aStatus; }

    void reset() { myStatus = Status::Invalid; }

    Status Tick()
    {
        if(myStatus != Status::Running)
        {
            Init();
        }

        myStatus = Update();

        if(myStatus != Status::Running)
        {
            Terminate(myStatus);
        }

        return myStatus;
    }

    std::shared_ptr<Node> myChild;

protected:
    Status myStatus = Status::Invalid;
};

