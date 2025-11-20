// This file was created by takkaO by reverse engineering the original file and using "Clean room design" techniques with ChatGPT.

/*************************************************************************
Copyright (C) 2025 takkaO

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*************************************************************************/

#ifndef WIFIGENERICV2_H
#define WIFIGENERICV2_H

#include "seeed_rpcUnified.h"
#include "rtl_wifi/wifi_unified.h"
#undef max
#undef min
#include <functional>
#include "WiFiType.h"

typedef void (*WiFiEventCb)(system_event_id_t event);
typedef std::function<void(system_event_id_t, system_event_info_t)> WiFiEventFuncCb;
typedef void (*WiFiEventSysCb)(system_event_t *);
typedef size_t wifi_event_id_t;
typedef enum {
    WIFI_POWER_19_5dBm = 78,
    WIFI_POWER_19dBm = 76,
    WIFI_POWER_18_5dBm = 74,
    WIFI_POWER_17dBm = 68,
    WIFI_POWER_15dBm = 60,
    WIFI_POWER_13dBm = 52,
    WIFI_POWER_11dBm = 44,
    WIFI_POWER_8_5dBm = 34,
    WIFI_POWER_7dBm = 28,
    WIFI_POWER_5dBm = 20,
    WIFI_POWER_2dBm = 8,
    WIFI_POWER_MINUS_1dBm = -4
} wifi_power_t;

static const int AP_STARTED_BIT        = (1 << 0);
static const int AP_HAS_IP6_BIT        = (1 << 1);
static const int AP_HAS_CLIENT_BIT     = (1 << 2);
static const int STA_STARTED_BIT       = (1 << 3);
static const int STA_CONNECTED_BIT     = (1 << 4);
static const int STA_HAS_IP_BIT        = (1 << 5);
static const int STA_HAS_IP6_BIT       = (1 << 6);
static const int ETH_STARTED_BIT       = (1 << 7);
static const int ETH_CONNECTED_BIT     = (1 << 8);
static const int ETH_HAS_IP_BIT        = (1 << 9);
static const int ETH_HAS_IP6_BIT       = (1 << 10);
static const int WIFI_SCANNING_BIT     = (1 << 11);
static const int WIFI_SCAN_DONE_BIT    = (1 << 12);
static const int WIFI_DNS_IDLE_BIT     = (1 << 13);
static const int WIFI_DNS_DONE_BIT     = (1 << 14);

class WiFiGenericClass {
public:
    static int setStatusBits(int bits);
    static int clearStatusBits(int bits);
    static int getStatusBits();
    static int waitStatusBits(int bits, uint32_t timeout_ms);

    static wifi_event_id_t onEvent(WiFiEventCb cbEvent, system_event_id_t event = SYSTEM_EVENT_MAX);
    static wifi_event_id_t onEvent(WiFiEventFuncCb cbEvent, system_event_id_t event = SYSTEM_EVENT_MAX);
    static wifi_event_id_t onEvent(WiFiEventSysCb cbEvent, system_event_id_t event = SYSTEM_EVENT_MAX);
    static void removeEvent(WiFiEventCb cbEvent, system_event_id_t event = SYSTEM_EVENT_MAX);
    static void removeEvent(WiFiEventSysCb cbEvent, system_event_id_t event = SYSTEM_EVENT_MAX);
    static void removeEvent(wifi_event_id_t id);

    static long int _eventCallback(void *arg, system_event_t *event);
    static int32_t channel();
    static bool mode(wifi_mode_t m);
    static wifi_mode_t getMode();
    static bool enableSTA(bool enable);
    static bool enableAP(bool enable);
    static bool setSleep(bool enable);
    static bool getSleep();
    static bool setTxPower(wifi_power_t power);
    static wifi_power_t getTxPower();
    static int hostByName(const char *aHostname, IPAddress &aResult);
    static IPAddress calculateNetworkID(IPAddress ip, IPAddress subnet);
    static IPAddress calculateBroadcast(IPAddress ip, IPAddress subnet);
    static uint8_t calculateSubnetCIDR(IPAddress subnetMask);

private:
    static wifi_mode_t _wifi_mode;
    static wifi_power_t _wifi_power;

protected:
    friend class WiFiSTAClass;
    friend class WiFiScanClass;
};

int htoi(const char *s);

#endif // WIFIGENERICV2_H