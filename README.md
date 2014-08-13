# kit
My personal C++11 toolset

## async
Stackful coroutines, task scheduling, strands, (Go-style) bufferable channels

## meta
Thread-aware serializable meta-objects, property trees, and reflection

## freq
Timelines, alarms, animation/easing, waypoints/keyframes, interpolation

## log
Logger w/ error handling, thread-safe scoped indent, silencing, and capturing

## math
some math stuff to use with glm

## common (kit.h)
Common stuff used by other modules, including:

- freezable: freeze objects as immutable
- make_unique: clone of c++14 function
- dummy_mutex
- ENTIRE() range macro
- bit() and mask()
- null_ptr_exception
- scoped_unlock
- scoped_dtor: deprecated in favor of BOOST_SCOPE_EXIT_ALL()
- thread-safe singleton
- timed function auto-retry
- index data structures w/ unused ID approximation

