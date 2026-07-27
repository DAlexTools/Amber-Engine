#include "Constraint.h"
#include <algorithm>

namespace AE::Physics
{

namespace
{
constexpr float ConstraintEpsilon = 0.000001f;

FVector2D VelocityAtPoint(const Body& body, const FVector2D& radius)
{
    return FVector2D(
        body.velocity.X - body.angularVelocity * radius.Y,
        body.velocity.Y + body.angularVelocity * radius.X);
}

void ApplyContactImpulse(Body& a, Body& b, const FVector2D& impulse, const FVector2D& ra, const FVector2D& rb)
{
    if (a.invMass > 0.0f)
    {
        a.velocity -= impulse * a.invMass;
        a.angularVelocity -= ra.CrossProduct(impulse) * a.invI;
    }
    if (b.invMass > 0.0f)
    {
        b.velocity += impulse * b.invMass;
        b.angularVelocity += rb.CrossProduct(impulse) * b.invI;
    }
}

float ComputeEffectiveMass(
    const Body& a,
    const Body& b,
    const FVector2D& ra,
    const FVector2D& rb,
    const FVector2D& axis)
{
    const float raCrossAxis = ra.CrossProduct(axis);
    const float rbCrossAxis = rb.CrossProduct(axis);
    const float k = a.invMass + b.invMass +
        raCrossAxis * raCrossAxis * a.invI +
        rbCrossAxis * rbCrossAxis * b.invI;

    return k > ConstraintEpsilon ? 1.0f / k : 0.0f;
}
}

///////////////////////////////////////////////////////////////////////////////
// Mat6x6 with the all inverse mass and inverse I of bodies "a" and "b"
///////////////////////////////////////////////////////////////////////////////
//  [ 1/ma  0     0     0     0     0    ]
//  [ 0     1/ma  0     0     0     0    ]
//  [ 0     0     1/Ia  0     0     0    ]
//  [ 0     0     0     1/mb  0     0    ]
//  [ 0     0     0     0     1/mb  0    ]
//  [ 0     0     0     0     0     1/Ib ]
///////////////////////////////////////////////////////////////////////////////
MatMN Constraint::GetInvM() const
{
    MatMN invM(6, 6);
    invM.Zero();
    invM.rows[0][0] = a->invMass;
    invM.rows[1][1] = a->invMass;
    invM.rows[2][2] = a->invI;
    invM.rows[3][3] = b->invMass;
    invM.rows[4][4] = b->invMass;
    invM.rows[5][5] = b->invI;
    
    return invM;
}



///////////////////////////////////////////////////////////////////////////////
// VectorN with the all linear and angular velocities of bodies "a" and "b"
///////////////////////////////////////////////////////////////////////////////
//  [ va.x ]
//  [ va.y ]
//  [ ωa   ]
//  [ vb.x ]
//  [ vb.y ]
//  [ ωb   ]
///////////////////////////////////////////////////////////////////////////////
VectorN Constraint::GetVelocities() const
{
    VectorN V(6);
    V.Zero();
    V[0] = a->velocity.X;
    V[1] = a->velocity.Y;
    V[2] = a->angularVelocity;
    V[3] = b->velocity.X;
    V[4] = b->velocity.Y;
    V[5] = b->angularVelocity;
    return V;
}

/**
 * 
 */
JointConstraint::JointConstraint()
: Constraint(), jacobian(1, 6), cachedLambda(1), bias(0.0f)
{
    cachedLambda.Zero();
}

/**
 * 
 */
JointConstraint::JointConstraint(Body* a, Body* b, const FVector2D& anchorPoint)
: Constraint(), jacobian(1, 6), cachedLambda(1), bias(0.0f)
{
    this->a = a;
    this->b = b;
    this->aPoint = a->WorldSpaceToLocalSpace(anchorPoint);
    this->bPoint = b->WorldSpaceToLocalSpace(anchorPoint);
    cachedLambda.Zero();
}

/**
 * 
 */
void JointConstraint::PreSolve(const float dt)
{
    // Get the anchor point position in world space
    const FVector2D pa = a->LocalSpaceToWorldSpace(aPoint);
    const FVector2D pb = b->LocalSpaceToWorldSpace(bPoint);

    const FVector2D ra = pa - a->position;
    const FVector2D rb = pb - b->position;

    jacobian.Zero();

    FVector2D J1 = (pa - pb) * 2.0;
    jacobian.rows[0][0] = J1.X;  // A linear velocity.x
    jacobian.rows[0][1] = J1.Y;  // A linear velocity.y

    float J2 = ra.CrossProduct(pa - pb) * 2.0;
    jacobian.rows[0][2] = J2;  // A angular velocity

    FVector2D J3 = (pb - pa) * 2.0;
    jacobian.rows[0][3] = J3.X;  // B linear velocity.x
    jacobian.rows[0][4] = J3.Y;  // B linear velocity.y

    float J4 = rb.CrossProduct(pb - pa) * 2.0;
    jacobian.rows[0][5] = J4;  // B angular velocity

    // Warm starting (apply cached lambda)
    const MatMN Jt = jacobian.Transpose();
    VectorN impulses = Jt * cachedLambda;

    // Apply the impulses to both bodies
    a->ApplyImpulseLinear(FVector2D(impulses[0], impulses[1]));      // A linear impulse
    a->ApplyImpulseAngular(impulses[2]);                            // A angular impulse
    b->ApplyImpulseLinear(FVector2D(impulses[3], impulses[4]));      // B linear impulse
    b->ApplyImpulseAngular(impulses[5]);                            // B angular impulse

    // Compute the bias term (baumgarte stabilization)
    const float beta = 0.02f;
    float C = (pb - pa).DotProduct(pb - pa);
    C = std::max(0.0f, C - 0.01f);
    bias = (beta / dt) * C;
}

/**
 * 
 */
void JointConstraint::Solve()
{
    const VectorN V = GetVelocities();
    const MatMN invM = GetInvM();

    const MatMN J = jacobian;
    const MatMN Jt = jacobian.Transpose();

    // Compute lambda using Ax=b (Gauss-Seidel method)
    const MatMN lhs = J * invM * Jt;  // A
    VectorN rhs = J * V * -1.0f;   // b
    rhs[0] -= bias;
    const VectorN lambda = MatMN::SolveGaussSeidel(lhs, rhs);
    cachedLambda += lambda;

    // Compute the impulses with both direction and magnitude
    const VectorN impulses = Jt * lambda;

    // Apply the impulses to both bodies
    a->ApplyImpulseLinear(FVector2D(impulses[0], impulses[1]));  // A linear impulse
    a->ApplyImpulseAngular(impulses[2]);                    // A angular impulse
    b->ApplyImpulseLinear(FVector2D(impulses[3], impulses[4]));  // B linear impulse
    b->ApplyImpulseAngular(impulses[5]);                    // B angular impulse
}

/**
 * 
 */
void JointConstraint::PostSolve()
{
    // Limit the warm starting to reasonable limits
    cachedLambda[0] = std::clamp(cachedLambda[0], -10000.0f, 10000.0f);
}

/**
 * 
 */
PenetrationConstraint::PenetrationConstraint() :
    Constraint(),
    normal(FVector2D::Zero),
    tangent(FVector2D::Zero),
    ra(FVector2D::Zero),
    rb(FVector2D::Zero),
    cachedNormalLambda(0.0f),
    cachedTangentLambda(0.0f),
    bias(0.0f),
    normalMass(0.0f),
    tangentMass(0.0f),
    friction(0.0f)
{
}

/**
 * 
 */
PenetrationConstraint::PenetrationConstraint(Body* a, Body* b, const FVector2D& aCollisionPoint, const FVector2D& bCollisionPoint, const FVector2D& normal)
    : Constraint(),
      normal(normal),
      tangent(FVector2D::Zero),
      ra(FVector2D::Zero),
      rb(FVector2D::Zero),
      cachedNormalLambda(0.0f),
      cachedTangentLambda(0.0f),
      bias(0.0f),
      normalMass(0.0f),
      tangentMass(0.0f),
      friction(0.0f)
{
    this->a = a;
    this->b = b;
    this->aPoint = a->WorldSpaceToLocalSpace(aCollisionPoint);
    this->bPoint = b->WorldSpaceToLocalSpace(bCollisionPoint);
}

/**
 * 
 */
void PenetrationConstraint::PreSolve(const float dt)
{
    // Get the collision points and normal in world space
    const FVector2D pa = a->LocalSpaceToWorldSpace(aPoint);
    const FVector2D pb = b->LocalSpaceToWorldSpace(bPoint);
    normal.Normalize();
    tangent = normal.Normal();
    ra = pa - a->position;
    rb = pb - b->position;

    friction = std::max(a->friction, b->friction);
    normalMass = ComputeEffectiveMass(*a, *b, ra, rb, normal);
    tangentMass = friction > 0.0f ? ComputeEffectiveMass(*a, *b, ra, rb, tangent) : 0.0f;

    const FVector2D warmImpulse = normal * cachedNormalLambda + tangent * cachedTangentLambda;
    ApplyContactImpulse(*a, *b, warmImpulse, ra, rb);

    // Compute the bias term (baumgarte stabilization)
    const float beta = 0.2f;
    float C = (pb - pa).DotProduct(-normal);
    C = std::min(0.0f, C + 0.01f);
    bias = (beta / dt) * C;
}

/**
 * 
 */
void PenetrationConstraint::Solve()
{
    FVector2D relativeVelocity = VelocityAtPoint(*b, rb) - VelocityAtPoint(*a, ra);

    float lambdaNormal = -(relativeVelocity.DotProduct(normal) + bias) * normalMass;
    const float oldNormalLambda = cachedNormalLambda;
    cachedNormalLambda = std::max(0.0f, cachedNormalLambda + lambdaNormal);
    lambdaNormal = cachedNormalLambda - oldNormalLambda;

    ApplyContactImpulse(*a, *b, normal * lambdaNormal, ra, rb);

    if (friction <= 0.0f || tangentMass <= 0.0f)
    {
        return;
    }

    relativeVelocity = VelocityAtPoint(*b, rb) - VelocityAtPoint(*a, ra);

    float lambdaTangent = -relativeVelocity.DotProduct(tangent) * tangentMass;
    const float maxFriction = cachedNormalLambda * friction;
    const float oldTangentLambda = cachedTangentLambda;
    cachedTangentLambda = std::clamp(
        cachedTangentLambda + lambdaTangent,
        -maxFriction,
        maxFriction);
    lambdaTangent = cachedTangentLambda - oldTangentLambda;

    ApplyContactImpulse(*a, *b, tangent * lambdaTangent, ra, rb);
}

void PenetrationConstraint::PostSolve()
{
    // TODO: Maybe we should clamp the values of cached lambda to reasonable limits
}

}
