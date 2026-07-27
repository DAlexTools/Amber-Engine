#ifndef SHAPE_H
#define SHAPE_H

#include "Core/Math/Vector2D.h"
#include <vector>

namespace AE::Physics
{

using TVector2D = std::vector<FVector2D>;

enum ShapeType
{
    CIRCLE,
    POLYGON,
    BOX
};

struct Shape
{
    virtual             ~Shape() = default;
    virtual ShapeType   GetType() const = 0;
    virtual Shape*      Clone() const = 0;
    virtual void        UpdateVertices(float angle, const FVector2D& position) = 0;
    virtual float       GetMomentOfInertia() const = 0;
};

struct CircleShape : public Shape
{
    CircleShape(const float radius);
    virtual ~CircleShape();

    virtual ShapeType   GetType() const override;
    virtual Shape*      Clone() const override;
    virtual void        UpdateVertices(float angle, const FVector2D& position) override;
    virtual float       GetMomentOfInertia() const override;

    float               radius;
};

struct PolygonShape : public Shape
{
    PolygonShape() = default;
    PolygonShape(const TVector2D vertices);
    virtual ~PolygonShape();

    virtual ShapeType   GetType() const override;
    virtual Shape*      Clone() const override;
    virtual void        UpdateVertices(float angle, const FVector2D& position) override;
    virtual float       GetMomentOfInertia() const override;
    FVector2D            EdgeAt(int index) const;
    float               FindMinSeparation(const PolygonShape* other, int& indexReferenceEdge, FVector2D& supportPoint) const;
    int                 FindIncidentEdge(const FVector2D& normal) const;
    int                 ClipSegmentToLine(const TVector2D& contactsIn, TVector2D& contactsOut, const FVector2D& c0, const FVector2D& c1) const;
    float               PolygonArea() const;
    FVector2D            PolygonCentroid() const;

    float               height;
    float               width;
    TVector2D           localVertices;
    TVector2D           worldVertices;
};

struct BoxShape : public PolygonShape
{
    BoxShape(float width, float height);
    virtual ~BoxShape();

    virtual ShapeType   GetType() const override;
    virtual Shape*      Clone() const override;
    virtual float       GetMomentOfInertia() const override;
};

}

using AE::Physics::ShapeType;
using AE::Physics::Shape;
using AE::Physics::CircleShape;
using AE::Physics::PolygonShape;
using AE::Physics::BoxShape;
using AE::Physics::TVector2D;

static constexpr ShapeType CIRCLE = AE::Physics::CIRCLE;
static constexpr ShapeType POLYGON = AE::Physics::POLYGON;
static constexpr ShapeType BOX = AE::Physics::BOX;

#endif
