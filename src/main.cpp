#include <homeshell/Homeshell.hpp>

int main(int argc, char** argv)
{
    using namespace homeshell;

    Homeshell homeshell;

    return homeshell.run().code;
}
