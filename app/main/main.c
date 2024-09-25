#include <stdio.h>
#include <ncs_version.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <zephyr/drivers/hwinfo.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/reboot.h>

#include "lte.h"
#include "mqtt.h"

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

static atomic_t read_reset_cause = ATOMIC_INIT(0);

static volatile uint32_t reset_cause = 0;
static volatile int32_t reset_error = 0;

uint32_t appl_reset_cause(int *flags, uint16_t *reboot_code) {
    uint32_t cause = 0;

    if (atomic_cas(&read_reset_cause, 0, 1)) {
        reset_error = hwinfo_get_reset_cause(&cause);
        if (reset_error == 0 && cause) {
            LOG_INF("Current reset cause: %08x", cause);

            if (cause & RESET_PIN) {
                LOG_INF("RESET_PIN");
            }
            if (cause & RESET_SOFTWARE) {
                LOG_INF("RESET_SOFTWARE");
            }
            if (cause & RESET_BROWNOUT) {
                LOG_INF("RESET_BROWNOUT");
            }
            if (cause & RESET_POR) {
                LOG_INF("RESET_POR");
            }
            if (cause & RESET_WATCHDOG) {
                LOG_INF("WATCHDOG");
            }
            if (cause & RESET_DEBUG) {
                LOG_INF("DEBUG");
            }
            if (cause & RESET_SECURITY) {
                LOG_INF("RESET_SECURITY");
            }
            if (cause & RESET_LOW_POWER_WAKE) {
                LOG_INF("LOWPOWER");
            }
            if (cause & RESET_CPU_LOCKUP) {
                LOG_INF("CPU");
            }
            if (cause & RESET_PARITY) {
                LOG_INF("RESET_PARITY");
            }
            if (cause & RESET_PLL) {
                LOG_INF("RESET_PLL");
            }
            if (cause & RESET_CLOCK) {
                LOG_INF("RESET_CLOCK");
            }
            if (cause & RESET_HARDWARE) {
                LOG_INF("RESET_HARDWARE");
            }
            if (cause & RESET_USER) {
                LOG_INF("RESET_USER");
            }
            if (cause & RESET_TEMPERATURE) {
                LOG_INF("RESET_TEMPERATURE");
            }
        } else {
            LOG_INF("No reset cause or error reading reset cause (error: %d)", reset_error);
        }
    }
    
    return cause;
}

int main(void) {

	int reset_cause = 0;
	uint16_t reboot_cause = 0;

	appl_reset_cause(&reset_cause, &reboot_cause);

	k_sleep(K_MSEC(100));

	lteInit();	

	k_sleep(K_MSEC(100));

	mqttInit();

	return 0;
}