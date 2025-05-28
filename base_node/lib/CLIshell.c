#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/time_units.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_backend.h>
#include "CLIshell.h"
#include "localVariables.h"
#include "servo.h"

static int read_sensor_data(const struct shell *shell, size_t argc, char **argv) {
    shell_print(shell, "ultrasonic: %d", latest_distance_cm);
    shell_print(shell, "magnetometer: %d", latest_avg_value);
    if (door_locked) {
        shell_print(shell, "Door is locked");
    } else {
        shell_print(shell, "Door is unlocked");
    }
    return 0;
}
static int door(const struct shell *shell, size_t argc, char **argv) {
    if (argc != 2) {
        shell_error(shell, "Usage: door <lock|unlock>");
        return -EINVAL;
    }

    if (strcmp(argv[1], "lock") == 0) {
        if (door_locked) {
            shell_print(shell, "Door is already locked");
        } else {
            set_servo_locked(true);
            door_locked = true;
            shell_print(shell, "Door is now locked");
        }
    } else if (strcmp(argv[1], "unlock") == 0) {
        if (!door_locked) {
            shell_print(shell, "Door is already unlocked");
        } else {
            set_servo_locked(false);
            door_locked = false;
            shell_print(shell, "Door is now unlocked");
        }
    } else {
        shell_error(shell, "Unknown command: %s", argv[1]);
        return -EINVAL;
    }
    return 0;

}

void register_shell_commands(void) {
    SHELL_CMD_REGISTER(status, NULL, "Onboard led controll", read_sensor_data);
    SHELL_CMD_REGISTER(door, NULL, "Door control", door);
}

