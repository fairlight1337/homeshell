#pragma once

#include <iostream>
#include <string>

#ifdef _WIN32
#include <conio.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

namespace homeshell
{

/**
 * @brief Utility for secure password input without echo
 */
class PasswordInput
{
public:
    /**
     * @brief Read a password from stdin without echoing characters
     * @param prompt The prompt to display before reading
     * @return The password string
     */
    static std::string readPassword(const std::string& prompt = "Password: ")
    {
        std::cout << prompt << std::flush;

        std::string password;

#ifdef _WIN32
        // Windows implementation using _getch()
        char ch;
        while ((ch = _getch()) != '\r' && ch != '\n')
        {
            if (ch == '\b')
            {
                // Handle backspace
                if (!password.empty())
                {
                    password.pop_back();
                    std::cout << "\b \b" << std::flush;
                }
            }
            else
            {
                password += ch;
                std::cout << '*' << std::flush;
            }
        }
#else
        // Unix/Linux implementation using termios
        termios oldt;
        tcgetattr(STDIN_FILENO, &oldt);
        termios newt = oldt;
        newt.c_lflag &= ~ECHO;
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);

        std::getline(std::cin, password);

        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
#endif

        std::cout << std::endl;
        return password;
    }

    /**
     * @brief Read a password with confirmation
     * @param prompt The initial prompt
     * @param confirm_prompt The confirmation prompt
     * @return The password if both match, empty string otherwise
     */
    static std::string
    readPasswordWithConfirmation(const std::string& prompt = "New password: ",
                                 const std::string& confirm_prompt = "Confirm password: ")
    {
        std::string password1 = readPassword(prompt);
        if (password1.empty())
        {
            return "";
        }

        std::string password2 = readPassword(confirm_prompt);

        if (password1 != password2)
        {
            std::cerr << "Passwords do not match!" << std::endl;
            return "";
        }

        return password1;
    }
};

} // namespace homeshell
