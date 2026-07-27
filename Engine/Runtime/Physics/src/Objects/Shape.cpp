#include "Objects/Shape.h"
#include <iostream>
#include <limits>
#include "Constants.h"
#include "Core/Math/PhysicsMath.h"
#include "Logging/Logger.h"

namespace AE::Physics
{

namespace ShapeConstants
{
constexpr double MomentInertiaDot = 0.5;
constexpr double TwoDoubleValue = 2.0;
constexpr double MultipliedRigidbody = 0.083333;
}

CircleShape::CircleShape(float radius)
{
    this->radius = radius;
    AE::Logger::Log("CircleShape constructor called (radius " +  std::to_string(radius) + " ).", "Physics");
}

CircleShape::~CircleShape()
{
    AE::Logger::Log("CircleShape destructor called", "Physics");
}

Shape* CircleShape::Clone() const
{
    return new CircleShape(radius);
}

void CircleShape::UpdateVertices(float angle, const FVector2D& position)
{
    return;  // Circles don't have vertices... nothing to do here
}

ShapeType CircleShape::GetType() const
{
    return CIRCLE;
}

float CircleShape::GetMomentOfInertia() const
{
    // For solid circles, the moment of inertia is 1/2 * r^2
    // But this still needs to be multiplied by the rigidbody's mass
    return ShapeConstants::MomentInertiaDot * (radius * radius);
}

PolygonShape::PolygonShape(const TVector2D vertices)
{
    float minX = std::numeric_limits<float>::max();
    float minY = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float maxY = std::numeric_limits<float>::lowest();

    // Initialize the vertices of the polygon shape and set width and height
    for (auto vertex : vertices)
    {
        localVertices.push_back(vertex);
        worldVertices.push_back(vertex);

        // Find min and max X and Y to calculate polygon width and height
        minX = Math::Min(minX, vertex.X);
        maxX = Math::Max(maxX, vertex.X);
        minY = Math::Min(minY, vertex.Y);
        maxY = Math::Max(maxY, vertex.Y);
    }
    width = maxX - minX;
    height = maxY - minY;

    AE::Logger::Log("PolygonShape constructor called", "Physics");
}

PolygonShape::~PolygonShape()
{
    AE::Logger::Log("PolygonShape destructor called", "Physics");
}

ShapeType PolygonShape::GetType() const
{
    return POLYGON;
}

Shape* PolygonShape::Clone() const
{
    return new PolygonShape(localVertices);
}

float PolygonShape::PolygonArea() const
{
    float area = AE::Physics::ZERO_FLOAT;

    for (int i = AE::Physics::ZERO; i < static_cast<int>(localVertices.size()); i++)
    {
        int j = (i + AE::Physics::POSITIVE) % localVertices.size();
        area += localVertices[i].CrossProduct(localVertices[j]);
    }
    return area / ShapeConstants::TwoDoubleValue;
}

FVector2D PolygonShape::PolygonCentroid() const
{
    FVector2D cg = FVector2D::Zero;

    for (int i = AE::Physics::ZERO; i < static_cast<int>(localVertices.size()); i++)
    {
        int j = (i + AE::Physics::POSITIVE) % localVertices.size();
        cg += (localVertices[i] + localVertices[j]) * localVertices[i].CrossProduct(localVertices[j]);
    }
    return cg / 6 / PolygonArea();
}

float PolygonShape::GetMomentOfInertia() const
{
    float acc0 = AE::Physics::ZERO;
    float acc1 = AE::Physics::ZERO;

    for (unsigned int i = AE::Physics::ZERO; i < static_cast<unsigned int>(localVertices.size()); i++)
    {
        const auto a = localVertices[i];
        const auto b = localVertices[(i + AE::Physics::POSITIVE) % localVertices.size()];
        const auto cross = abs(a.CrossProduct(b));
        acc0 += cross * (a.DotProduct(a) + b.DotProduct(b) + a.DotProduct(b));
        acc1 += cross;
    }

    return acc0 / 6 / acc1;
}

FVector2D PolygonShape::EdgeAt(int index) const
{
    const int currVertex = index;
    const int nextVertex = (index + AE::Physics::POSITIVE) % worldVertices.size();

    return worldVertices[nextVertex] - worldVertices[currVertex];
}

float PolygonShape::FindMinSeparation(const PolygonShape* other, int& indexReferenceEdge, FVector2D& supportPoint) const
{
    float separation = std::numeric_limits<float>::lowest();
    
    // Loop all the vertices of "this" polygon
    for (int i = AE::Physics::ZERO; i < static_cast<int>(this->worldVertices.size()); i++)
    {
        const FVector2D va = this->worldVertices[i];
        const FVector2D normal = this->EdgeAt(i).Normal();
        
        // Loop all the vertices of the "other" polygon
        float minSep = std::numeric_limits<float>::max();
        FVector2D minVertex;
        
        for (int j = 0; j < static_cast<int>(other->worldVertices.size()); j++)
        {
            FVector2D vb = other->worldVertices[j];
            const float proj = (vb - va).DotProduct(normal);
            
            if (proj < minSep)
            {
                minSep = proj;
                minVertex = vb;
            }
        }

        if (minSep > separation)
        {
            separation = minSep;
            indexReferenceEdge = i;
            supportPoint = minVertex;
        }
    }
    return separation;
}

int PolygonShape::FindIncidentEdge(const FVector2D& normal) const
{
    int indexIncidentEdge = AE::Physics::ZERO;

    float minProj = std::numeric_limits<float>::max();
    
    for (int i = AE::Physics::ZERO; i < static_cast<int>(this->worldVertices.size()); ++i)
    {
        const auto edgeNormal = this->EdgeAt(i).Normal();
        const auto proj = edgeNormal.DotProduct(normal);
        
        if (proj < minProj)
        {
            minProj = proj;
            indexIncidentEdge = i;
        }
    }
    return indexIncidentEdge;
}

int PolygonShape::ClipSegmentToLine(const TVector2D& contactsIn, TVector2D& contactsOut, const FVector2D& c0, const FVector2D& c1) const
{
    // Start with no output points
    int numOut = AE::Physics::ZERO;

    // Calculate the distance of end points to the line
    FVector2D normal = (c1 - c0).Normalize();
    const float dist0 = (contactsIn[0] - c0).CrossProduct(normal);
    const float dist1 = (contactsIn[1] - c0).CrossProduct(normal);

    // If the points are behind the plane
    if (dist0 <= AE::Physics::ZERO) contactsOut[numOut++] = contactsIn[0];
    if (dist1 <= AE::Physics::ZERO) contactsOut[numOut++] = contactsIn[1];

    // If the points are on different sides of the plane (one distance is negative and the other is positive)
    if (dist0 * dist1 < AE::Physics::ZERO)
    {
        const float totalDist = dist0 - dist1;

        // Fint the intersection using linear interpolation: lerp(start,end) => start + t*(end-start)
        const float t = dist0 / (totalDist);
        const FVector2D contact = contactsIn[0] + (contactsIn[1] - contactsIn[0]) * t;
        contactsOut[numOut] = contact;
        numOut++;
    }
    return numOut;
}

void PolygonShape::UpdateVertices(float angle, const FVector2D& position)
{
    // Loop all the vertices, transforming from local to world space
    for (int i = 0; i < static_cast<int>(localVertices.size()); i++)
    {
        // First rotate, then we translate
        worldVertices[i] = localVertices[i].Rotate(angle);
        worldVertices[i] += position;
    }
}

BoxShape::BoxShape(float width, float height)
{
    this->width = width;
    this->height = height;

    // Load the vertices of the box polygon
    localVertices.push_back(FVector2D(-width / ShapeConstants::TwoDoubleValue, -height / ShapeConstants::TwoDoubleValue));
    localVertices.push_back(FVector2D(+width / ShapeConstants::TwoDoubleValue, -height / ShapeConstants::TwoDoubleValue));
    localVertices.push_back(FVector2D(+width / ShapeConstants::TwoDoubleValue, +height / ShapeConstants::TwoDoubleValue));
    localVertices.push_back(FVector2D(-width / ShapeConstants::TwoDoubleValue, +height / ShapeConstants::TwoDoubleValue));

    worldVertices.push_back(FVector2D(-width / ShapeConstants::TwoDoubleValue, -height / ShapeConstants::TwoDoubleValue));
    worldVertices.push_back(FVector2D(+width / ShapeConstants::TwoDoubleValue, -height / ShapeConstants::TwoDoubleValue));
    worldVertices.push_back(FVector2D(+width / ShapeConstants::TwoDoubleValue, +height / ShapeConstants::TwoDoubleValue));
    worldVertices.push_back(FVector2D(-width / ShapeConstants::TwoDoubleValue, +height / ShapeConstants::TwoDoubleValue));
}

BoxShape::~BoxShape()
{
    // TODO: ...
}

ShapeType BoxShape::GetType() const
{
    return BOX;
}

Shape* BoxShape::Clone() const
{
    return new BoxShape(width, height);
}

float BoxShape::GetMomentOfInertia() const
{
    // For a rectangle, the moment of inertia is 1/12 * (w^2 + h^2)
    // But this still needs to be multiplied by the rigidbody's mass
    return (ShapeConstants::MultipliedRigidbody) * (width * width + height * height);
}

}
