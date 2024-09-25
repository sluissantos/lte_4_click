#pragma once

#include <stdio.h>
#include <string.h>
#include <ncs_version.h>

#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/mqtt.h>

#include <modem/nrf_modem_lib.h>

#if NCS_VERSION_NUMBER < 0x20600
#include <zephyr/random/rand32.h>
#else 
#include <zephyr/random/random.h>
#endif

#define IMEI_LEN 15
#define CGSN_RESPONSE_LENGTH (IMEI_LEN + 6 + 1)
#define CLIENT_ID_LEN sizeof("nrf-") + IMEI_LEN

/**@brief Initialize the MQTT client structure
 */
int mqttClientInit(struct mqtt_client *client);

/**@brief Initialize the file descriptor structure used by poll.
 */
int mqttFdsInit(struct mqtt_client *c, struct pollfd *fds);

/**@brief Function to publish data on the configured topic
 */
int mqttDatapPublish(struct mqtt_client *c, enum mqtt_qos qos,
	uint8_t *data, size_t len);

/**@brief Function to init mqtt
 */
void mqttInit(void);

void mqttConnectionThread(void *p1, void *p2, void *p3);