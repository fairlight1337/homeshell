#include <homeshell/Homeshell.hpp>
#include <homeshell/Shell.hpp>

namespace homeshell
{

// Static member definition
Shell* Shell::instance_ = nullptr;

Homeshell::Homeshell()
{
}

Homeshell::~Homeshell()
{
}

Status Homeshell::run()
{
    return Status{0, "Success"};
}

} // namespace homeshell
