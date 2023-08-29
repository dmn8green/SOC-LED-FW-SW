#pragma once

/**
 * @brief Singleton template class.
 * 
 * This is a poor man singleton class.  It is not thread safe.
 * It is enough for our current need.
 */
template<typename T>
class Singleton {
public:
    static T& instance();

    Singleton(const Singleton&) = delete;
    Singleton& operator= (const Singleton) = delete;

protected:
    struct token {};
    Singleton() {}
};

#include <memory>
template<typename T>
T& Singleton<T>::instance()
{
    // I'm using a constructor token to allow the base class to call
    // the subclass's constructor without needing to be a friend.

    static T instance{token{}}; // Guaranteed to be destroyed.
                                // Instantiated on first use.
    return instance;
}