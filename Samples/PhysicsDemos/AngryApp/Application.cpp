#include "Application.h"
#include "Physics/Constants.h"
#include "Physics/Force.h"
#include "Physics/CollisionDetection.h"
#include "Physics/Contact.h"
#include "Physics/Constraint.h"

#include "../Renderer/SDL/Graphics.h"
#include "../Common/SamplePaths.h"

#define DEBUG_IFNO 0

/**
 * Running bool
 */
bool Application::IsRunning() const
{
	return running;
}

/**
 *Setup function (executed once in the beginning of the simulation)
 */
void Application::Setup()
{
	running = Graphics::OpenWindow();

	// Create a physics world with gravity of -9.8 m/s2
	world = new World(-9.8);

	// Load texture for the background image
	const std::string backgroundPath = ResolvePhysicsDemoAssetPath("../Content/angrybirds/background.png");
	SDL_Surface* bgSurface = IMG_Load(backgroundPath.c_str());
	if (bgSurface)
	{
		bgTexture = SDL_CreateTextureFromSurface(Graphics::Renderer, bgSurface);
		SDL_FreeSurface(bgSurface);
	}

	// Add bird
	Body* bird = new Body(CircleShape(45), 100, Graphics::Height() / 2.0 + 220, 3.0);
	bodyTextures.Set(bird, "../Content/angrybirds/bird-red.png");
	world->AddBody(bird);

	// Add a floor and walls to contain objects objects
	Body* floor = new Body(BoxShape(Graphics::Width() - 50, 50), Graphics::Width() / 2.0, Graphics::Height() / 2.0 + 340, 0.0);
	Body* leftFence = new Body(BoxShape(50, Graphics::Height() - 200), 0, Graphics::Height() / 2.0 - 35, 0.0);
	Body* rightFence = new Body(BoxShape(50, Graphics::Height() - 200), Graphics::Width(), Graphics::Height() / 2.0 - 35, 0.0);
	Body* top = new Body(BoxShape(Graphics::Width() + 50, 50), Graphics::Width() / 2.0, Graphics::Height() / 2.0 - 500, 0.0);

	world->AddBody(floor);
	world->AddBody(leftFence);
	world->AddBody(rightFence);
	world->AddBody(top);

	// Add a stack of boxes
	for (int i = 1; i <= 4; i++)
	{
		const float mass = 10.0 / static_cast<float>(i);
		Body* box = new Body(BoxShape(50, 50), 600, floor->position.Y - i * 55, mass);
		bodyTextures.Set(box, "../Content/angrybirds/wood-box.png");
		box->friction = 0.9;
		box->restitution = 0.1;
		world->AddBody(box);
	}

	// Add structure with blocks
	Body* plank1 = new Body(BoxShape(50, 150), Graphics::Width() / 2.0 + 20, floor->position.Y - 100, 5.0);
	Body* plank2 = new Body(BoxShape(50, 150), Graphics::Width() / 2.0 + 180, floor->position.Y - 100, 5.0);
	Body* plank3 = new Body(BoxShape(250, 25), Graphics::Width() / 2.0 + 100.0f, floor->position.Y - 200, 2.0);
	bodyTextures.Set(plank1, "../Content/angrybirds/wood-plank-solid.png");
	bodyTextures.Set(plank2, "../Content/angrybirds/wood-plank-solid.png");
	bodyTextures.Set(plank3, "../Content/angrybirds/wood-plank-cracked.png");
	world->AddBody(plank1);
	world->AddBody(plank2);
	world->AddBody(plank3);

	// Add a triangle polygon
	std::vector<FVector2D> triangleVertices = {FVector2D(30, 30), FVector2D(-30, 30), FVector2D(0, -30)};
	Body* triangle = new Body(PolygonShape(triangleVertices), plank3->position.X, plank3->position.Y - 50, 0.5);
	bodyTextures.Set(triangle, "../Content/angrybirds/wood-triangle.png");
	world->AddBody(triangle);

	// Add a pyramid of boxes
	constexpr int numRows = 5;
	for (int col = 0; col < numRows; col++)
	{
		for (int row = 0; row < col; row++)
		{
			const float x = (plank3->position.X + 200.0f) + col * 50.0f - (row * 25.0f);
			const float y = (floor->position.Y - 50.0f) - row * 52.0f;
			const float mass = (5.0f / (row + 1.0f));
			Body* box = new Body(BoxShape(50, 50), x, y, mass);
			box->friction = 0.9;
			box->restitution = 0.0;
			bodyTextures.Set(box, "../Content/angrybirds/wood-box.png");
			world->AddBody(box);
		}
	}

	// Add a bridge of connected steps and joints
	const int numSteps = 10;
	const int spacing = 33;
	Body* startStep = new Body(BoxShape(80, 20), 200, 200, 0.0);
	bodyTextures.Set(startStep, "../Content/angrybirds/rock-bridge-anchor.png");
	world->AddBody(startStep);
	Body* last = floor;

	for (int i = 1; i <= numSteps; i++)
	{
		const float x = startStep->position.X + 30 + (i * spacing);
		const float y = startStep->position.Y + 20;
		const float mass = (i == numSteps) ? 0.0 : 3.0;

		Body* step = new Body(CircleShape(15), x, y, mass);
		bodyTextures.Set(step, "../Content/angrybirds/wood-bridge-step.png");
		world->AddBody(step);

		JointConstraint* joint = new JointConstraint(last, step, step->position);
		world->AddConstraint(joint);

		last = step;
	}

	Body* endStep = new Body(BoxShape(80, 20), last->position.X + 60, last->position.Y - 20, 0.0);
	bodyTextures.Set(endStep, "../Content/angrybirds/rock-bridge-anchor.png");
	world->AddBody(endStep);

	// Add pigs
	Body* pig1 = new Body(CircleShape(30), plank1->position.X + 80, floor->position.Y - 50, 3.0);
	Body* pig2 = new Body(CircleShape(30), plank2->position.X + 400, floor->position.Y - 50, 3.0);
	Body* pig3 = new Body(CircleShape(30), plank2->position.X + 460, floor->position.Y - 50, 3.0);
	Body* pig4 = new Body(CircleShape(30), 220, 130, 1.0);
	bodyTextures.Set(pig1, "../Content/angrybirds/pig-1.png");
	bodyTextures.Set(pig2, "../Content/angrybirds/pig-2.png");
	bodyTextures.Set(pig3, "../Content/angrybirds/pig-1.png");
	bodyTextures.Set(pig4, "../Content/angrybirds/pig-2.png");
	world->AddBody(pig1);
	world->AddBody(pig2);
	world->AddBody(pig3);
	world->AddBody(pig4);

	// Add a big static circle in the middle of the screen
	Body* bigBall = new Body(CircleShape(64), Graphics::Width() / 2.0, Graphics::Height() / 2.0, 0.0);
	bodyTextures.Set(bigBall, "../Content/bowlingball.png");
	world->AddBody(bigBall);
}

/**
 *  Input processing.\
 */
void Application::Input()
{
	SDL_Event event;

	while (SDL_PollEvent(&event))
	{
		diagnostics.HandleEvent(event);

		switch (event.type)
		{
		case SDL_QUIT:
			running = false;
			break;

		case SDL_KEYDOWN:
			if (event.key.keysym.sym == SDLK_ESCAPE)
			{
				running = false;
			}
			if (event.key.keysym.sym == SDLK_d)
			{
				debug = !debug;
			}
			if (event.key.keysym.sym == SDLK_UP || event.key.keysym.sym == SDLK_SPACE)
			{
				world->GetBodies()[0]->ApplyImpulseLinear(FVector2D(0.0, -1110.0));
			}
			if (event.key.keysym.sym == SDLK_LEFT)
			{
				world->GetBodies()[0]->ApplyImpulseLinear(FVector2D(-150.0, 0.0));
			}
			if (event.key.keysym.sym == SDLK_RIGHT)
			{
				world->GetBodies()[0]->ApplyImpulseLinear(FVector2D(+150.0, 0.0));
			}
			break;

		case SDL_MOUSEBUTTONDOWN:
			if (event.button.button == SDL_BUTTON_LEFT)
			{
				int x, y;
				SDL_GetMouseState(&x, &y);
				Body* box = new Body(BoxShape(60, 60), x, y, 1.0);
				bodyTextures.Set(box, "../Content/angrybirds/rock-box.png");
				box->angularVelocity = 0.0;
				box->friction = 0.9;
				world->AddBody(box);
			}

			if (event.button.button == SDL_BUTTON_RIGHT)
			{
				int x, y;
				SDL_GetMouseState(&x, &y);
				Body* rock = new Body(CircleShape(30), x, y, 1.0);
				bodyTextures.Set(rock, "../Content/angrybirds/rock-round.png");
				rock->friction = 0.4;
				world->AddBody(rock);
			}

			break;
		}
	}
}

/**
 *  Update function (called several times per second to update objects)
 */
float Application::TimeDeductions()
{
	// Wait some time until the reach the target frame time in milliseconds
	static int timePreviousFrame;
	const int timeToWait = AE::Physics::MILLISECS_PER_FRAME - (SDL_GetTicks() - timePreviousFrame);

	if (timeToWait > 0)
	{
		SDL_Delay(timeToWait);
	}

	// Calculate the deltatime in seconds
	float deltaTime = (SDL_GetTicks() - timePreviousFrame) / 1000.0f;

	if (deltaTime > 0.016)
	{
		deltaTime = 0.016;
	}

	// Set the time of the current frame to be used in the next one
	timePreviousFrame = SDL_GetTicks();
	return deltaTime;
}

/**
 * Update time application
 */
void Application::Update()
{
	diagnostics.BeginFrame();
	const uint64 updateStart = LegacyDiagnosticsOverlay::Counter();

	Graphics::ClearScreen(0xFF0F0721);

	/* time deduction. */
	float deltaTime = TimeDeductions();

	/* World update. */
	if (!diagnostics.IsPaused())
	{
		world->Update(deltaTime);
	}

	diagnostics.SetUpdateMs(LegacyDiagnosticsOverlay::ElapsedMilliseconds(updateStart));
}

/**
 *  Render function (called several times per second to draw objects)
 */
void Application::Render()
{
	const uint64 renderStart = LegacyDiagnosticsOverlay::Counter();

	Graphics::DrawTexture(Graphics::Width() / 2.0, Graphics::Height() / 2.0, Graphics::Width(), Graphics::Height(), 0.0f, bgTexture);

	// Draw all bodies
	for (const auto& body : world->GetBodies())
	{
		if (body->shape->GetType() == CIRCLE)
		{
			CircleShape* circleShape = (CircleShape*)body->shape;
			SDL_Texture* texture = bodyTextures.Get(body);

			if (!debug && texture)
			{
				Graphics::DrawTexture(body->position.X, body->position.Y, circleShape->radius * 2, circleShape->radius * 2, body->rotation, texture);
			}
			else if (debug)
			{
				Graphics::DrawCircle(body->position.X, body->position.Y, circleShape->radius, body->rotation, 0xFF0000FF);
			}
		}

		if (body->shape->GetType() == BOX)
		{
			BoxShape* boxShape = (BoxShape*)body->shape;
			SDL_Texture* texture = bodyTextures.Get(body);
			if (!debug && texture)
			{
				Graphics::DrawTexture(body->position.X, body->position.Y, boxShape->width, boxShape->height, body->rotation, texture);
			}
			else if (debug)
			{
				Graphics::DrawPolygon(body->position.X, body->position.Y, boxShape->worldVertices, 0xFF0000FF);
			}
		}

		if (body->shape->GetType() == POLYGON)
		{
			PolygonShape* polygonShape = (PolygonShape*)body->shape;
			SDL_Texture* texture = bodyTextures.Get(body);
			if (!debug && texture)
			{
				Graphics::DrawTexture(
					body->position.X, body->position.Y, polygonShape->width, polygonShape->height, body->rotation, texture);
			}
			else if (debug)
			{
				Graphics::DrawPolygon(body->position.X, body->position.Y, polygonShape->worldVertices, 0xFF0000FF);
			}
		}
	}

	diagnostics.SetRenderMs(LegacyDiagnosticsOverlay::ElapsedMilliseconds(renderStart));
	LegacyDiagnosticsData diagnosticsData;
	diagnosticsData.sampleName = "AngryApp";
	diagnosticsData.world = world;
	diagnosticsData.debugDraw = debug;
	diagnosticsData.controls = "D debug, arrows/space impulse, LMB box, RMB rock, Esc quit";
	diagnostics.Draw(diagnosticsData);

	Graphics::RenderFrame();
}

/**
 * Render Object implementation
 */
void Application::RenderObjects()
{ /** */
}

/**
 * Destroy objects implementation
 */
void Application::Destroy()
{
	bodyTextures.Clear();
	SDL_DestroyTexture(bgTexture);
	delete world;
	Graphics::CloseWindow();
}
