//******************************************************************************
/**
 * @file Singleton.h
 * @author pat laplante (plaplante@appliedlogix.com)
 * @brief Singleton class definition
 * @version 0.1
 * @date 2023-09-19
 * 
 * @copyright Copyright MN8 (c) 2023
 */
//******************************************************************************
#pragma once

#include "Utils/NoCopy.h"

//******************************************************************************
/**
 * @brief Singleton template class.
 * 
 * This is a poor man singleton class.  It is not thread safe.
 * It is enough for our current need.
 */
template<typename T>
class Singleton : public NoCopy {
public:
    static T& instance();

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