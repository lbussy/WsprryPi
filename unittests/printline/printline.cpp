#include <iostream>

// Recursive variadic functions for stdout/err control
template <typename T>
void prtStdOut(T t) 
{
    std::cout << t << std::endl << std::flush;
}
template<typename T, typename... Args>
void prtStdOut(T t, Args... args)
{
    std::cout << t;
    prtStdOut(args...);
}

template <typename T>
void prtStdErr(T t) 
{
    std::cerr << t << std::endl << std::flush;
}
template<typename T, typename... Args>
void prtStdErr(T t, Args... args)
{
    std::cerr << t;
    prtStdErr(args...);
}

int main ()
{
    prtStdOut("Standard");
    prtStdOut("Standard", " test1", " test2");
    prtStdOut("Standard", " test1", " test2");
    prtStdErr("Error");
    prtStdErr("Error", " test1", " test2");
    prtStdErr("Error", " test1", " test2");
}