/**
 * @file error.h
 * Error handling functions
 *
*/

#ifndef ERROR_H
#define ERROR_H

#include <iostream>
#include <string>

namespace LOG
{

const std::string RED = "\033[31m";
const std::string GREEN = "\033[32m";
const std::string YELLOW = "\033[33m";
const std::string NC = "\033[0m"; // No Color

/**
 * Variadic template to print the arguments of the message
*/
void print_args()
{
    std::cerr << std::endl;
}

template <typename T, typename... Args>
void print_args(T first, Args... args)
{
    std::cerr << first;
    print_args(args...);
}


/**
 * Print a warning message
*/
template <typename... Args>
void Warning(bool condition, Args... args)
{
#ifndef NODEBUG

    if(condition)
    {
        std::cerr << YELLOW << "Warning: " << NC;
        print_args(args...);
    }

#endif
}

/**
 * Print an info message
*/
template <typename... Args>
void Info(bool condition, Args... args)
{
#ifndef NODEBUG

    if(condition)
    {
        std::cerr << "Info: ";
        print_args(args...);
    }

#endif
}

/**
 * Print an error message
*/
template <typename... Args>
void Error(bool condition, Args... args)
{
#ifndef NODEBUG

    if(condition)
    {
        std::cerr << RED << "\nERROR: " << NC;
        print_args(args...);
    }

#endif
}

} // namespace LOG

#endif // ERROR_H