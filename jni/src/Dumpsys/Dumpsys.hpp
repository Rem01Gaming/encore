#include <cerrno>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

struct RecentAppList {
    std::string package_name;
    bool visible;
};

struct DumpsysWindowDisplays {
    bool screen_awake;
    std::vector<RecentAppList> recent_app;
};

struct DumpsysPower {
    bool screen_awake;
    bool is_plugged;
    bool battery_saver;
    bool battery_saver_sticky;
};

namespace Dumpsys {

/**
 * @brief Retrieves window display information by parsing `dumpsys window displays`.
 *
 * This function executes `dumpsys window displays` and parses its output to determine
 * the screen's awake state and a list of recent applications.
 *
 * @param result A reference to a DumpsysWindowDisplays struct to store the parsed information.
 * @throws std::runtime_error if popen fails or if required information cannot be found in the dumpsys output.
 */
void WindowDisplays(DumpsysWindowDisplays &result);

/**
 * @brief Retrieves power-related information by parsing `dumpsys power`.
 *
 * This function executes `dumpsys power` and parses its output to determine
 * the screen's awake state, whether the device is plugged in, and the status
 * of battery saver modes.
 *
 * @param result A reference to a DumpsysPower struct to store the parsed information.
 * @throws std::runtime_error if popen fails or if required information cannot be found in the dumpsys output.
 */
void Power(DumpsysPower &result);

/**
 * @brief Gets the Process ID (PID) of an application by its package name.
 *
 * This function executes `dumpsys activity top` and searches for the line
 * corresponding to the given package name to extract its PID.
 *
 * @param package_name The package name of the application to find.
 * @return The PID of the application if found.
 * @throws std::runtime_error if popen fails, the package is not found, or the PID cannot be extracted.
 */
pid_t GetAppPID(const std::string &package_name);

} // namespace Dumpsys
