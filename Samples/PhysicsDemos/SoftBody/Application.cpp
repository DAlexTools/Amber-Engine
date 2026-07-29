#include "Application.h"
#include "Physics/Constants.h"
#include "Physics/Force.h"

bool Application::IsRunning()
{
	return running;
}

///////////////////////////////////////////////////////////////////////////////
// Setup function (executed once in the beginning of the simulation)
///////////////////////////////////////////////////////////////////////////////
void Application::Setup()
{
	running = Graphics::OpenWindow();

	// Create a physics world with gravity of -9.8 m/s2
	world = new World(-9.8);

	// Add a floor and walls to contain objects objects
	Body* floor = new Body(BoxShape(Graphics::Width() - 50, 50), Graphics::Width() / 2.0, Graphics::Height() / 2.0 + 340, 0.0);
	Body* leftFence = new Body(BoxShape(50, Graphics::Height() - 200), 0, Graphics::Height() / 2.0 - 35, 0.0);
	Body* rightFence = new Body(BoxShape(50, Graphics::Height() - 200), Graphics::Width(), Graphics::Height() / 2.0 - 35, 0.0);
	Body* top = new Body(BoxShape(Graphics::Width() + 50, 50), Graphics::Width() / 2.0, Graphics::Height() / 2.0 - 500, 0.0);
	floor->restitution = 0.7;
	leftFence->restitution = 0.2;
	rightFence->restitution = 0.2;
	top->restitution = 0.2;

	world->AddBody(floor);
	world->AddBody(leftFence);
	world->AddBody(rightFence);
	world->AddBody(top);

	Particle* a = new Particle(100, 100, 1.0);
	Particle* b = new Particle(300, 100, 1.0);
	Particle* c = new Particle(300, 300, 1.0);
	Particle* d = new Particle(100, 300, 1.0);

	a->radius = 6;
	b->radius = 6;
	c->radius = 6;
	d->radius = 6;

	particles.push_back(a);
	particles.push_back(b);
	particles.push_back(c);
	particles.push_back(d);
}

///////////////////////////////////////////////////////////////////////////////
// Input processing
///////////////////////////////////////////////////////////////////////////////
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
				running = false;
			if (event.key.keysym.sym == SDLK_UP)
				pushForce.Y = -50 * AE::Physics::PIXELS_PER_METER;
			if (event.key.keysym.sym == SDLK_RIGHT)
				pushForce.X = 50 * AE::Physics::PIXELS_PER_METER;
			if (event.key.keysym.sym == SDLK_DOWN)
				pushForce.Y = 50 * AE::Physics::PIXELS_PER_METER;
			if (event.key.keysym.sym == SDLK_LEFT)
				pushForce.X = -50 * AE::Physics::PIXELS_PER_METER;
			break;
		case SDL_KEYUP:
			if (event.key.keysym.sym == SDLK_UP)
				pushForce.Y = 0;
			if (event.key.keysym.sym == SDLK_RIGHT)
				pushForce.X = 0;
			if (event.key.keysym.sym == SDLK_DOWN)
				pushForce.Y = 0;
			if (event.key.keysym.sym == SDLK_LEFT)
				pushForce.X = 0;
			break;
		case SDL_MOUSEMOTION:
			mouseCursor.X = event.motion.x;
			mouseCursor.Y = event.motion.y;
			break;
		case SDL_MOUSEBUTTONDOWN:
			if (!leftMouseButtonDown && event.button.button == SDL_BUTTON_LEFT)
			{
				leftMouseButtonDown = true;
				int X, Y;
				SDL_GetMouseState(&X, &Y);
				mouseCursor.X = X;
				mouseCursor.Y = Y;
			}
			break;
		case SDL_MOUSEBUTTONUP:
			if (leftMouseButtonDown && event.button.button == SDL_BUTTON_LEFT)
			{
				leftMouseButtonDown = false;
				int lastParticle = NUM_PARTICLES - 1;
				FVector2D impulseDirection = (particles[lastParticle]->position - mouseCursor).UnitVector();
				float impulseMagnitude = (particles[lastParticle]->position - mouseCursor).Magnitude() * 5.0;
				particles[lastParticle]->velocity = impulseDirection * impulseMagnitude;
			}
			break;
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
// Update function (called several times per second to update objects)
///////////////////////////////////////////////////////////////////////////////
void Application::Update()
{
	diagnostics.BeginFrame();
	const uint64 updateStart = LegacyDiagnosticsOverlay::Counter();

	// Wait some time until the reach the target frame time in milliseconds
	static int timePreviousFrame;
	int timeToWait = AE::Physics::MILLISECS_PER_FRAME - (SDL_GetTicks() - timePreviousFrame);
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

	if (!diagnostics.IsPaused())
	{
		particles[0]->AddForce(pushForce);

		// Apply forces to the particles
		for (auto particle : particles)
		{
			// Apply a drag force
			FVector2D drag = Force::GenerateDragForce(*particle, 0.003);
			particle->AddForce(drag);

			// Apply weight force
			FVector2D weight = FVector2D(0.0, particle->mass * 9.8 * AE::Physics::PIXELS_PER_METER);
			particle->AddForce(weight);
		}

		// Attach particles with springs
		FVector2D ab = Force::GenerateSpringForce(*particles[0], *particles[1], restLength, k); // a <-> b
		particles[0]->AddForce(ab);
		particles[1]->AddForce(-ab);

		FVector2D bc = Force::GenerateSpringForce(*particles[1], *particles[2], restLength, k); // b <-> c
		particles[1]->AddForce(bc);
		particles[2]->AddForce(-bc);

		FVector2D cd = Force::GenerateSpringForce(*particles[2], *particles[3], restLength, k); // c <-> d
		particles[2]->AddForce(cd);
		particles[3]->AddForce(-cd);

		FVector2D da = Force::GenerateSpringForce(*particles[3], *particles[0], restLength, k); // d <-> a
		particles[3]->AddForce(da);
		particles[0]->AddForce(-da);

		FVector2D ac = Force::GenerateSpringForce(*particles[0], *particles[2], restLength, k); // a <-> c
		particles[0]->AddForce(ac);
		particles[2]->AddForce(-ac);

		FVector2D bd = Force::GenerateSpringForce(*particles[1], *particles[3], restLength, k); // b <-> d
		particles[1]->AddForce(bd);
		particles[3]->AddForce(-bd);

		// Integrate the acceleration and velocity to estimate the new position
		for (auto particle : particles)
		{
			particle->Integrate(deltaTime);
		}

		// Check the boundaries of the window
		for (auto particle : particles)
		{
			// Nasty hardcoded flip in velocity if it touches the limits of the screen window
			if (particle->position.X - particle->radius <= 0)
			{
				particle->position.X = particle->radius;
				particle->velocity.X *= -0.9;
			}
			else if (particle->position.X + particle->radius >= Graphics::Width())
			{
				particle->position.X = Graphics::Width() - particle->radius;
				particle->velocity.X *= -0.9;
			}
			if (particle->position.Y - particle->radius <= 0)
			{
				particle->position.Y = particle->radius;
				particle->velocity.Y *= -0.9;
			}
			else if (particle->position.Y + particle->radius >= Graphics::Height())
			{
				particle->position.Y = Graphics::Height() - particle->radius;
				particle->velocity.Y *= -0.9;
			}
		}
	}

	diagnostics.SetUpdateMs(LegacyDiagnosticsOverlay::ElapsedMilliseconds(updateStart));
}

///////////////////////////////////////////////////////////////////////////////
// Render function (called several times per second to draw objects)
///////////////////////////////////////////////////////////////////////////////
void Application::Render()
{
	const uint64 renderStart = LegacyDiagnosticsOverlay::Counter();

	Graphics::ClearScreen(0xFFFFFFFF);

	if (leftMouseButtonDown)
	{
		int lastParticle = NUM_PARTICLES - 1;
		Graphics::DrawLine(particles[lastParticle]->position.X, particles[lastParticle]->position.Y, mouseCursor.X, mouseCursor.Y, 0xFF0000FF);
	}

	// Draw all springs
	Graphics::DrawLine(particles[0]->position.X, particles[0]->position.Y, particles[1]->position.X, particles[1]->position.Y, 0xFF313131);
	Graphics::DrawLine(particles[1]->position.X, particles[1]->position.Y, particles[2]->position.X, particles[2]->position.Y, 0xFF313131);
	Graphics::DrawLine(particles[2]->position.X, particles[2]->position.Y, particles[3]->position.X, particles[3]->position.Y, 0xFF313131);
	Graphics::DrawLine(particles[3]->position.X, particles[3]->position.Y, particles[0]->position.X, particles[0]->position.Y, 0xFF313131);
	Graphics::DrawLine(particles[0]->position.X, particles[0]->position.Y, particles[2]->position.X, particles[2]->position.Y, 0xFF313131);
	Graphics::DrawLine(particles[1]->position.X, particles[1]->position.Y, particles[3]->position.X, particles[3]->position.Y, 0xFF313131);

	// Draw all particles
	Graphics::DrawFillCircle(particles[0]->position.X, particles[0]->position.Y, particles[0]->radius, 0xFFEEBB00);
	Graphics::DrawFillCircle(particles[1]->position.X, particles[1]->position.Y, particles[1]->radius, 0xFFEEBB00);
	Graphics::DrawFillCircle(particles[2]->position.X, particles[2]->position.Y, particles[2]->radius, 0xFFEEBB00);
	Graphics::DrawFillCircle(particles[3]->position.X, particles[3]->position.Y, particles[3]->radius, 0xFFEEBB00);

	// Draw all bodies
	for (const auto& body : world->GetBodies())
	{
		if (body->shape->GetType() == CIRCLE)
		{
			CircleShape* circleShape = (CircleShape*)body->shape;

			Graphics::DrawCircle(body->position.X, body->position.Y, circleShape->radius, body->rotation, 0xFF0000FF);
		}

		if (body->shape->GetType() == BOX)
		{
			BoxShape* boxShape = (BoxShape*)body->shape;
			Graphics::DrawPolygon(body->position.X, body->position.Y, boxShape->worldVertices, 0xFF0000FF);
		}

		if (body->shape->GetType() == POLYGON)
		{
			PolygonShape* polygonShape = (PolygonShape*)body->shape;
			Graphics::DrawPolygon(body->position.X, body->position.Y, polygonShape->worldVertices, 0xFF0000FF);
		}
	}

	diagnostics.SetRenderMs(LegacyDiagnosticsOverlay::ElapsedMilliseconds(renderStart));
	LegacyDiagnosticsData diagnosticsData;
	diagnosticsData.sampleName = "SoftBodyApp";
	diagnosticsData.world = world;
	diagnosticsData.particleCount = particles.size();
	diagnosticsData.debugDraw = debug;
	diagnosticsData.controls = "Arrows apply force, LMB drag/release particle, Esc quit";
	diagnostics.Draw(diagnosticsData);

	Graphics::RenderFrame();
}

///////////////////////////////////////////////////////////////////////////////
// Destroy function to delete objects and close the window
///////////////////////////////////////////////////////////////////////////////
void Application::Destroy()
{
	for (auto particle : particles)
	{
		delete particle;
	}

	delete world;
	Graphics::CloseWindow();
}
