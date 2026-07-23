#ifndef MACRO_H
#define MACRO_H


#if ASSERTIONS_ENABLED
#define debugBreak() asm { int 3 }
#define ASSERT(expr) \
    if (expr) { } \
    else \
    { \
        reportAssertionFailure(#expr, \
            __FILE__, __LINE__); \
        debugBreak(); \
    } \
#else

#define ASSERT(expr) 

#endif



/**
 * check condition isboolean return 
 */
#define check(condition, msg)       \
    if (!(condition))               \
    {                               \
        AE::Logger::Err(msg);           \
        (__nop(), __debugbreak());      \
        return;                     \
    }


#endif

// Expands to nothing - used as a placeholder
#define UE_EMPTY

// Expands to nothing when used as a function - used as a placeholder
#define UE_EMPTY_FUNCTION(...)


#if defined(__INTELLISENSE__)
#define AE_DEPRECATED(Version, Message)
#else
#define AE_DEPRECATED(Version, Message)                                        \
  [[deprecated(                                                                \
      Message                                                                  \
      " Please update your code to the new API before upgrading to the next "  \
      "release, otherwise your project will no longer compile.")]]
#endif


#if AE_VALIDATE_INTERNAL_API
#define AE_INTERNAL [[deprecated("Please remove usage of this internal API before upgrading to the next release, otherwise your project will no longer compile.")]]
#else
#define AE_INTERNAL
#endif