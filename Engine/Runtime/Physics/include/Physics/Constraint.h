#ifndef CONSTRAINT_H
#define CONSTRAINT_H

#include "Objects/Body.h"
#include "Core/Math/MatMN.h"

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

    Vector2D aPoint;  // The constraint point in A's local space
    Vector2D bPoint;  // The constraint point in B's local space

    virtual ~Constraint() = default;

    MatMN GetInvM() const;
    VectorN GetVelocities() const;

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
    MatMN       jacobian;
    VectorN     cachedLambda;
    float       bias;

public:
    JointConstraint();
    JointConstraint(Body* a, Body* b, const Vector2D& anchorPoint);
    void PreSolve(const float dt) override;
    void Solve() override;
    void PostSolve() override;
};

class PenetrationConstraint : public Constraint
{
private:
    Vector2D    normal;            // Collision normal in world space, pointing from A to B
    Vector2D    tangent;
    Vector2D    ra;
    Vector2D    rb;
    float       cachedNormalLambda;
    float       cachedTangentLambda;
    float       bias;
    float       normalMass;
    float       tangentMass;
    float       friction;          // Friction coefficient between the two penetrating bodies

public:
    PenetrationConstraint();
    PenetrationConstraint(Body* a, Body* b, const Vector2D& aCollisionPoint, const Vector2D& bCollisionPoint, const Vector2D& normal);
    void PreSolve(const float dt) override;
    void Solve() override;
    void PostSolve() override;
};

}

using AE::Physics::Constraint;
using AE::Physics::JointConstraint;
using AE::Physics::PenetrationConstraint;

#endif
