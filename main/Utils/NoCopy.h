#pragma once

//******************************************************************************
/**
 * @brief NoCopy class.
 * 
 * This class is used to disable the copy constructor and the assignment operator
 * for a class.
 */

//******************************************************************************
class NoCopy {
public:
    NoCopy() = default;
    ~NoCopy() = default;

    // No copy constructor
    NoCopy(const NoCopy&) = delete;
    NoCopy& operator=(const NoCopy&) = delete;
};
