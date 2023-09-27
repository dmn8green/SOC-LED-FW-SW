//******************************************************************************
/**
 * @file NoCopy.h
 * @author pat laplante (plaplante@appliedlogix.com)
 * @brief NoCopy class definition
 * @version 0.1
 * @date 2023-09-19
 * 
 * @copyright Copyright MN8 (c) 2023
 */
//******************************************************************************
#pragma once

//******************************************************************************
/**
 * @brief NoCopy class.
 * 
 * This class is used to disable the copy constructor and the assignment operator
 * for a class.
 */
class NoCopy {
public:
    NoCopy() = default;
    ~NoCopy() = default;

    // No copy constructor
    NoCopy(const NoCopy&) = delete;
    NoCopy& operator=(const NoCopy&) = delete;
};
