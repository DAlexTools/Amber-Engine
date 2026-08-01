#ifndef EVENTBUS_H
#define EVENTBUS_H

#include "../Logging/Logger.h"
#include "./Event.h"
#include <map>
#include <functional>
#include <typeindex>
#include <list>

/**
 * @interface IEventCallback
 * @brief Interface for event callback handling.
 *
 * This is a base interface class that provides a contract for handling
 * events in the system. Derived classes must implement the Call() method
 * to define the behavior when an event occurs.
 */
class IEventCallback
{
private:
	/**
	 * @brief Abstract method that is called when an event is triggered.
	 *
	 * This method is a pure virtual function that must be overridden in
	 * derived classes to implement the specific logic for handling events.
	 *
	 * @param event The event that triggered the callback.
	 */
	virtual void Call(Event& event) = 0;

public:
	/**
	 * @brief Virtual destructor for the IEventCallback class.
	 *
	 * The destructor is virtual to ensure proper cleanup of derived
	 * classes when an IEventCallback object is destroyed.
	 */
	virtual ~IEventCallback() = default;

	/**
	 * @brief Executes the callback function.
	 *
	 * This method triggers the Call() function to handle the event,
	 * which is implemented by derived classes.
	 *
	 * @param event The event to be processed.
	 */
	void Execute(Event& event)
	{
		Call(event);
	}
};

/**
 * @class EventCallback
 * @brief A template-based callback handler for specific event types.
 *
 * This class implements the IEventCallback interface and provides
 * the ability to bind a specific event handling function to a particular
 * event type and an owner instance. It is templated to allow flexibility
 * in the types of events and owner instances that can be handled.
 *
 * @tparam TOwner The type of the class that owns the callback function.
 * @tparam TEvent The type of the event to be handled.
 */
template <typename TOwner, typename TEvent>
class EventCallback : public IEventCallback
{
private:
	/**
	 * @brief Alias for a callback function type.
	 *
	 * This typedef defines a type for the callback function, which is a
	 * member function of the TOwner class that takes a TEvent reference.
	 */
	typedef void (TOwner::*CallbackFunction)(TEvent&);

	/**
	 * @brief Pointer to the instance of the owner class.
	 *
	 * This pointer holds the reference to the object that owns the callback function.
	 */
	TOwner* ownerInstance;

	/**
	 * @brief The callback function to be invoked.
	 *
	 * This member holds the callback function that will be called when the event is triggered.
	 */
	CallbackFunction callbackFunction;

	/**
	 * @brief Calls the callback function with the event.
	 *
	 * This method overrides the Call() method from IEventCallback and
	 * invokes the stored callback function on the owner instance, passing
	 * the event as a parameter.
	 *
	 * @param event The event to be passed to the callback function.
	 */
	virtual void Call(Event& event) override
	{
		std::invoke(callbackFunction, ownerInstance, static_cast<TEvent&>(event));
	}

public:
	/**
	 * @brief Constructor to initialize the event callback.
	 *
	 * This constructor initializes the owner instance and the callback function
	 * to be invoked when the event is triggered.
	 *
	 * @param ownerInstance A pointer to the instance of the owner class.
	 * @param callbackFunction A pointer to the callback function to be called.
	 */
	EventCallback(TOwner* ownerInstance, CallbackFunction callbackFunction)
	{
		this->ownerInstance = ownerInstance;
		this->callbackFunction = callbackFunction;
	}

	/**
	 * @brief Virtual destructor for the EventCallback class.
	 *
	 * The destructor is virtual to ensure proper cleanup of resources.
	 */
	virtual ~EventCallback() = default;
};

/**
 * @typedef HandlerList
 * @brief Alias for a list of event callback handlers.
 *
 * This alias defines a list of unique pointers to IEventCallback objects,
 * which represent the subscribers for the event bus. These handlers are
 * responsible for responding to specific events when they are emitted.
 */
using HandlerList = std::list<std::unique_ptr<IEventCallback>>;

/**
 * @class EventBus
 * @brief A class that handles event subscription and emission.
 *
 * The EventBus is responsible for managing event handlers (callbacks) and
 * emitting events to the subscribed handlers. It allows the decoupling of
 * event producers and consumers by providing a system where events can
 * be subscribed to and emitted dynamically.
 */
class EventBus
{
	// Stores event handlers for each event type
	std::map<std::type_index, std::unique_ptr<HandlerList>> subscribers;

public:
	/**
	 * @brief Constructor for the EventBus class.
	 *
	 * Initializes the EventBus and logs its creation.
	 */
	EventBus()
	{
		AE::Logger::Log("Event bus contructor called");
	}

	/**
	 * @brief Destructor for the EventBus class.
	 *
	 * Cleans up the EventBus and logs its destruction.
	 */
	~EventBus()
	{
		AE::Logger::Log("Event bus destructor called");
	}

	/**
	 * @brief Resets the EventBus by clearing all subscriptions.
	 *
	 * This method clears all event subscriptions, effectively removing all
	 * event handlers and resetting the state of the EventBus.
	 */
	void Reset()
	{
		subscribers.clear();
	}

	/**
	 * @brief Subscribes a callback function to a specific event type.
	 *
	 * This template function allows an owner instance to subscribe to an
	 * event type by specifying a callback function. The callback function
	 * will be invoked when the specified event is emitted.
	 *
	 * @tparam TEvent The event type that will trigger the callback.
	 * @tparam TOwner The type of the class that owns the callback function.
	 * @param ownerInstance A pointer to the instance that owns the callback.
	 * @param callbackFunction A pointer to the callback function that will be invoked.
	 */
	template <typename TEvent, typename TOwner>
	void SubscribeToEvent(TOwner* ownerInstance, void (TOwner::*callbackFunction)(TEvent&))
	{
		// Create a new subscriber list if not already present for this event type
		if (!subscribers[typeid(TEvent)].get())
		{
			subscribers[typeid(TEvent)] = std::make_unique<HandlerList>();
		}

		// Create a new EventCallback and add it to the list of subscribers
		auto subscriber = std::make_unique<EventCallback<TOwner, TEvent>>(ownerInstance, callbackFunction);
		subscribers[typeid(TEvent)]->push_back(std::move(subscriber));
	}

	/**
	 * @brief Emits an event and invokes all subscribed handlers.
	 *
	 * This template function emits an event of type TEvent to all the subscribed
	 * handlers. The event will be constructed with the provided arguments,
	 * and each handler will be called with the event.
	 *
	 * @tparam TEvent The type of the event to emit.
	 * @tparam TArgs The types of the arguments to be passed to the event constructor.
	 * @param args Arguments that will be forwarded to the event constructor.
	 */
	template <typename TEvent, typename... TArgs>
	void EmitEvent(TArgs&&... args)
	{
		const auto handlers = subscribers[typeid(TEvent)].get();

		if (handlers)
		{
			for (auto it = handlers->begin(); it != handlers->end(); it++)
			{
				auto handler = it->get();
				TEvent event(std::forward<TArgs>(args)...);
				handler->Execute(event);
			}
		}
	}
};

#endif
