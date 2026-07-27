#ifndef Vector2D_H
#define Vector2D_H

#include <iostream>

/**
 * FVector2D
 */
namespace AE::Physics 
{

struct FVector2D 
{
public:
	FVector2D();
	FVector2D(float X, float Y);
	~FVector2D() = default;

	void Add(const FVector2D &V);           
	void Sub(const FVector2D &V);            
	void Scale(const float N);               
	[[nodiscard]] FVector2D Rotate(const float Angle) const; 

	float Magnitude() const;        
	float MagnitudeSquared() const; 

	FVector2D &Normalize();      
	FVector2D UnitVector() const; 
	FVector2D Normal() const;    

	[[nodiscard]] float DotProduct(const FVector2D &V) const;  
	[[nodiscard]] float CrossProduct(const FVector2D &V) const; 
	[[nodiscard]] float Length() const;
	[[nodiscard]] float LengthSq() const;
	[[nodiscard]] static FVector2D ZeroVector(); 

	/* Override operators function */
	FVector2D &operator=(const FVector2D &V);  
	bool operator==(const FVector2D &V) const;
	bool operator!=(const FVector2D &V) const;

	FVector2D operator+(const FVector2D &V) const;
	FVector2D operator-(const FVector2D &V) const; 
	FVector2D operator*(const float N) const;      
	FVector2D operator/(const float N) const;     
	FVector2D operator-();                        

	FVector2D &operator+=(const FVector2D &V);
	FVector2D &operator-=(const FVector2D &V); 
	FVector2D &operator*=(const float N);      
	FVector2D &operator/=(const float N);      

	static const FVector2D Zero;
	static const FVector2D UnitX;
	static const FVector2D UnitY;
	static const FVector2D NegUnitX;
	static const FVector2D NegUnitY;

	float X;
	float Y;
};

} // namespace AE::Physics

using AE::Physics::FVector2D;

#endif
