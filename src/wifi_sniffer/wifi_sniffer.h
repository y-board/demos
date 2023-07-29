#ifndef WIFI_SNIFFER_H
#define WIFI_SNIFFER_H

#include "Arduino.h"
#include "ybadge.h"
#include <stdint.h>

void wifi_sniffer_init();
void wifi_sniffer_loop();

void wifi_sniffer_rx_packet(void *buf, wifi_promiscuous_pkt_type_t type);
void wifi_sniffer_sniff_action();

#endif /* WIFI_SNIFFER_H */
