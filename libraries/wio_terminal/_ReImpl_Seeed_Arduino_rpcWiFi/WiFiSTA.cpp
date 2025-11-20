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
#include "WiFiSTA.h"

extern "C" {
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include "lwip/err.h"
#include "lwip/dns.h"
}

bool WiFiSTAClass::_autoReconnect = true;
bool WiFiSTAClass::_useStaticIp   = false;

static wl_status_t _sta_status              = WL_NO_SHIELD;
static EventGroupHandle_t _sta_status_group = NULL;

void WiFiSTAClass::_setStatus(wl_status_t status) {
	if (_sta_status_group == nullptr) {
		// イベントグループが初期化されていない場合、作成する
		_sta_status_group = xEventGroupCreate();
		if (_sta_status_group == nullptr) {
			// イベントグループ作成失敗時、ステータスを設定して戻る
			_sta_status = status;
			return;
		}
	}

	// イベントグループが初期化されている場合でも、常にビットをクリア
	xEventGroupClearBits(_sta_status_group, 0x00FFFFFF);

	// ステータスをイベントグループにセット
	xEventGroupSetBits(_sta_status_group, status);
}

wl_status_t WiFiSTAClass::status() {
	if (_sta_status_group == nullptr) {
		// イベントグループが未初期化の場合、内部ステータスを返す
		return _sta_status;
	}

	// WiFiの接続状態を確認
	if (wifi_is_connected_to_ap() != RTW_SUCCESS) {
		_setStatus(WL_DISCONNECTED);
	}

	// すべてのビットをクリアし、その結果を取得
	EventBits_t clearedBits = xEventGroupClearBits(_sta_status_group, 0);

	// クリアされたビットの値を返す
	return static_cast<wl_status_t>(clearedBits);
}

wl_status_t WiFiSTAClass::begin(const char *ssid, const char *passphrase, int32_t channel, const uint8_t *bssid, bool connect) {
	// STAモードの有効化
	if (!WiFi.enableSTA(true)) {
		return WL_CONNECT_FAILED;
	}

	// SSIDの検証
	if (ssid == nullptr || strlen(ssid) == 0 || strlen(ssid) > 31) {
		return WL_CONNECT_FAILED;
	}

	// パスフレーズの検証
	if (passphrase != nullptr && strlen(passphrase) > 64) {
		return WL_CONNECT_FAILED;
	}

	// セキュリティタイプの設定
	rtw_security_t security = (passphrase == nullptr) ? RTW_SECURITY_OPEN : RTW_SECURITY_WPA2_AES_PSK;

	// チャンネルの設定
	if (channel != 0 && bssid == nullptr) {
		uint8_t pscan_config = PSCAN_ENABLE | PSCAN_FAST_SURVEY;
		if (wifi_set_pscan_chan((uint8_t *)&channel, &pscan_config, 1) < 0) {
			wifi_off(); // Wi-Fi再起動
			wifi_on(RTW_MODE_STA);
			return WL_CONNECT_FAILED;
		}
	}

	// 接続の試行

	bool connectResult = (bssid != nullptr)
	                         ? wifi_connect_bssid((unsigned char *)bssid, (char *)ssid, security, (char *)passphrase, ETH_ALEN, strlen(ssid), strlen(passphrase), -1, NULL)
	                         : wifi_connect((char *)ssid, security, (char *)passphrase, strlen(ssid), strlen(passphrase), -1, NULL);

	if (connectResult == RTW_ERROR) {
		return WL_CONNECT_FAILED;
	}

	// DHCPの設定
	if (!_useStaticIp) {
		int dhcpResult = tcpip_adapter_dhcpc_start(TCPIP_ADAPTER_IF_STA);
		if (dhcpResult == 0x5002) {
			return WL_CONNECT_FAILED;
		}
	} else {
		tcpip_adapter_up(TCPIP_ADAPTER_IF_STA);
	}

	// 接続状態の確認
	if (connect && wifi_is_connected_to_ap() != RTW_SUCCESS) {
		return WL_CONNECT_FAILED;
	}

	// 現在の接続状態を返す
	return status();
}

wl_status_t WiFiSTAClass::begin() {
	// WiFi機能の有効確認
	if (!WiFi.enableSTA(true)) {
		return WL_CONNECT_FAILED;
	}

	// 再接続データの取得
	wlan_fast_reconnect_profile_t wifi_info;
	if (!wifi_get_reconnect_data(&wifi_info)) {
		return WL_CONNECT_FAILED;
	}

	// セキュリティタイプとキーIDの設定
	rtw_security_t security = wifi_info.security_type;
	char key_id             = (char)(wifi_info.channel >> 28);

	// WiFiへの接続試行
	bool connectResult = (security == RTW_SECURITY_OPEN)
	                         ? wifi_connect((char *)wifi_info.psk_essid, security, NULL, strlen((char *)wifi_info.psk_essid), 0, key_id, NULL)
	                         : wifi_connect((char *)wifi_info.psk_essid, security, (char *)wifi_info.psk_passphrase, strlen((char *)wifi_info.psk_essid), strlen((char *)wifi_info.psk_passphrase), key_id, NULL);

	if (!connectResult) {
		return WL_CONNECT_FAILED;
	}

	// DHCPクライアントの起動
	if (!_useStaticIp) {
		if (tcpip_adapter_dhcpc_start(TCPIP_ADAPTER_IF_STA) == 0x5002) {
			return WL_CONNECT_FAILED;
		}
	} else {
		tcpip_adapter_dhcpc_stop(TCPIP_ADAPTER_IF_STA);
	}

	// 接続状態の確認
	if (status() != WL_CONNECTED && wifi_is_connected_to_ap() != RTW_SUCCESS) {
		return WL_CONNECT_FAILED;
	}

	// 正常な接続状態を取得
	return status();
}

bool WiFiSTAClass::reconnect() {
	// WiFiモードの確認
	if (WiFi.getMode() != WIFI_MODE_STA) {
		return false; // STAモードでない場合、接続失敗
	}

	// WiFiから切断
	if (wifi_disconnect() != RTW_SUCCESS) {
		return false; // 切断失敗
	}

	// 再接続の試行
	if (begin() == WL_CONNECT_FAILED) {
		return false; // 接続失敗
	}

	return true; // 接続成功
}

bool WiFiSTAClass::disconnect(bool wifioff, bool eraseap) {
	// WiFiモードの確認
	if (WiFi.getMode() != WIFI_MODE_STA) {
		return false; // STAモードでない場合、接続失敗
	}

	// WiFiからの切断
	if (wifi_disconnect() != RTW_SUCCESS) {
		return false; // 切断失敗
	}

	// WiFi機能のオフにする
	if (wifioff) {
		if (!WiFi.enableSTA(false)) {
			return false; // WiFi機能オフ失敗
		}
	}

	return true; // 切断成功
}

bool WiFiSTAClass::config(IPAddress local_ip, IPAddress gateway, IPAddress subnet, IPAddress dns1, IPAddress dns2) {
	// WiFi機能を有効にする
	if (!WiFi.enableSTA(true)) {
		return false; // WiFi機能有効化失敗
	}

	// tcpip_adapter_ip_info_t構造体の作成
	tcpip_adapter_ip_info_t info;
	if (local_ip == IPAddress(0, 0, 0, 0)) {
		memset(&info, 0, sizeof(info)); // local_ipが0の場合、すべてを0に設定
	} else {
		info.ip.addr      = static_cast<uint32_t>(local_ip);
		info.gw.addr      = static_cast<uint32_t>(gateway);
		info.netmask.addr = static_cast<uint32_t>(subnet);
	}

	// DHCPクライアントの停止
	int dhcp_stop_result = tcpip_adapter_dhcpc_stop(TCPIP_ADAPTER_IF_STA);
	if (dhcp_stop_result != 0 && dhcp_stop_result != 0x5004) {
		return false; // DHCP停止失敗
	}

	// IP情報の設定
	if (tcpip_adapter_set_ip_info(TCPIP_ADAPTER_IF_STA, &info) != 0) {
		return false; // IP設定失敗
	}

	// info.ip.addrが0でない場合、静的IPフラグを設定
	if (info.ip.addr != 0) {
		_useStaticIp = true;
	} else {
		// info.ip.addrが0の場合、DHCPクライアントを再起動
		if (tcpip_adapter_dhcpc_start(TCPIP_ADAPTER_IF_STA) == 0x5002) {
			return false; // DHCP起動失敗
		}
		_useStaticIp = false;
	}

	// DNSサーバーの設定
	if (dns1 != IPAddress(0, 0, 0, 0)) {
		tcpip_adapter_dns_info_t tcpip_dns0;
		tcpip_dns0.ipv4 = dns1;
		tcpip_adapter_set_dns_info(TCPIP_ADAPTER_IF_MAX, TCPIP_ADAPTER_DNS_MAIN, &tcpip_dns0);
	}

	if (dns2 != IPAddress(0, 0, 0, 0)) {
		tcpip_adapter_dns_info_t tcpip_dns1;
		tcpip_dns1.ipv4 = dns2;
		tcpip_adapter_set_dns_info(TCPIP_ADAPTER_IF_MAX, TCPIP_ADAPTER_DNS_BACKUP, &tcpip_dns1);
	}

	return true; // 設定成功
}

bool WiFiSTAClass::isConnected() {
	return status() == WL_CONNECTED;
}

bool WiFiSTAClass::clearConnectedSetting() {
	return wifi_clear_reconnect_data() != 0;
}

bool WiFiSTAClass::setAutoConnect(bool autoConnect) {
	return false;
}

bool WiFiSTAClass::getAutoConnect() {
	return false;
}

bool WiFiSTAClass::setAutoReconnect(bool autoReconnect) {
	return false;
}

bool WiFiSTAClass::getAutoReconnect() {
	return false;
}

uint8_t WiFiSTAClass::waitForConnectResult() {
	if (!(WiFiGenericClass::getMode() & WIFI_MODE_STA)) {
		return WL_DISCONNECTED;
	}

	uint8_t result;
	for (int i = 0; i < 100; i++) {
		result = status();
		if (result == WL_CONNECTED || result >= WL_DISCONNECTED) {
			return result;
		}
		delay(100);
	}

	return status();
}

IPAddress WiFiSTAClass::localIP() {
	if (WiFiGenericClass::getMode() == WIFI_MODE_NULL) {
		return IPAddress();
	}

	tcpip_adapter_ip_info_t ip;
	tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &ip);

	return IPAddress(ip.ip.addr);
}

uint8_t *WiFiSTAClass::macAddress(uint8_t *mac) {
	if (WiFiGenericClass::getMode() == WIFI_MODE_NULL) {
		memset(mac, 0, 6);
		return mac;
	}

	String macStr = macAddress();
	if (macStr.length() != 17) {
		memset(mac, 0, 6);
		return mac;
	}

	for (int i = 0; i < 6; ++i) {
		mac[i] = htoi(macStr.substring(i * 3, i * 3 + 2).c_str());
	}

	return mac;
}

String WiFiSTAClass::macAddress() {
	if (WiFiGenericClass::getMode() == WIFI_MODE_NULL) {
		return String();
	}

	char macStr[18] = {0};
	tcpip_adapter_get_mac(TCPIP_ADAPTER_IF_AP, (uint8_t *)macStr);

	return String(macStr);
}

IPAddress WiFiSTAClass::subnetMask() {
	if (WiFiGenericClass::getMode() == WIFI_MODE_NULL) {
		return IPAddress();
	}

	tcpip_adapter_ip_info_t ip;
	tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &ip);

	return IPAddress(ip.netmask.addr);
}

IPAddress WiFiSTAClass::gatewayIP() {
	if (WiFiGenericClass::getMode() == WIFI_MODE_NULL) {
		return IPAddress();
	}

	tcpip_adapter_ip_info_t ip;
	tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &ip);

	return IPAddress(ip.gw.addr);
}

IPAddress WiFiSTAClass::dnsIP(uint8_t dns_no) {
	// WiFiモードがWIFI_MODE_NULLの場合、空のIPAddressを返す
	if (WiFiGenericClass::getMode() == WIFI_MODE_NULL) {
		return IPAddress(); // 空のIPアドレスを返す
	}

	// DNS情報を格納する構造体を準備
	tcpip_adapter_dns_info_t tcpip_dns_ip;

	if (dns_no == TCPIP_ADAPTER_DNS_MAIN) {
		// プライマリDNS情報を取得
		tcpip_adapter_get_dns_info(TCPIP_ADAPTER_IF_MAX, TCPIP_ADAPTER_DNS_MAIN, &tcpip_dns_ip);
	} else {
		// セカンダリDNS情報を取得
		tcpip_adapter_get_dns_info(TCPIP_ADAPTER_IF_MAX, TCPIP_ADAPTER_DNS_BACKUP, &tcpip_dns_ip);
	}

	// 取得したIPv4アドレスからIPAddressオブジェクトを生成して返す
	return IPAddress(tcpip_dns_ip.ipv4);
}

IPAddress WiFiSTAClass::broadcastIP() {
	// WiFiモードがWIFI_MODE_NULLの場合、空のIPAddressを返す
	if (WiFiGenericClass::getMode() == WIFI_MODE_NULL) {
		return IPAddress(); // 空のIPアドレスを返す
	}

	// IP情報を格納する構造体を準備
	tcpip_adapter_ip_info_t ip;

	// STAインターフェースのIP情報を取得
	tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &ip);

	// ゲートウェイIPアドレスとサブネットマスクからブロードキャストIPを計算して返す
	return WiFiGenericClass::calculateBroadcast(IPAddress(ip.gw.addr), IPAddress(ip.netmask.addr));
}

IPAddress WiFiSTAClass::networkID() {
	// WiFiモードがWIFI_MODE_NULLの場合、空のIPAddressを返す
	if (WiFiGenericClass::getMode() == WIFI_MODE_NULL) {
		return IPAddress(); // 空のIPアドレスを返す
	}

	// IP情報を格納する構造体を準備
	tcpip_adapter_ip_info_t ip;

	// STAインターフェースのIP情報を取得
	tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &ip);

	// IPアドレスとサブネットマスクからネットワークIDを計算して返す
	return WiFiGenericClass::calculateNetworkID(IPAddress(ip.ip.addr), IPAddress(ip.netmask.addr));
}

uint8_t WiFiSTAClass::subnetCIDR() {
	// WiFiモードがWIFI_MODE_NULLの場合、0を返す
	if (WiFiGenericClass::getMode() == WIFI_MODE_NULL) {
		return 0;
	}

	// IP情報を格納する構造体を準備
	tcpip_adapter_ip_info_t ip;

	// STAインターフェースのIP情報を取得
	tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &ip);

	// サブネットマスクからサブネットCIDRを計算して返す
	return WiFiGenericClass::calculateSubnetCIDR(IPAddress(ip.netmask.addr));
}

String WiFiSTAClass::SSID() {
	// 現在のWiFiモードを取得
	WiFiMode_t current_mode = WiFiGenericClass::getMode();

	rtw_wifi_setting_t wifi_setting;

	// APSTAモードの場合
	if (current_mode == WIFI_MODE_APSTA) {
		// WLAN1_NAMEから設定を取得
		wifi_get_setting(WLAN1_NAME, &wifi_setting);
		// SSIDを返す
		return String(reinterpret_cast<char *>(wifi_setting.ssid));
	}
	// STAモードの場合
	else if (current_mode == WIFI_MODE_STA) {
		// WLAN0_NAMEから設定を取得
		wifi_get_setting(WLAN0_NAME, &wifi_setting);
		// SSIDを返す
		return String(reinterpret_cast<char *>(wifi_setting.ssid));
	}

	// その他のモードの場合は空のStringを返す
	return String(); // 空のStringオブジェクトを返す
}

String WiFiSTAClass::psk() {
	// 現在のWiFiモードを取得
	WiFiMode_t current_mode = WiFiGenericClass::getMode();

	// モードが無効の場合、空のStringを返す
	if (current_mode == WIFI_MODE_NULL) {
		return String();
	}

	wifi_config_t conf;

	// Wi-Fi設定を取得
	esp_wifi_get_config(WIFI_IF_STA, &conf);

	// パスワードを返す
	return String(reinterpret_cast<char *>(conf.sta.password));
}

uint8_t *WiFiSTAClass::BSSID() {
	// 現在のWiFiモードを取得
	WiFiMode_t current_mode = WiFiGenericClass::getMode();

	// モードが無効の場合、NULLを返す
	if (current_mode == WIFI_MODE_NULL) {
		return NULL;
	}

	// 定数サイズの静的配列を作成
	static uint8_t bssid[6];

	// MACアドレスを取得
	wifi_get_ap_bssid(bssid);

	// 取得したMACアドレスをポインタとして返す
	return reinterpret_cast<uint8_t *>(bssid);
}

String WiFiSTAClass::BSSIDstr() {
	// BSSID()メソッドを呼び出してMACアドレスを取得
	uint8_t *mac = BSSID();

	// 取得に失敗した場合は空のStringを返す
	if (mac == NULL) {
		return String();
	}

	// MACアドレスを文字列としてフォーマット
	char mac_str[18];
	sprintf(mac_str, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

	// フォーマットした文字列をString型で返す
	return String(mac_str);
}

int8_t WiFiSTAClass::RSSI() {
	// WiFiのモードを取得
	if (WiFiGenericClass::getMode() == WIFI_MODE_NULL) {
		// モードが無効な場合は0を返す
		return 0;
	}

	// RSSI値を格納する変数を作成
	int rssi = 0;

	// RSSIを取得する
	if (wifi_get_rssi(&rssi) == 0) {
		// 取得成功した場合はrssiの値を返す
		return rssi;
	}

	// 取得に失敗した場合は0を返す
	return 0;
}

const char *WiFiSTAClass::getHostname() {
	if (WiFiGenericClass::getMode() == WIFI_MODE_NULL) {
		// モードが無効な場合はNULLを返す
		return nullptr;
	}

	const char *hostname = nullptr;

	// ホスト名を取得
	if (tcpip_adapter_get_hostname(TCPIP_ADAPTER_IF_STA, &hostname) == 0) {
		// 取得に成功した場合、ホスト名を返す
		return hostname;
	}

	// 失敗した場合はNULLを返す
	return nullptr;
}

bool WiFiSTAClass::setHostname(const char *hostname) {
	// WiFiのモードを取得
	if (WiFiGenericClass::getMode() == WIFI_MODE_NULL) {
		// モードが無効な場合はfalseを返す
		return false;
	}

	// ホスト名を設定する
	if (tcpip_adapter_set_hostname(TCPIP_ADAPTER_IF_STA, hostname) == 0) {
		// 設定成功した場合はtrueを返す
		return true;
	}

	// 設定に失敗した場合はfalseを返す
	return false;
}

bool WiFiSTAClass::enableIpV6() {
	// WiFiのモードを取得
	if (WiFiGenericClass::getMode() == WIFI_MODE_NULL) {
		// モードが無効な場合はfalseを返す
		return false;
	}

	// IPv6のリンクローカルアドレスを作成する
	if (tcpip_adapter_create_ip6_linklocal(TCPIP_ADAPTER_IF_STA) == 0) {
		// 成功した場合はtrueを返す
		return true;
	}

	// 失敗した場合はfalseを返す
	return false;
}

IPv6Address WiFiSTAClass::localIPv6() {
	// WiFiのモードを取得
	if (WiFiGenericClass::getMode() == WIFI_MODE_NULL) {
		// モードが無効な場合は空のIPv6Addressを返す
		return IPv6Address();
	}

	// ip6_addr_t 型の変数を作成
	static ip6_addr_t addr;

	// IPv6リンクローカルアドレスを取得
	if (tcpip_adapter_get_ip6_linklocal(TCPIP_ADAPTER_IF_STA, &addr) == 0) {
		// 取得に成功した場合、IPv6Addressオブジェクトを作成して返す
		return IPv6Address(reinterpret_cast<const uint8_t *>(&addr));
	}

	// 失敗した場合は空のIPv6Addressを返す
	return IPv6Address();
}
