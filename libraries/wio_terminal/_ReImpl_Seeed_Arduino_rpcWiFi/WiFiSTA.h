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

#ifndef WIFI_STACLASS_H
#define WIFI_STACLASS_H

#include "WiFiType.h"
#include "WiFiGeneric.h"

class WiFiSTAClass {
public:
    static void _setStatus(wl_status_t status);
    wl_status_t status();
    wl_status_t begin(const char *ssid, const char *passphrase = NULL, int32_t channel = 0, const uint8_t *bssid = NULL, bool connect = true);
    wl_status_t begin();
    bool reconnect();
    bool disconnect(bool wifioff = false, bool eraseap = false);
    bool config(IPAddress local_ip, IPAddress gateway, IPAddress subnet, IPAddress dns1 = (uint32_t)0x00000000, IPAddress dns2 = (uint32_t)0x00000000);
    bool isConnected();
    bool clearConnectedSetting();
    bool setAutoConnect(bool autoConnect);
    bool getAutoConnect();
    bool setAutoReconnect(bool autoReconnect);
    bool getAutoReconnect();
    uint8_t waitForConnectResult();
    IPAddress localIP();
    uint8_t* macAddress(uint8_t* mac);
    String macAddress();
    IPAddress subnetMask();
    IPAddress gatewayIP();
    IPAddress dnsIP(uint8_t dns_no = 0);
    IPAddress broadcastIP();
    IPAddress networkID();
    uint8_t subnetCIDR();
    String SSID();
    String psk();
    uint8_t* BSSID();
    String BSSIDstr();
    int8_t RSSI();
    const char* getHostname();
    bool setHostname(const char *hostname);
    bool enableIpV6();
    IPv6Address localIPv6();

    static bool _useStaticIp;
    static bool _autoReconnect;
};

#endif // WIFI_STACLASS_H
