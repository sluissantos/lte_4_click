#include "lte.h"
#include <stdio.h>
#include <ncs_version.h>
#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>
#include <zephyr/logging/log.h>
#include <modem/nrf_modem_lib.h>
#include <modem/lte_lc.h>

/* Semaphore for LTE connection */
static K_SEM_DEFINE(lte_connected, 0, 1);

LOG_MODULE_REGISTER(lte, LOG_LEVEL_INF);

/**
 * @brief LTE event handler
 *
 * This function is called by the modem on LTE events.
 *
 * @param evt LTE event
 */
static void lte_handler(const struct lte_lc_evt *const evt) {
     switch (evt->type) {
     case LTE_LC_EVT_NW_REG_STATUS:
        if ((evt->nw_reg_status != LTE_LC_NW_REG_REGISTERED_HOME) &&
            (evt->nw_reg_status != LTE_LC_NW_REG_REGISTERED_ROAMING)) {
            break;
        }
		LOG_INF("Network registration status: %s",
				evt->nw_reg_status == LTE_LC_NW_REG_REGISTERED_HOME ?
				"Connected - home network" : "Connected - roaming");
		k_sem_give(&lte_connected);
        break;
	case LTE_LC_EVT_RRC_UPDATE:
		LOG_INF("RRC mode: %s", evt->rrc_mode == LTE_LC_RRC_MODE_CONNECTED ?
				"Connected" : "Idle");
		break;
     default:
             break;
     }
}

/**
 * @brief Configure the modem and connect to the LTE network.
 *
 * This function initializes the modem library and the LTE link control library,
 * and then connects to the LTE network using the lte_lc_connect_async function.
 * It waits for the connection to complete using a semaphore.
 *
 * @return 0 if successful, a negative value if an error occurred.
 */
void lteInit(void) {
    LOG_INF("lteInit ..");

	int err;

	err = nrf_modem_lib_init();
	if (err) {
		LOG_ERR("Failed to initialize the modem library, error: %d", err);
		return;
	}

	lte_lc_modem_events_enable();

	/* Set modem to LTE-M only */
	err = lte_lc_system_mode_set(LTE_LC_SYSTEM_MODE_LTEM, LTE_LC_SYSTEM_MODE_PREFER_LTEM);
	if (err) {
		LOG_ERR("Failed to set LTE system mode, error: %d", err);
		return;
	}

	err = lte_lc_connect_async(lte_handler);
	if (err) {
		LOG_ERR("Error in lte_lc_connect_async, error: %d", err);
		return;
	}

	k_sem_take(&lte_connected, K_FOREVER);

	return;
}
