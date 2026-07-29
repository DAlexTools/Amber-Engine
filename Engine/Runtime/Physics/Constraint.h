#ifndef CONSTRAINT_H
#define CONSTRAINT_H

#include "Body.h"
#include "Core/Math/MatrixMN.h"

namespace AE::Physics
{

/**
 * 
 */
class Constraint
{
public:
    Body* a;
    Body* b;

    FVector2D aPoint;  // The constraint point in A's local space
    FVector2D bPoint;  // The constraint point in B's local space

    virtual ~Constraint() = default;

    AE::Math::MatrixMN GetInvM() const;
    AE::Math::FVectorN GetVelocities() const;

    virtual void PreSolve(const float dt) {}
    virtual void Solve() {}
    virtual void PostSolve() {}
};

/**
 * 
 */
class JointConstraint : public Constraint
{
private:
    AE::Math::MatrixMN jacobian;
    AE::Math::FVectorN cachedLambda;
    float       bias;

public:
    JointConstraint();
    JointConstraint(Body* a, Body* b, const FVector2D& anchorPoint);
    void PreSolve(const float dt) override;
    void Solve() override;
    void PostSolve() override;
};

class PenetrationConstraint : public Constraint
{
private:
    FVector2D    normal;            // Collision normal in world space, pointing from A to B
    FVector2D    tangent;
    FVector2D    ra;
    FVector2D    rb;
    float       cachedNormalLambda;
    float       cachedTangentLambda;
    float       bias;
    float       normalMass;
    float       tangentMass;
    float       friction;          // Friction coefficient between the two penetrating bodies

public:
    PenetrationConstraint();
    PenetrationConstraint(Body* a, Body* b, const FVector2D& aCollisionPoint, const FVector2D& bCollisionPoint, const FVector2D& normal);
    void PreSolve(const float dt) override;
    void Solve() override;
    void PostSolve() override;
};

}

using AE::Physics::Constraint;
using AE::Physics::JointConstraint;
using AE::Physics::PenetrationConstraint;

#endif
