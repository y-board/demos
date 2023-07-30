#ifndef WIFI_SNIFFER_H
#define WIFI_SNIFFER_H

#include "Arduino.h"
#include "ybadge.h"
#include <stdint.h>

// Required functions
void wifi_sniffer_init();
void wifi_sniffer_loop();

// Callback for when a packet is received
void wifi_sniffer_rx_packet(void *buf, wifi_promiscuous_pkt_type_t type);

// State machine states
void sniff_state();
void adjust_channel_state();
void increase_channel_state();
void decrease_channel_state();

#endif /* WIFI_SNIFFER_H */
