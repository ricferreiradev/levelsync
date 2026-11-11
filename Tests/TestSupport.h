#pragma once

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

inline void require(bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

inline void near(double actual, double expected, double tolerance, const std::string& message)
{
    require(std::isfinite(actual) && std::abs(actual - expected) <= tolerance,
            message + ": expected " + std::to_string(expected) + ", got " + std::to_string(actual));
}

template <typename Function>
void run(const char* name, Function test, int& failures)
{
    try
    {
        test();
        std::cout << "PASS " << name << '\n';
    }
    catch (const std::exception& error)
    {
        ++failures;
        std::cerr << "FAIL " << name << ": " << error.what() << '\n';
    }
}
