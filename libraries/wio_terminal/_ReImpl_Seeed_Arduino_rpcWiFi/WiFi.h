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

#ifndef WIFI_H
#define WIFI_H

#include <stdint.h>
#include <Print.h>
#include <IPAddress.h>
#include <IPv6Address.h>
#include "WiFiType.h"
#include "WiFiSTA.h"
#include "WiFiScan.h"
#include "WiFiGeneric.h"
#include "WiFiClient.h"
#include "WiFiUdp.h"

class WiFiClass : public WiFiGenericClass, public WiFiSTAClass, public WiFiScanClass {
public:
    using WiFiGenericClass::channel;
    using WiFiSTAClass::SSID;
    using WiFiSTAClass::RSSI;
    using WiFiSTAClass::BSSID;
    using WiFiSTAClass::BSSIDstr;
    using WiFiScanClass::SSID;
    using WiFiScanClass::encryptionType;
    using WiFiScanClass::RSSI;
    using WiFiScanClass::BSSID;
    using WiFiScanClass::BSSIDstr;
    using WiFiScanClass::channel;

    // Diagnostic output (no implementation, does nothing)
    void printDiag(Print& dest) {}

    // Friend classes
    friend class WiFiClient;
    friend class WiFiUDP;
};

// Declare an external instance of WiFiClass
extern WiFiClass WiFi;

#endif // WIFI_H
