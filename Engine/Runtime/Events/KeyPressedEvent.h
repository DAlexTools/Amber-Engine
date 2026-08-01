#ifndef KEYPRESSEDEVENT_H
#define KEYPRESSEDEVENT_H

#include "../EntityComponentSystem/ECS.h"
#include "../EventBus/Event.h"
#include <SDL2/SDL.h>

/**
 * @class KeyPressedEvent
 * @brief Represents an event triggered when a key is pressed.
 *
 * This class is derived from the base Event class and encapsulates the information
 * about the key that was pressed, specifically its SDL keycode.
 */
class KeyPressedEvent : public Event
{
public:
	/**
	 * @brief The SDL keycode representing the key that was pressed.
	 *
	 * SDL_Keycode is used to uniquely identify the key that triggered the event.
	 */
	SDL_Keycode symbol;

	/**
	 * @brief Constructor for creating a KeyPressedEvent with a specific key.
	 *
	 * @param symbol The SDL_Keycode representing the key that was pressed.
	 * This value is passed to the constructor to initialize the symbol member variable.
	 */
	KeyPressedEvent(SDL_Keycode symbol)
		: symbol(symbol)
	{
	}
};

/**
 *
 */
class KeyReleasedEvent : public Event
{
public:
	/**
	 * @brief The SDL keycode representing the key that was pressed.
	 *
	 * SDL_Keycode is used to uniquely identify the key that triggered the event.
	 */
	SDL_Keycode symbol;

	/**
	 * @brief Constructor for creating a KeyPressedEvent with a specific key.
	 *
	 * @param symbol The SDL_Keycode representing the key that was pressed.
	 * This value is passed to the constructor to initialize the symbol member variable.
	 */
	KeyReleasedEvent(SDL_Keycode symbol)
		: symbol(symbol)
	{
	}
};

#endif
