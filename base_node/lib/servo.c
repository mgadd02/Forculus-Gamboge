#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <zephyr/drivers/pwm.h>
#include <stdbool.h>

#define PWM_PERIOD_US       20000U      // 20 ms
#define LOCKED_PULSE_US      700U       // 0.7 ms pulse for locked
#define UNLOCKED_PULSE_US   2500U       // 2.5 ms pulse for unlocked

static const struct pwm_dt_spec servo_pwm = PWM_DT_SPEC_GET(DT_ALIAS(pwm_servo));

void set_servo_locked(bool locked)
{
    if (!pwm_is_ready_dt(&servo_pwm)) {
        printk("PWM device %s not ready\n", servo_pwm.dev->name);
        return;
    }

    uint32_t pulse = locked ? LOCKED_PULSE_US : UNLOCKED_PULSE_US;

    int ret = pwm_set_dt(&servo_pwm,
                         PWM_USEC(PWM_PERIOD_US),
                         PWM_USEC(pulse));

    if (ret < 0) {
        printk("Failed to move servo to %s position (err %d)\n",
               locked ? "locked" : "unlocked", ret);
    } else {
        printk("Servo moved to %s position\n", locked ? "locked" : "unlocked");
    }
}

