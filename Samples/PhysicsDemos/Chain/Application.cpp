#include "Application.h"
#include "Physics/Constants.h"
#include "Physics/Force.h"
#include "Physics/CollisionDetection.h"
#include "Physics/Contact.h"
#include "Physics/Constraint.h"

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

	anchor = FVector2D(Graphics::Width() / 2.0, 30);

	for (int i = 0; i < NUM_PARTICLES; i++)
	{
		Particle* particle = new Particle(anchor.X, anchor.Y + (i * restLength), 2.0);
		particle->radius = 6;
		particles.push_back(particle);
	}
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
				int x, y;
				SDL_GetMouseState(&x, &y);
				mouseCursor.X = x;
				mouseCursor.Y = y;
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
		// Apply forces to the particles
		for (auto particle : particles)
		{
			particle->AddForce(pushForce);

			// Apply a drag force
			FVector2D drag = Force::GenerateDragForce(*particle, 0.002);
			particle->AddForce(drag);

			// Apply weight force
			FVector2D weight = FVector2D(0.0, particle->mass * 9.8 * AE::Physics::PIXELS_PER_METER);
			particle->AddForce(weight);
		}

		// Attach the head to the anchor with a spring
		FVector2D springForce = Force::GenerateSpringForce(*particles[0], anchor, restLength, k);
		particles[0]->AddForce(springForce);

		// Connect the particles with the one before in a chain of springs
		for (int i = 1; i < NUM_PARTICLES; i++)
		{
			int currParticle = i;
			int prevParticle = i - 1;
			FVector2D springForce = Force::GenerateSpringForce(*particles[currParticle], *particles[prevParticle], restLength, k);
			particles[currParticle]->AddForce(springForce);
			particles[prevParticle]->AddForce(-springForce);
		}

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

	// Draw the anchor and the spring to the first bob
	Graphics::DrawFillCircle(anchor.X, anchor.Y, 5, 0xFF001155);
	Graphics::DrawLine(anchor.X, anchor.Y, particles[0]->position.X, particles[0]->position.Y, 0xFF313131);

	// Draw all the springs from one particle to the next
	for (int i = 0; i < NUM_PARTICLES - 1; i++)
	{
		int currParticle = i;
		int nextParticle = i + 1;
		Graphics::DrawLine(particles[currParticle]->position.X, particles[currParticle]->position.Y, particles[nextParticle]->position.X, particles[nextParticle]->position.Y, 0xFF313131);
	}

	// Draw all the bob particles
	for (auto particle : particles)
	{
		Graphics::DrawFillCircle(particle->position.X, particle->position.Y, particle->radius, 0xFFEEBB00);
	}

	diagnostics.SetRenderMs(LegacyDiagnosticsOverlay::ElapsedMilliseconds(renderStart));
	LegacyDiagnosticsData diagnosticsData;
	diagnosticsData.sampleName = "ChainApp";
	diagnosticsData.particleCount = particles.size();
	diagnosticsData.controls = "Arrows apply force, LMB drag/release last particle, Esc quit";
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

	Graphics::CloseWindow();
}
