#pragma once

#include <homeshell/Status.hpp>
#include <homeshell/version.h>

namespace homeshell
{

class Homeshell
{
public:
    Homeshell();
    ~Homeshell();

    Status run();
};

} // namespace homeshell
