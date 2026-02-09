
#include <tuple>
#include <fstream>
#include <string>

/**
 * Variadic template to print a tuple of any size.
 *
*/
template<class Tuple, std::size_t N>
struct TuplePrinter {
    static void print(std::ostream& os, const std::string & separator, const Tuple& t)
    {
        TuplePrinter<Tuple, N-1>::print(os, separator, t);
        os << separator << std::get<N-1>(t);
    }
};

template<class Tuple>
struct TuplePrinter<Tuple, 1> {
    static void print(std::ostream& os, const std::string & separator, const Tuple& t)
    {
        os << std::get<0>(t);
    }
};

template<typename... Args>
void print_tuple(std::ostream& os, const std::string & separator, const std::tuple<Args...>& t)
{
    //os << "(";
    TuplePrinter<std::tuple<Args...>, sizeof...(Args)>::print(os, separator, t);
    //os << ")";
}


