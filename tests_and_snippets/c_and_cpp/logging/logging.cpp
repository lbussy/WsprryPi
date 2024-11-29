#include "lcblog.hpp"

LCBLog llog;

int main()
{
    llog.setDaemon(false);
    std::cout << std::endl << "Testing standard out." << std::endl;
    llog.logS(100);
    llog.logS(100.01);
    llog.logS("\t\t\t\t\t\tFoo");
    llog.logS("Foo", " foo foo.");
    llog.logS("Foo", " \t\t\t\t\t\t\t\t\tfoo foo.");
    llog.logS("Foo ", 100, " \t\t\t\t\t\t\t\t\tfoo foo.");
    llog.logS("Foo ", 100.01, " \nfoo foo.");

    llog.setDaemon(true);
    std::cout << std::endl << "Testing standard out with stamps." << std::endl;
    llog.logS(100);
    llog.logS(100.01);
    llog.logS("\t\t\t\t\t\tFoo");
    llog.logS("Foo", " foo foo.");
    llog.logS("Foo", " \t\t\t\t\t\t\t\t\tfoo foo.");
    llog.logS("Foo ", 100, " \t\t\t\t\t\t\t\t\tfoo foo.");
    llog.logS("Foo ", 100.01, " \nfoo foo.");

    llog.setDaemon(false);
    std::cerr << std::endl << "Testing standard error." << std::endl;
    llog.logE(100);
    llog.logE(100.01);
    llog.logE("\t\t\t\t\t\tFoo");
    llog.logE("Foo", " foo foo.");
    llog.logE("Foo", " \t\t\t\t\t\t\t\t\tfoo foo.");
    llog.logE("Foo ", 100, " \t\t\t\t\t\t\t\t\tfoo foo.");
    llog.logE("Foo ", 100.01, " \nfoo foo.");

    llog.setDaemon(true);
    std::cerr << std::endl << "Testing standard error." << std::endl;
    llog.logE(100);
    llog.logE(100.01);
    llog.logE("\t\t\t\t\t\tFoo");
    llog.logE("Foo", " foo foo.");
    llog.logE("Foo", " \t\t\t\t\t\t\t\t\tfoo foo.");
    llog.logE("Foo ", 100, " \t\t\t\t\t\t\t\t\tfoo foo.");
    llog.logE("Foo ", 100.01, " \nfoo foo.");
}
