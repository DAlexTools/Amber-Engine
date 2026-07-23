
#include "CollisionDetection.h"
#include <limits>

namespace AE::Physics
{


/**
 * 
 */
bool CollisionDetection::IsColliding(Body* a, Body* b, std::vector<Contact>& contacts)
{
    const bool aIsCircle = a->shape->GetType() == CIRCLE;
    const bool bIsCircle = b->shape->GetType() == CIRCLE;
    const bool aIsPolygon = a->shape->GetType() == POLYGON || a->shape->GetType() == BOX;
    const bool bIsPolygon = b->shape->GetType() == POLYGON || b->shape->GetType() == BOX;

    if (aIsCircle && bIsCircle)
    {
        return IsCollidingCircleCircle(a, b, contacts);
    }

    if (aIsPolygon && bIsPolygon)
    {
        return IsCollidingPolygonPolygon(a, b, contacts);
    }

    if (aIsPolygon && bIsCircle)
    {
        return IsCollidingPolygonCircle(a, b, contacts);
    }

    if (aIsCircle && bIsPolygon)
    {
        return IsCollidingPolygonCircle(b, a, contacts);
    }

    return false;
}

/**
 * 
 */
bool CollisionDetection::IsCollidingCircleCircle(Body* a, Body* b, std::vector<Contact>& contacts)
{
    CircleShape* aCircleShape = (CircleShape*)a->shape;
    CircleShape* bCircleShape = (CircleShape*)b->shape;

    const Vector2D ab = b->position - a->position;
    const float radiusSum = aCircleShape->radius + bCircleShape->radius;

    if (!(ab.MagnitudeSquared() <= (radiusSum * radiusSum))) return false;
    
    Contact contact;
    contact.a = a;
    contact.b = b;

    contact.normal = ab;
    contact.normal.Normalize();

    contact.start = b->position - contact.normal * bCircleShape->radius;
    contact.end = a->position + contact.normal * aCircleShape->radius;

    contact.depth = (contact.end - contact.start).Magnitude();

    contacts.push_back(contact);

    return true;
}

}

/**
 *	check bool polygon on polygon body 
 *	@return is colliding polygon
 *	@args body a - pointer of body a
 *	@args body b - pointer of body b
 *	@args vector Contact object body  
 */
bool CollisionDetection::IsCollidingPolygonPolygon(Body* a, Body* b, std::vector<Contact>& contacts)
{
    PolygonShape* aPolygonShape = (PolygonShape*)a->shape;
    PolygonShape* bPolygonShape = (PolygonShape*)b->shape;
    
    int aIndexReferenceEdge, bIndexReferenceEdge;
    Vector2D aSupportPoint, bSupportPoint;
    const float abSeparation = aPolygonShape->FindMinSeparation(bPolygonShape, aIndexReferenceEdge, aSupportPoint);
    
    if (abSeparation >= 0) return false;

    const float baSeparation = bPolygonShape->FindMinSeparation(aPolygonShape, bIndexReferenceEdge, bSupportPoint);
    
    if (baSeparation >= 0) return false;
    
    PolygonShape* referenceShape;
    PolygonShape* incidentShape;
    
    int indexReferenceEdge;
    
    if (abSeparation > baSeparation)
    {
        referenceShape = aPolygonShape;
        incidentShape = bPolygonShape;
        indexReferenceEdge = aIndexReferenceEdge;
    }
    else
    {
        referenceShape = bPolygonShape;
        incidentShape = aPolygonShape;
        indexReferenceEdge = bIndexReferenceEdge;
    }

    // Find the reference edge based on the index that returned from the function
    const Vector2D referenceEdge = referenceShape->EdgeAt(indexReferenceEdge);

    /////////////////////////////////////
    // Clipping
    /////////////////////////////////////
    const int incidentIndex = incidentShape->FindIncidentEdge(referenceEdge.Normal());
    const int incidentNextIndex = (incidentIndex + 1) % incidentShape->worldVertices.size();
    
    const Vector2D v0 = incidentShape->worldVertices[incidentIndex];
    const Vector2D v1 = incidentShape->worldVertices[incidentNextIndex];

    std::vector<Vector2D> contactPoints = { v0, v1 };
    std::vector<Vector2D> clippedPoints = contactPoints;
    
    for (int i = 0; i < static_cast<int>(referenceShape->worldVertices.size()); i++)
    {
        if (i == indexReferenceEdge) continue;

        const Vector2D c0 = referenceShape->worldVertices[i];
        const Vector2D c1 = referenceShape->worldVertices[(i + 1) % referenceShape->worldVertices.size()];
        
        const int numClipped = referenceShape->ClipSegmentToLine(contactPoints, clippedPoints, c0, c1);
        
        if (numClipped < 2)
        {
            break;
        }

        contactPoints = clippedPoints;  // make the next contact points the ones that were just clipped
    }

    const auto vref = referenceShape->worldVertices[indexReferenceEdge];

    // Loop all clipped points, but only consider those where separation is negative (objects are penetrating each other)
    for (auto& vclip : clippedPoints)
    {
        const float separation = (vclip - vref).DotProduct(referenceEdge.Normal());
        
        if (separation <= 0)
        {
            Contact contact;
            contact.a = a;
            contact.b = b;
            contact.normal = referenceEdge.Normal();
            contact.start = vclip;
            contact.end = vclip + contact.normal * -separation;
            
	        if (baSeparation >= abSeparation)
            {
                std::swap(contact.start, contact.end);  // the start-end points are always from "a" to "b"
                contact.normal *= -1.0;                 // the collision normal is always from "a" to "b"
            }

            contacts.push_back(contact);
        }
    }

    return true;
}

/**
 *	check collision colliding is circle on polygon 
 *	@return bool is colliding shape polygon 
 *	@arg body ptr - pointer of body polygon 
 *	@arg body ptr - pointer circle body polygon 
 *	@arg vector Contact object  
 */
bool CollisionDetection::IsCollidingPolygonCircle(Body* polygon, Body* circle, std::vector<Contact>& contacts)
{
    const PolygonShape* polygonShape = (PolygonShape*)polygon->shape;
    const CircleShape* circleShape = (CircleShape*)circle->shape;
    const std::vector<Vector2D>& polygonVertices = polygonShape->worldVertices;

    bool isOutside = false;
    Vector2D minCurrVertex;
    Vector2D minNextVertex;
    float distanceCircleEdge = std::numeric_limits<float>::lowest();

    // Loop all the edges of the polygon/box finding the nearest edge to the circle center
    for (int i = 0; i < static_cast<int>(polygonVertices.size()); i++)
    {
        int currVertex = i;
        int nextVertex = (i + 1) % polygonVertices.size();
        Vector2D edge = polygonShape->EdgeAt(currVertex);
        Vector2D normal = edge.Normal();

        // Compare the circle center with the rectangle vertex
        const Vector2D vertexToCircleCenter = circle->position - polygonVertices[currVertex];
        const float projection = vertexToCircleCenter.DotProduct(normal);

        // If we found a dot product projection that is in the positive/outside side of the normal
        if (projection > 0)
        {
            // Circle center is outside the polygon
            distanceCircleEdge = projection;
            minCurrVertex = polygonVertices[currVertex];
            minNextVertex = polygonVertices[nextVertex];
            isOutside = true;
            break;
        }
        else
        {
            // Circle center is inside the rectangle, find the min edge (the one with the least negative projection)
            if (projection > distanceCircleEdge)
            {
                distanceCircleEdge = projection;
                minCurrVertex = polygonVertices[currVertex];
                minNextVertex = polygonVertices[nextVertex];
            }
        }
    }

    Contact contact;

    if (isOutside)
    {
        ///////////////////////////////////////
        // Check if we are inside region A:
        ///////////////////////////////////////
        Vector2D v1 = circle->position - minCurrVertex;  // vector from the nearest vertex to the circle center
        Vector2D v2 = minNextVertex - minCurrVertex;     // the nearest edge (from curr vertex to next vertex)
        if (v1.DotProduct(v2) < 0)
        {
            // Distance from vertex to circle center is greater than radius... no collision
            if (v1.Magnitude() > circleShape->radius)
            {
                return false;
            }
            else
            {
                // Detected collision in region A:
                contact.a = polygon;
                contact.b = circle;
                contact.depth = circleShape->radius - v1.Magnitude();
                contact.normal = v1.Normalize();
                contact.start = circle->position + (contact.normal * -circleShape->radius);
                contact.end = contact.start + (contact.normal * contact.depth);
            }
        }
        else
        {
            ///////////////////////////////////////
            // Check if we are inside region B:
            ///////////////////////////////////////
            v1 = circle->position - minNextVertex;  // vector from the next nearest vertex to the circle center
            v2 = minCurrVertex - minNextVertex;     // the nearest edge
            if (v1.DotProduct(v2) < 0)
            {
                // Distance from vertex to circle center is greater than radius... no collision
                if (v1.Magnitude() > circleShape->radius)
                {
                    return false;
                }
                else
                {
                    // Detected collision in region B:
                    contact.a = polygon;
                    contact.b = circle;
                    contact.depth = circleShape->radius - v1.Magnitude();
                    contact.normal = v1.Normalize();
                    contact.start = circle->position + (contact.normal * -circleShape->radius);
                    contact.end = contact.start + (contact.normal * contact.depth);
                }
            }
            else
            {
                ///////////////////////////////////////
                // We are inside region C:
                ///////////////////////////////////////
                if (distanceCircleEdge > circleShape->radius)
                {
                    // No collision... Distance between the closest distance and the circle center is greater than the radius.
                    return false;
                }
                else
                {
                    // Detected collision in region C:
                    contact.a = polygon;
                    contact.b = circle;
                    contact.depth = circleShape->radius - distanceCircleEdge;
                    contact.normal = (minNextVertex - minCurrVertex).Normal();
                    contact.start = circle->position - (contact.normal * circleShape->radius);
                    contact.end = contact.start + (contact.normal * contact.depth);
                }
            }
        }
    }
    else
    {
        // The center of circle is inside the polygon... it is definitely colliding!
        contact.a = polygon;
        contact.b = circle;
        contact.depth = circleShape->radius - distanceCircleEdge;
        contact.normal = (minNextVertex - minCurrVertex).Normal();
        contact.start = circle->position - (contact.normal * circleShape->radius);
        contact.end = contact.start + (contact.normal * contact.depth);
    }

    contacts.push_back(contact);

    return true;
}
