
#include "Physics/Particles/Cloth.h"
#include "Physics/Core/Constants.h"

namespace AE::Physics
{

Cloth::Cloth(int width, int height, int spacing, int startX, int startY)
{
	for (int y = AE::Physics::ZERO; y <= height; y++)
	{
		for (int x = AE::Physics::ZERO; x <= width; x++)
		{
			Point* point = new Point(startX + x * spacing, startY + y * spacing);

			if (x != AE::Physics::ZERO)
			{
				Point* leftPoint = points[this->points.size() - AE::Physics::POSITIVE];
				Stick* s = new Stick(*point, *leftPoint, spacing);
				leftPoint->AddStick(s, AE::Physics::ZERO);
				point->AddStick(s, AE::Physics::ZERO);
				sticks.push_back(s);
			}

			if (y != AE::Physics::ZERO)
			{
				Point* upPoint = points[x + (y - AE::Physics::POSITIVE) * (width + AE::Physics::POSITIVE)];
				Stick* s = new Stick(*point, *upPoint, spacing);
				upPoint->AddStick(s, AE::Physics::POSITIVE);
				point->AddStick(s, AE::Physics::POSITIVE);
				sticks.push_back(s);
			}

			if (y == AE::Physics::ZERO && x % 2 == AE::Physics::ZERO)
			{
				point->Pin();
			}

			points.push_back(point);
		}
	}
}

Cloth::~Cloth()
{
	for (auto point : points)
		delete point;
	for (auto stick : sticks)
		delete stick;
}

void Cloth::Update(Mouse* mouse, float deltaTime, int windowWidth, int windowHeight)
{
	for (int i = AE::Physics::ZERO; i < static_cast<int>(points.size()); i++)
	{
		Point* p = points[i];
		p->Update(deltaTime, drag, gravity, elasticity, mouse, windowWidth, windowHeight);
	};

	for (int i = AE::Physics::ZERO; i < static_cast<int>(sticks.size()); i++)
	{
		sticks[i]->Update();
	};
}

} // namespace AE::Physics
