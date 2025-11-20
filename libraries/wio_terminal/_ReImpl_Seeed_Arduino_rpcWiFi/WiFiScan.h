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

#ifndef WiFiScanClass_h
#define WiFiScanClass_h

#include "WiFiType.h"
#include "WiFiGeneric.h"

class WiFiScanClass {
public:
    // Public methods
    int16_t scanNetworks(bool async = false, bool show_hidden = false, bool passive = false, uint32_t max_ms_per_chan = 300);
    int16_t scanComplete();
    void scanDelete();
    bool getNetworkInfo(uint8_t networkItem, String &ssid, uint8_t &encryptionType, int32_t &RSSI, uint8_t* &BSSID, int32_t &channel);
    
    String SSID(uint8_t networkItem);
    wifi_auth_mode_t encryptionType(uint8_t networkItem);
    int32_t RSSI(uint8_t networkItem);
    uint8_t* BSSID(uint8_t networkItem);
    String BSSIDstr(uint8_t networkItem);
    int32_t channel(uint8_t networkItem);

    // Static methods
    static void* getScanInfoByIndex(int i); 
    static void _scanDone();

protected:
    // Static protected methods
    static void* _getScanInfoByIndex(int i);

    // Static class variables
    static bool _scanAsync;
    static uint32_t _scanStarted;
    static uint32_t _scanTimeout;
    static uint16_t _scanCount;
    static void* _scanResult;
};

#endif // WiFiScanClass_h
