#include "mqtt.h"

/* Buffers for MQTT client. */
static uint8_t rx_buffer[CONFIG_MQTT_MESSAGE_BUFFER_SIZE];
static uint8_t tx_buffer[CONFIG_MQTT_MESSAGE_BUFFER_SIZE];
static uint8_t payload_buf[CONFIG_MQTT_PAYLOAD_BUFFER_SIZE];

/* MQTT client instance. */
static struct mqtt_client client;

/* MQTT Broker details. */
static struct sockaddr_storage broker;

/* File descriptor structure used by poll. */
static struct pollfd fds;

/* Stack size and priority for the MQTT connection thread */
#define MQTT_CONNECTION_THREAD_STACK_SIZE 2048
#define MQTT_CONNECTION_THREAD_PRIORITY 6

K_THREAD_STACK_DEFINE(mqttConnection_Stack, MQTT_CONNECTION_THREAD_STACK_SIZE);
struct k_thread mqttConnection_Thread;

/* Stack size and priority for the MQTT publisher thread */
#define MQTT_PUBLISHER_THREAD_STACK_SIZE 1024
#define MQTT_PUBLISHER_THREAD_PRIORITY 7

/* Thread definition */
static void mqttPublishThread(void);

/* Create the thread */
K_THREAD_DEFINE(mqttPublish_Thread, MQTT_PUBLISHER_THREAD_STACK_SIZE,
                mqttPublishThread, NULL, NULL, NULL,
                MQTT_PUBLISHER_THREAD_PRIORITY, 0, 0);

LOG_MODULE_REGISTER(mqtt, LOG_LEVEL_INF);

static bool mqtt_connected = false;
K_MUTEX_DEFINE(mqtt_mutex);

/**
 * @brief Reads the received payload from the MQTT server.
 *
 * @param c      MQTT client instance.
 * @param length Length of the payload to read.
 *
 * @return 0 on success, negative errno on error.
 *
 * @details If the payload is larger than the payload buffer, it is truncated to
 *          fit into the buffer. If the payload is longer than the buffer, it
 *          is read in chunks of the buffer size until it fits. The function
 *          will return -EMSGSIZE if the payload is larger than the buffer, or
 *          -EIO if there is an error reading the payload.
 */
static int mqttGetReceivedPayload(struct mqtt_client *c, size_t length) {
	int ret;
	int err = 0;

	/* Return an error if the payload is larger than the payload buffer.
	 * Note: To allow new messages, we have to read the payload before returning.
	 */
	if (length > sizeof(payload_buf)) {
		err = -EMSGSIZE;
	}

	/* Truncate payload until it fits in the payload buffer. */
	while (length > sizeof(payload_buf)) {
		ret = mqtt_read_publish_payload_blocking(
				c, payload_buf, (length - sizeof(payload_buf)));
		if (ret == 0) {
			return -EIO;
		} else if (ret < 0) {
			return ret;
		}

		length -= ret;
	}

	ret = mqtt_readall_publish_payload(c, payload_buf, length);
	if (ret) {
		return ret;
	}

	return err;
}

/**
 * @brief Subscribe to a topic.
 *
 * @param c The MQTT client instance.
 *
 * @return 0 on success, negative error code on failure.
 *
 * @details This function subscribes to the topic specified in the
 *          CONFIG_MQTT_SUB_TOPIC setting. 
 *
 *          The function will return 0 on success, or a negative error
 *          code on failure.
 */
static int mqttSubscribe(struct mqtt_client *const c) {
	struct mqtt_topic subscribe_topic = {
		.topic = {
			.utf8 = CONFIG_MQTT_SUB_TOPIC,
			.size = strlen(CONFIG_MQTT_SUB_TOPIC)
		},
		.qos = MQTT_QOS_1_AT_LEAST_ONCE
	};

	const struct mqtt_subscription_list subscription_list = {
		.list = &subscribe_topic,
		.list_count = 1,
		.message_id = 1234
	};

	LOG_INF("Subscribing on \"%s\"", CONFIG_MQTT_SUB_TOPIC);

	return mqtt_subscribe(c, &subscription_list);
}

/**
 * @brief Print a buffer to the log as a string.
 *
 * @param prefix A string to print before the buffer.
 * @param data   The buffer to print.
 * @param len    The length of the buffer.
 *
 * @details The buffer is null-terminated and the resulting string is
 *          printed using LOG_INF.
 */
static void mqttDataPrint(uint8_t *prefix, uint8_t *data, size_t len, uint8_t* topic) {
	char buf[len + 1];

	memcpy(buf, data, len);
	buf[len] = 0;
	LOG_INF("%s\"%s\" on \"%s\"", (char *)prefix, (char *)buf, topic);
}

/**
 * @brief Publish a message to an MQTT topic.
 *
 * @param c      MQTT client instance.
 * @param qos    QOS level of the message.
 * @param data   Buffer containing the payload.
 * @param len    Length of the payload buffer.
 *
 * @return 0 on success, negative error code on failure.
 *
 * @details Publish a message to the topic specified in the
 *          CONFIG_MQTT_PUB_TOPIC setting. The QOS level of the message
 *          is set to @p qos. The payload is taken from the buffer
 *          @p data, with length @p len.
 *
 *          The function will also return -EIO if there is an error
 *          reading the payload.
 */
int mqttDataPublish(struct mqtt_client *c, enum mqtt_qos qos,
	uint8_t *data, size_t len) {
	struct mqtt_publish_param param;

	param.message.topic.qos = qos;
	param.message.topic.topic.utf8 = CONFIG_MQTT_PUB_TOPIC;
	param.message.topic.topic.size = strlen(CONFIG_MQTT_PUB_TOPIC);
	param.message.payload.data = data;
	param.message.payload.len = len;
	param.message_id = sys_rand32_get();
	param.dup_flag = 0;
	param.retain_flag = 0;

	mqttDataPrint("Publishing ", data, len, CONFIG_MQTT_PUB_TOPIC);

	return mqtt_publish(c, &param);
}

/**
 * @brief MQTT event handler.
 *
 * This function is called by the MQTT client API when an event occurs.
 *
 * @param c MQTT client instance.
 * @param evt MQTT event structure.
 */
void mqttEvtHandler(struct mqtt_client *const c, const struct mqtt_evt *evt) {
	int err;

	switch (evt->type) {
		case MQTT_EVT_CONNACK:
			if (evt->result != 0) {
				LOG_ERR("MQTT connect failed: %d", evt->result);
				break;
			}

			LOG_INF("MQTT client connected");

			k_mutex_lock(&mqtt_mutex, K_FOREVER);
			mqtt_connected = true;
			k_mutex_unlock(&mqtt_mutex);

			mqttSubscribe(c);

			break;

		case MQTT_EVT_DISCONNECT:
			LOG_INF("MQTT client disconnected: %d", evt->result);

			k_mutex_lock(&mqtt_mutex, K_FOREVER);
			mqtt_connected = false;
			k_mutex_unlock(&mqtt_mutex);

			break;

		case MQTT_EVT_PUBLISH: {
			const struct mqtt_publish_param *p = &evt->param.publish;
			LOG_INF("MQTT PUBLISH result=%d",
				evt->result);

			err = mqttGetReceivedPayload(c, p->message.payload.len);
			
			//Send acknowledgment to the broker on receiving QoS1 publish message 
			if (p->message.topic.qos == MQTT_QOS_1_AT_LEAST_ONCE) {
				const struct mqtt_puback_param ack = {
					.message_id = p->message_id
				};
				/* Send acknowledgment. */
				mqtt_publish_qos1_ack(c, &ack);
			}

			if (err >= 0) {
				mqttDataPrint("Received: ", payload_buf, p->message.payload.len, (uint8_t *)p->message.topic.topic.utf8);
			// Payload buffer is smaller than the received data 
			} else if (err == -EMSGSIZE) {
				LOG_ERR("Received payload (%d bytes) is larger than the payload buffer size (%d bytes).",
					p->message.payload.len, sizeof(payload_buf));
			// Failed to extract data, disconnect 
			} else {
				LOG_ERR("get_received_payload failed: %d", err);
				LOG_INF("Disconnecting MQTT client...");

				err = mqtt_disconnect(c);
				if (err) {
					LOG_ERR("Could not disconnect: %d", err);
				}
			}
		} break;

		case MQTT_EVT_PUBACK:
			if (evt->result != 0) {
				LOG_ERR("MQTT PUBACK error: %d", evt->result);
				break;
			}
			//LOG_INF("PUBACK packet id: %u", evt->param.puback.message_id);
			break;

		case MQTT_EVT_SUBACK:
			if (evt->result != 0) {
				LOG_ERR("MQTT SUBACK error: %d", evt->result);
				break;
			}

			LOG_INF("SUBACK packet id: %u", evt->param.suback.message_id);
			break;

		case MQTT_EVT_PINGRESP:
			if (evt->result != 0) {
				LOG_ERR("MQTT PINGRESP error: %d", evt->result);
			}
			break;

		default:
			LOG_INF("Unhandled MQTT event type: %d", evt->type);
			break;
	}
}

/**
 * @brief Initialize the MQTT broker address.
 *
 * @details This function resolves the hostname of the MQTT broker using
 *          getaddrinfo() and sets the address of the broker in the global
 *          'broker' variable.
 *
 * @return 0 on success, or a negative error code on failure.
 */
static int mqtt_broker_init(void) {
	int err;
	struct addrinfo *result;
	struct addrinfo *addr;
	struct addrinfo hints = {
		.ai_family = AF_INET,
		.ai_socktype = SOCK_STREAM
	};

	err = getaddrinfo(CONFIG_MQTT_BROKER_HOSTNAME, NULL, &hints, &result);
	if (err) {
		LOG_ERR("getaddrinfo failed: %d", err);
		return -ECHILD;
	}

	addr = result;

	/* Look for address of the broker. */
	while (addr != NULL) {
		/* IPv4 Address. */
		if (addr->ai_addrlen == sizeof(struct sockaddr_in)) {
			struct sockaddr_in *broker4 =
				((struct sockaddr_in *)&broker);
			char ipv4_addr[NET_IPV4_ADDR_LEN];

			broker4->sin_addr.s_addr =
				((struct sockaddr_in *)addr->ai_addr)
				->sin_addr.s_addr;
			broker4->sin_family = AF_INET;
			broker4->sin_port = htons(CONFIG_MQTT_BROKER_PORT);

			inet_ntop(AF_INET, &broker4->sin_addr.s_addr,
				  ipv4_addr, sizeof(ipv4_addr));
			LOG_INF("IPv4 Address found %s", (char *)(ipv4_addr));

			break;
		} else {
			LOG_ERR("ai_addrlen = %u should be %u or %u",
				(unsigned int)addr->ai_addrlen,
				(unsigned int)sizeof(struct sockaddr_in),
				(unsigned int)sizeof(struct sockaddr_in6));
		}

		addr = addr->ai_next;
	}

	/* Free the address. */
	freeaddrinfo(result);

	return err;
}

/**
 * @brief Get the client id to use for the MQTT connection.
 *
 * @details If CONFIG_MQTT_CLIENT_ID is set, that value is used.
 *          Otherwise, the function attempts to obtain the IMEI of the device
 *          using the AT+CGSN command and generates a client id string
 *          of the form "nrf-<imei>".
 *
 * @return The client id to use for the MQTT connection.
 */
static const uint8_t* mqttClientIdGet(void) {
	static uint8_t client_id[MAX(sizeof(CONFIG_MQTT_CLIENT_ID),
				     CLIENT_ID_LEN)];

	if (strlen(CONFIG_MQTT_CLIENT_ID) > 0) {
		snprintf(client_id, sizeof(client_id), "%s",
			 CONFIG_MQTT_CLIENT_ID);
		goto exit;
	}

	char imei_buf[CGSN_RESPONSE_LENGTH + 1];
	int err;

	err = nrf_modem_at_cmd(imei_buf, sizeof(imei_buf), "AT+CGSN");
	if (err) {
		LOG_ERR("Failed to obtain IMEI, error: %d", err);
		goto exit;
	}

	imei_buf[IMEI_LEN] = '\0';

	snprintf(client_id, sizeof(client_id), "nrf-%.*s", IMEI_LEN, imei_buf);
	LOG_INF("client_id = %s", (char *)(client_id));

exit:
	LOG_DBG("client_id = %s", (char *)(client_id));

	return client_id;
}

/**
 * @brief Initialize the MQTT client
 *
 * This function initializes the MQTT client instance. It resolves the configured
 * hostname and initializes the MQTT broker structure. It configures the MQTT
 * client with the broker details and the event handler. It also configures the
 * MQTT buffers and the transport type.
 *
 * @param client MQTT client instance to be initialized
 *
 * @return 0 on success, negative error code on failure
 */
int mqttClientInit(struct mqtt_client *client) {
	int err;
	/* Initializes the client instance. */
	mqtt_client_init(client);

	/* Resolves the configured hostname and initializes the MQTT broker structure */
	err = mqtt_broker_init();
	if (err) {
		LOG_ERR("Failed to initialize broker connection");
		return err;
	}

	/* MQTT client configuration */
	client->broker = &broker;
	client->evt_cb = mqttEvtHandler;
	client->client_id.utf8 = mqttClientIdGet();
	client->client_id.size = strlen(client->client_id.utf8);
	client->password = NULL;
	client->user_name = NULL;
	client->protocol_version = MQTT_VERSION_3_1_1;

	/* MQTT buffers configuration */
	client->rx_buf = rx_buffer;
	client->rx_buf_size = sizeof(rx_buffer);
	client->tx_buf = tx_buffer;
	client->tx_buf_size = sizeof(tx_buffer);

	/* We are not using TLS in Exercise 1 */
	client->transport.type = MQTT_TRANSPORT_NON_SECURE;

	return err;
}


/**
 * @brief Initialize pollfd for MQTT client
 *
 * @param c MQTT client instance
 * @param fds pollfd structure to be initialized
 *
 * @return 0 on success, -ENOTSUP if MQTT client is configured to use TLS
 *
 * This function initializes the pollfd structure with the file descriptor
 * of the MQTT client's TCP socket. The events are set to POLLIN.
 */
int mqttFdsInit(struct mqtt_client *c, struct pollfd *fds) {
	if (c->transport.type == MQTT_TRANSPORT_NON_SECURE) {
		fds->fd = c->transport.tcp.sock;
	} else {
		return -ENOTSUP;
	}

	fds->events = POLLIN;

	return 0;
}

/**
 * @brief Initialize the MQTT client and start the main loop.
 *
 * This function initializes the MQTT client and starts the main loop where it
 * will connect to the broker, wait for incoming data, and send keepalive
 * messages. If the connection is lost, it will try to reconnect.
 *
 * @note If the function returns, it means that an error occurred.
 */
void mqttInit(void) {
	LOG_INF("mqttInit ..");
	int err;

	err = mqttClientInit(&client);
	if (err) {
		LOG_ERR("Failed to initialize MQTT client: %d", err);
		return;
	}

    k_thread_create(&mqttConnection_Thread, mqttConnection_Stack, MQTT_CONNECTION_THREAD_STACK_SIZE,
                    mqttConnectionThread, NULL, NULL, NULL,
                    MQTT_CONNECTION_THREAD_PRIORITY, 0, K_NO_WAIT);
}

void mqttConnectionThread(void *p1, void *p2, void *p3) {

	int err;
	uint16_t connect_attempt = 0;

	while (1) {
		do_connect:
			if (connect_attempt++ > 0) {
				LOG_INF("Reconnecting in %d seconds...",
					CONFIG_MQTT_RECONNECT_DELAY_S);
				k_sleep(K_SECONDS(CONFIG_MQTT_RECONNECT_DELAY_S));
			}

			LOG_INF("Connection to broker using mqtt_connect");
			err = mqtt_connect(&client);
			if (err) {
				LOG_ERR("Error in mqtt_connect: %d", err);
				goto do_connect;
			}

			err = mqttFdsInit(&client,&fds);
			if (err) {
				LOG_ERR("Error in mqttFdsInit: %d", err);
				return;
			}

			while (1) {
				err = poll(&fds, 1, mqtt_keepalive_time_left(&client));
				if (err < 0) {
					LOG_ERR("Error in poll(): %d", errno);
					break;
				}

				err = mqtt_live(&client);
				if ((err != 0) && (err != -EAGAIN)) {
					LOG_ERR("Error in mqtt_live: %d", err);
					break;
				}

				if ((fds.revents & POLLIN) == POLLIN) {
					err = mqtt_input(&client);
					if (err != 0) {
						LOG_ERR("Error in mqtt_input: %d", err);
						break;
					}
				}

				if ((fds.revents & POLLERR) == POLLERR) {
					LOG_ERR("POLLERR");
					break;
				}

				if ((fds.revents & POLLNVAL) == POLLNVAL) {
					LOG_ERR("POLLNVAL");
					break;
				}
			}

			LOG_INF("Disconnecting MQTT client");

			err = mqtt_disconnect(&client);
			if (err) {
				LOG_ERR("Could not disconnect MQTT client: %d", err);
			}
		k_sleep(K_MSEC(50));
		goto do_connect;
	}
}

/**
 * @brief Thread to publish a message periodically to the configured topic.
 *
 * @details This function is run in a separate thread and will publish a message
 *          to the configured topic every n seconds, if the MQTT client is
 *          connected.
 */
static void mqttPublishThread(void) {
    while (1) {
        k_mutex_lock(&mqtt_mutex, K_FOREVER);
        if (mqtt_connected) {
            char status[] = "1";
            int err = mqttDataPublish(&client, MQTT_QOS_1_AT_LEAST_ONCE,
                                      status, sizeof(status)-1);
            if (err) {
                LOG_ERR("Failed to publish message: %d", err);
            }
        }
        k_mutex_unlock(&mqtt_mutex);

        k_sleep(K_SECONDS(CONFIG_MQTT_PUBLISH_PERIOD_S));
    }
}