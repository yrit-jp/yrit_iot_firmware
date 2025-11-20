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

#include "WiFi.h"
#include "WiFiGeneric.h"
#include "WiFiScan.h"

extern "C" {
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
}

// クラス変数の初期化（型を追加）
bool WiFiScanClass::_scanAsync       = false;
uint32_t WiFiScanClass::_scanStarted = 0;
uint32_t WiFiScanClass::_scanTimeout = 10000;
uint16_t WiFiScanClass::_scanCount   = 0;
void *WiFiScanClass::_scanResult     = 0;

int16_t WiFiScanClass::scanNetworks(bool async, bool show_hidden, bool passive, uint32_t max_ms_per_chan) {

	if (WiFiGenericClass::getStatusBits() & WIFI_SCANNING_BIT) {
		return WIFI_SCAN_RUNNING;
	}

	WiFiScanClass::_scanTimeout = max_ms_per_chan * 20;
	WiFiScanClass::_scanAsync   = async;

	WiFi.enableSTA(true);
	scanDelete();

	if (wifi_scan_start() != RTW_SUCCESS) {

		return WIFI_SCAN_FAILED;
	}

	WiFiScanClass::_scanStarted = millis();
	if (WiFiScanClass::_scanStarted == 0) {
		WiFiScanClass::_scanStarted = 1;
	}

	WiFiGenericClass::clearStatusBits(WIFI_SCAN_DONE_BIT);
	WiFiGenericClass::setStatusBits(WIFI_SCANNING_BIT);

	if (WiFiScanClass::_scanAsync) {
		return WIFI_SCAN_RUNNING;
	}

	if (WiFiGenericClass::waitStatusBits(WIFI_SCAN_DONE_BIT, 10000)) {
		return WiFiScanClass::_scanCount;
	}

	return WIFI_SCAN_FAILED;
}

void WiFiScanClass::_scanDone() {
	WiFiScanClass::_scanCount = wifi_scan_get_ap_num();

	if (WiFiScanClass::_scanCount > 0) {
		WiFiScanClass::_scanResult = new wifi_ap_record_t[WiFiScanClass::_scanCount];
		if (!WiFiScanClass::_scanResult ||
		    wifi_scan_get_ap_records(WiFiScanClass::_scanCount, (wifi_ap_record_t *)WiFiScanClass::_scanResult) != RTW_SUCCESS) {
			WiFiScanClass::_scanCount = 0;
		}
	}

	WiFiScanClass::_scanStarted = 0;
	WiFiGenericClass::setStatusBits(WIFI_SCAN_DONE_BIT);
	WiFiGenericClass::clearStatusBits(WIFI_SCANNING_BIT);
}

void *WiFiScanClass::_getScanInfoByIndex(int i) {
	if (!WiFiScanClass::_scanResult || i < 0 || i >= WiFiScanClass::_scanCount) {
		return nullptr;
	}
	return reinterpret_cast<wifi_ap_record_t *>(WiFiScanClass::_scanResult) + i;
}

int16_t WiFiScanClass::scanComplete() {
	if (WiFiScanClass::_scanStarted > 0 &&
	    (millis() - WiFiScanClass::_scanStarted) > WiFiScanClass::_scanTimeout) {
		WiFiGenericClass::clearStatusBits(WIFI_SCANNING_BIT);
		return WIFI_SCAN_FAILED;
	}

	if (WiFiGenericClass::getStatusBits() & WIFI_SCAN_DONE_BIT) {
		return WiFiScanClass::_scanCount;
	}

	if (WiFiGenericClass::getStatusBits() & WIFI_SCANNING_BIT) {
		return WIFI_SCAN_RUNNING;
	}

	return WIFI_SCAN_FAILED;
}

void WiFiScanClass::scanDelete() {
	WiFiGenericClass::clearStatusBits(WIFI_SCAN_DONE_BIT);

	if (WiFiScanClass::_scanResult) {
		delete[] reinterpret_cast<wifi_ap_record_t *>(WiFiScanClass::_scanResult);
		WiFiScanClass::_scanResult = nullptr;
		WiFiScanClass::_scanCount  = 0;
	}
}

bool WiFiScanClass::getNetworkInfo(uint8_t i, String &ssid, uint8_t &encType, int32_t &rssi, uint8_t *&bssid, int32_t &channel) {
	wifi_ap_record_t *record = reinterpret_cast<wifi_ap_record_t *>(_getScanInfoByIndex(i));
	if (!record) {
		return false;
	}

	ssid    = String(reinterpret_cast<const char *>(record->ssid));
	encType = record->authmode;
	rssi    = record->rssi;
	bssid   = record->bssid;
	channel = record->primary;

	return true;
}

String WiFiScanClass::SSID(uint8_t i) {
	wifi_ap_record_t *record = reinterpret_cast<wifi_ap_record_t *>(_getScanInfoByIndex(i));
	if (!record) {
		return String();
	}
	return String(reinterpret_cast<const char *>(record->ssid));
}

wifi_auth_mode_t WiFiScanClass::encryptionType(uint8_t index) {
	wifi_ap_record_t *record = reinterpret_cast<wifi_ap_record_t *>(_getScanInfoByIndex(index));
	if (!record) {
		return WIFI_AUTH_OPEN;
	}
	return record->authmode;
}

int32_t WiFiScanClass::RSSI(uint8_t index) {
	wifi_ap_record_t *record = reinterpret_cast<wifi_ap_record_t *>(_getScanInfoByIndex(index));
	if (!record) {
		return 0;
	}
	return record->rssi;
}

uint8_t *WiFiScanClass::BSSID(uint8_t index) {
	wifi_ap_record_t *record = reinterpret_cast<wifi_ap_record_t *>(_getScanInfoByIndex(index));
	if (!record) {
		return nullptr;
	}
	return record->bssid;
}

String WiFiScanClass::BSSIDstr(uint8_t index) {
	wifi_ap_record_t *record = reinterpret_cast<wifi_ap_record_t *>(_getScanInfoByIndex(index));
	if (!record) {
		return "";
	}

	char bssidStr[18];
	snprintf(bssidStr, sizeof(bssidStr), "%02X:%02X:%02X:%02X:%02X:%02X",
	         record->bssid[0], record->bssid[1], record->bssid[2],
	         record->bssid[3], record->bssid[4], record->bssid[5]);

	return String(bssidStr);
}

int32_t WiFiScanClass::channel(uint8_t index) {
	wifi_ap_record_t *record = reinterpret_cast<wifi_ap_record_t *>(_getScanInfoByIndex(index));
	if (!record) {
		return 0;
	}

	return record->primary; // Assuming 'primary' represents the channel number
}

void *WiFiScanClass::getScanInfoByIndex(int i) {
	return _getScanInfoByIndex(i); // Simply returns the result of _getScanInfoByIndex(i)
}