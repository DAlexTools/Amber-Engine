#ifndef EVENT_H
#define EVENT_H

/**
 * @class Event
 * @brief A base class for all event types.
 * 
 * This is a base class that represents a generic event in the system. 
 * It is meant to be inherited by specific event classes such as 
 * KeyPressedEvent, MouseEvent, etc.
 */
class Event
{
public:
    /**
     * @brief Default constructor for the Event class.
     * 
     * This constructor is used to initialize the base class object. 
     * It is the default constructor and does not perform any additional 
     * initialization.
     */
	Event() = default;

    /**
     * @brief Virtual destructor for the Event class.
     * 
     * The destructor is virtual to ensure that derived classes can properly 
     * clean up their resources when an Event object is destroyed.
     */
	virtual ~Event() = default;
};


#endif