/* main.c - Application main entry point */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
LOG_MODULE_REGISTER(raw_ipsp);

#include <zephyr.h>
#include <errno.h>
#include <stdio.h>

#include <net/socket.h>
#include <net/net_mgmt.h>


#define PEER_IPV6_ADDR "2001:db8::2"
#define PEER_PORT       8081
#define UDP_CHUNK_SIZE  256
static int udp_sock;
static uint8_t garbage[UDP_CHUNK_SIZE];

/* Wait for nw */
static struct net_mgmt_event_callback mgmt_cb;
K_SEM_DEFINE(sem_network_up, 0, 1);

static void net_event_handler(struct net_mgmt_event_callback *cb,
			      uint32_t mgmt_event, struct net_if *iface)
{
	if (mgmt_event == NET_EVENT_L4_CONNECTED) {
		LOG_INF("Network connected");
		k_sem_give(&sem_network_up);
		return;
	}

}


void spam(void)
{
	int ret;
	struct sockaddr_in6 addr = {
		.sin6_family = AF_INET6,
		.sin6_port = PEER_PORT,
	};
	inet_pton(AF_INET6, PEER_IPV6_ADDR, &addr.sin6_addr);


	udp_sock = socket(addr.sin6_family, SOCK_DGRAM, IPPROTO_UDP);
	if (udp_sock < 0) {
		LOG_ERR("SOCKET FAIL");
		k_panic();
	}

	ret = connect(udp_sock, (struct sockaddr *)&addr, sizeof(addr));
	if (ret < 0) {
		LOG_ERR("CONNECT FAIL");
		k_panic();
	}

	u64_t sent = 0;
	while (true) {
		k_yield();
		ret = send(udp_sock, garbage, sizeof(garbage), 0);
		if (ret < 0) {
			LOG_ERR("send error (%d)", errno);
			continue;
		}
		sent += ret;
	}
}

int main(void)
{
	LOG_INF("Waiting for network...");

	net_mgmt_init_event_callback(&mgmt_cb, net_event_handler,
				     NET_EVENT_L4_CONNECTED |
				     NET_EVENT_L4_DISCONNECTED);
	net_mgmt_add_event_callback(&mgmt_cb);

	/* k_sem_take(&sem_network_up, K_FOREVER); */

	spam();
}
