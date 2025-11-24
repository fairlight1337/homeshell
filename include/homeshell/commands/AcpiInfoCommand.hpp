/**
 * @file AcpiInfoCommand.hpp
 * @brief Display ACPI (Advanced Configuration and Power Interface) status
 *
 * This command displays the current ACPI status and configuration of the host
 * machine, including battery status, AC adapter state, and thermal information.
 * Information is read from the Linux sysfs interface (/sys/class/).
 *
 * @author Homeshell Development Team
 * @date 2025
 */

#pragma once

#include <homeshell/Command.hpp>
#include <homeshell/Status.hpp>

#include <dirent.h>

#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace homeshell
{

/**
 * @class AcpiInfoCommand
 * @brief Command to display ACPI status and configuration
 *
 * Displays information about:
 * - Battery status (capacity, state, health, voltage, current)
 * - AC adapter status (online/offline)
 * - Thermal zones (temperatures)
 *
 * Information is read from /sys/class/power_supply/ and /sys/class/thermal/
 *
 * @section Usage
 * @code
 * acpi-info              # Show all ACPI information
 * acpi-info -b           # Show only battery information
 * acpi-info -a           # Show only AC adapter information
 * acpi-info -t           # Show only thermal information
 * acpi-info --help       # Show help message
 * @endcode
 */
class AcpiInfoCommand : public ICommand
{
public:
    /**
     * @brief Get command name
     * @return Command name "acpi-info"
     */
    std::string getName() const override
    {
        return "acpi-info";
    }

    /**
     * @brief Get command description
     * @return Brief description of the command
     */
    std::string getDescription() const override
    {
        return "Display ACPI status and configuration";
    }

    /**
     * @brief Get command type
     * @return CommandType::Synchronous
     */
    CommandType getType() const override
    {
        return CommandType::Synchronous;
    }

    /**
     * @brief Execute the acpi-info command
     * @param context Command context with arguments
     * @return Status indicating success or failure
     */
    Status execute(const CommandContext& context) override
    {
        bool show_battery = false;
        bool show_adapter = false;
        bool show_thermal = false;
        bool show_all = true;

        // Parse arguments
        for (size_t i = 0; i < context.args.size(); ++i)
        {
            const std::string& arg = context.args[i];

            if (arg == "--help" || arg == "-h")
            {
                showHelp();
                return Status::ok();
            }
            else if (arg == "-b" || arg == "--battery")
            {
                show_battery = true;
                show_all = false;
            }
            else if (arg == "-a" || arg == "--adapter")
            {
                show_adapter = true;
                show_all = false;
            }
            else if (arg == "-t" || arg == "--thermal")
            {
                show_thermal = true;
                show_all = false;
            }
            else
            {
                return Status::error("Unknown option: " + arg +
                                     "\nUse --help for usage information");
            }
        }

        // If no specific options, show all
        if (show_all)
        {
            show_battery = true;
            show_adapter = true;
            show_thermal = true;
        }

        bool found_any = false;

        if (show_battery)
        {
            if (displayBatteryInfo())
                found_any = true;
        }

        if (show_adapter)
        {
            if (displayAdapterInfo())
                found_any = true;
        }

        if (show_thermal)
        {
            if (displayThermalInfo())
                found_any = true;
        }

        if (!found_any)
        {
            std::cout << "No ACPI information available on this system.\n";
            std::cout << "This may be a desktop system, VM, or ACPI is not enabled.\n";
        }

        return Status::ok();
    }

private:
    /**
     * @brief Display help information
     */
    void showHelp() const
    {
        std::cout << "Usage: acpi-info [OPTIONS]\n\n";
        std::cout << "Display ACPI (Advanced Configuration and Power Interface) information.\n\n";
        std::cout << "Options:\n";
        std::cout << "  -b, --battery    Show only battery information\n";
        std::cout << "  -a, --adapter    Show only AC adapter information\n";
        std::cout << "  -t, --thermal    Show only thermal information\n";
        std::cout << "  -h, --help       Show this help message\n\n";
        std::cout << "Without options, displays all available ACPI information.\n";
    }

    /**
     * @brief Read a single-line value from a sysfs file
     * @param path Path to the sysfs file
     * @return Content of the file, or empty string if not readable
     */
    std::string readSysfsValue(const std::string& path) const
    {
        std::ifstream file(path);
        if (!file.is_open())
            return "";

        std::string value;
        std::getline(file, value);

        // Trim whitespace
        size_t start = value.find_first_not_of(" \t\n\r");
        size_t end = value.find_last_not_of(" \t\n\r");

        if (start == std::string::npos)
            return "";

        return value.substr(start, end - start + 1);
    }

    /**
     * @brief Display battery information
     * @return true if battery information was found and displayed
     */
    bool displayBatteryInfo() const
    {
        const std::string power_supply_path = "/sys/class/power_supply/";

        DIR* dir = opendir(power_supply_path.c_str());
        if (!dir)
            return false;

        bool found_battery = false;
        struct dirent* entry;

        while ((entry = readdir(dir)) != nullptr)
        {
            if (entry->d_name[0] == '.')
                continue;

            std::string device_path = power_supply_path + entry->d_name + "/";
            std::string type = readSysfsValue(device_path + "type");

            if (type == "Battery")
            {
                if (!found_battery)
                {
                    std::cout << "=== Battery Information ===\n";
                    found_battery = true;
                }

                std::cout << "\nBattery: " << entry->d_name << "\n";

                // Status
                std::string status = readSysfsValue(device_path + "status");
                if (!status.empty())
                    std::cout << "  Status: " << status << "\n";

                // Capacity
                std::string capacity = readSysfsValue(device_path + "capacity");
                if (!capacity.empty())
                    std::cout << "  Capacity: " << capacity << "%\n";

                // Energy/Charge
                std::string energy_now = readSysfsValue(device_path + "energy_now");
                std::string energy_full = readSysfsValue(device_path + "energy_full");
                std::string charge_now = readSysfsValue(device_path + "charge_now");
                std::string charge_full = readSysfsValue(device_path + "charge_full");

                if (!energy_now.empty() && !energy_full.empty())
                {
                    double now_mwh = std::stod(energy_now) / 1000000.0;
                    double full_mwh = std::stod(energy_full) / 1000000.0;
                    std::cout << "  Energy: " << std::fixed << std::setprecision(2) << now_mwh
                              << " / " << full_mwh << " Wh\n";
                }
                else if (!charge_now.empty() && !charge_full.empty())
                {
                    double now_mah = std::stod(charge_now) / 1000.0;
                    double full_mah = std::stod(charge_full) / 1000.0;
                    std::cout << "  Charge: " << std::fixed << std::setprecision(2) << now_mah
                              << " / " << full_mah << " mAh\n";
                }

                // Voltage
                std::string voltage = readSysfsValue(device_path + "voltage_now");
                if (!voltage.empty())
                {
                    double voltage_v = std::stod(voltage) / 1000000.0;
                    std::cout << "  Voltage: " << std::fixed << std::setprecision(2) << voltage_v
                              << " V\n";
                }

                // Current
                std::string current = readSysfsValue(device_path + "current_now");
                if (!current.empty())
                {
                    double current_ma = std::stod(current) / 1000.0;
                    std::cout << "  Current: " << std::fixed << std::setprecision(2) << current_ma
                              << " mA\n";
                }

                // Power
                std::string power = readSysfsValue(device_path + "power_now");
                if (!power.empty())
                {
                    double power_w = std::stod(power) / 1000000.0;
                    std::cout << "  Power: " << std::fixed << std::setprecision(2) << power_w
                              << " W\n";
                }

                // Technology
                std::string technology = readSysfsValue(device_path + "technology");
                if (!technology.empty())
                    std::cout << "  Technology: " << technology << "\n";

                // Cycle count
                std::string cycle_count = readSysfsValue(device_path + "cycle_count");
                if (!cycle_count.empty())
                    std::cout << "  Cycle Count: " << cycle_count << "\n";

                // Health
                std::string energy_full_design = readSysfsValue(device_path + "energy_full_design");
                std::string charge_full_design = readSysfsValue(device_path + "charge_full_design");

                if (!energy_full.empty() && !energy_full_design.empty())
                {
                    double health =
                        (std::stod(energy_full) / std::stod(energy_full_design)) * 100.0;
                    std::cout << "  Health: " << std::fixed << std::setprecision(1) << health
                              << "%\n";
                }
                else if (!charge_full.empty() && !charge_full_design.empty())
                {
                    double health =
                        (std::stod(charge_full) / std::stod(charge_full_design)) * 100.0;
                    std::cout << "  Health: " << std::fixed << std::setprecision(1) << health
                              << "%\n";
                }
            }
        }

        closedir(dir);

        if (found_battery)
            std::cout << "\n";

        return found_battery;
    }

    /**
     * @brief Display AC adapter information
     * @return true if adapter information was found and displayed
     */
    bool displayAdapterInfo() const
    {
        const std::string power_supply_path = "/sys/class/power_supply/";

        DIR* dir = opendir(power_supply_path.c_str());
        if (!dir)
            return false;

        bool found_adapter = false;
        struct dirent* entry;

        while ((entry = readdir(dir)) != nullptr)
        {
            if (entry->d_name[0] == '.')
                continue;

            std::string device_path = power_supply_path + entry->d_name + "/";
            std::string type = readSysfsValue(device_path + "type");

            if (type == "Mains" || type == "USB" || type == "USB_PD")
            {
                if (!found_adapter)
                {
                    std::cout << "=== AC Adapter Information ===\n";
                    found_adapter = true;
                }

                std::cout << "\nAdapter: " << entry->d_name << " (" << type << ")\n";

                std::string online = readSysfsValue(device_path + "online");
                if (!online.empty())
                {
                    std::cout << "  Status: " << (online == "1" ? "Online" : "Offline") << "\n";
                }
            }
        }

        closedir(dir);

        if (found_adapter)
            std::cout << "\n";

        return found_adapter;
    }

    /**
     * @brief Display thermal zone information
     * @return true if thermal information was found and displayed
     */
    bool displayThermalInfo() const
    {
        const std::string thermal_path = "/sys/class/thermal/";

        DIR* dir = opendir(thermal_path.c_str());
        if (!dir)
            return false;

        bool found_thermal = false;
        struct dirent* entry;
        std::vector<std::pair<std::string, std::string>> thermal_zones;

        while ((entry = readdir(dir)) != nullptr)
        {
            std::string name = entry->d_name;

            if (name.find("thermal_zone") == 0)
            {
                std::string zone_path = thermal_path + name + "/";
                std::string temp_str = readSysfsValue(zone_path + "temp");
                std::string type = readSysfsValue(zone_path + "type");

                if (!temp_str.empty())
                {
                    thermal_zones.push_back({name, type});

                    if (!found_thermal)
                    {
                        std::cout << "=== Thermal Information ===\n";
                        found_thermal = true;
                    }

                    double temp_c = std::stod(temp_str) / 1000.0;

                    std::cout << "\nThermal Zone: " << name;
                    if (!type.empty())
                        std::cout << " (" << type << ")";
                    std::cout << "\n";

                    std::cout << "  Temperature: " << std::fixed << std::setprecision(1) << temp_c
                              << " °C\n";

                    // Try to read trip points
                    for (int i = 0; i < 5; ++i)
                    {
                        std::string trip_temp =
                            readSysfsValue(zone_path + "trip_point_" + std::to_string(i) + "_temp");
                        std::string trip_type =
                            readSysfsValue(zone_path + "trip_point_" + std::to_string(i) + "_type");

                        if (!trip_temp.empty() && !trip_type.empty())
                        {
                            double trip_c = std::stod(trip_temp) / 1000.0;
                            if (i == 0)
                                std::cout << "  Trip Points:\n";
                            std::cout << "    " << trip_type << ": " << std::fixed
                                      << std::setprecision(1) << trip_c << " °C\n";
                        }
                    }
                }
            }
        }

        closedir(dir);

        if (found_thermal)
            std::cout << "\n";

        return found_thermal;
    }
};

} // namespace homeshell
