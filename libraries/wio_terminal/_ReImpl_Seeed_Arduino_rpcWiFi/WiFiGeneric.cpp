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

extern "C"
{
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include "lwip/ip_addr.h"
#include "lwip/opt.h"
#include "lwip/err.h"
#include "lwip/dns.h"
} // extern "C"

#include <vector>

wifi_mode_t WiFiGenericClass::_wifi_mode   = WIFI_MODE_NULL;
wifi_power_t WiFiGenericClass::_wifi_power = WIFI_POWER_19_5dBm;

int htoi(const char *hexStr) {
	int result = 0;
	bool isHex = false;

	// 空の文字列やNULLポインタのチェック
	if (hexStr == nullptr || *hexStr == '\0') {
		return -1; // エラー処理、適宜変更
	}

	// 文字列が "0x" または "0X" で始まっている場合、スキップ
	if (hexStr[0] == '0' && (hexStr[1] == 'x' || hexStr[1] == 'X')) {
		hexStr += 2; // "0x"をスキップ
	}

	// 文字列の各文字を16進数として処理
	while (*hexStr) {
		if (isdigit(*hexStr)) {
			result = result * 16 + (*hexStr - '0');
		} else if (*hexStr >= 'a' && *hexStr <= 'f') {
			result = result * 16 + (*hexStr - 'a' + 10);
		} else if (*hexStr >= 'A' && *hexStr <= 'F') {
			result = result * 16 + (*hexStr - 'A' + 10);
		} else {
			return -1; // 無効な文字が含まれている場合
		}
		hexStr++;
	}

	return result;
}

static EventGroupHandle_t wifi_event_group = NULL;  // WiFiイベントグループ
static bool wifi_initialized               = false; // 初期化フラグ
static bool wifiLowLevelInit(void) {

	if (wifi_initialized) {
		// 既に初期化が行われている場合は、何もしない
		return true;
	}

	// WiFiイベントグループが未作成の場合、作成する
	if (wifi_event_group == NULL) {
		wifi_event_group = xEventGroupCreate();
		if (wifi_event_group == NULL) {

			return false; // 作成に失敗した場合はfalseを返す
		}

		// イベントグループに「WIFI_DNS_IDLE_BIT」をセット
		xEventGroupSetBits(wifi_event_group, WIFI_DNS_IDLE_BIT);
	}

	tcpip_adapter_init();

	// システムイベントコールバック関数の登録
	system_event_callback_reg(WiFiGenericClass::_eventCallback);

	// 初期化が成功したことを示すフラグを設定
	wifi_initialized = true;

	// 初期化成功を返す
	return true;
}

static bool wifi_started = false; // WiFi起動状態を管理するフラグ
// WiFi起動関数
static bool WiFiStart(void) {

	if (wifi_started) {
		// 既にWiFiが起動している場合はそのままtrueを返す
		return true;
	}

	// 低レベル初期化を実行
	if (!wifiLowLevelInit()) {
		return false; // 低レベル初期化が失敗した場合
	}

	// WiFiの起動状態を設定
	wifi_started = true;

	// システムイベントを生成
	system_event_t event;
	event.event_id = SYSTEM_EVENT_WIFI_READY;

	// イベントコールバックを呼び出す
	WiFiGenericClass::_eventCallback(nullptr, &event);

	// 成功を示す戻り値
	return true;
}

// -----------------------------------------------------------------------------------------------------------------------
// ------------------------------------------------- Generic WiFi function -----------------------------------------------
// -----------------------------------------------------------------------------------------------------------------------

typedef struct WiFiEventCbList_t {
	static wifi_event_id_t current_id;
	wifi_event_id_t id;
	WiFiEventCb cb;
	WiFiEventFuncCb fcb;
	WiFiEventSysCb scb;
	system_event_id_t event;

	WiFiEventCbList_t() : id(current_id++), cb(NULL), fcb(NULL), scb(NULL), event(SYSTEM_EVENT_WIFI_READY) {}
} WiFiEventCbList_t;

wifi_event_id_t WiFiEventCbList_t::current_id = 1;

static std::vector<WiFiEventCbList_t> event_callbacks;

int WiFiGenericClass::setStatusBits(int bits) {

	// イベントグループが存在しない場合は 0 を返す
	if (wifi_event_group == NULL) {

		return 0;
	}

	// 指定されたビットをイベントグループに設定
	EventBits_t result = xEventGroupSetBits(wifi_event_group, bits);

	// 設定されたビットの状態を返す
	return result;
}

// clearStatusBits メソッド
int WiFiGenericClass::clearStatusBits(int bits) {

	if (wifi_event_group == NULL) {
		return 0;
	}
	// 指定されたビットをイベントグループからクリア
	EventBits_t result = xEventGroupClearBits(wifi_event_group, bits);
	return result;
}

// getStatusBits メソッド
int WiFiGenericClass::getStatusBits() {

	if (wifi_event_group == NULL) {
		return 0;
	}
	// 現在設定されているビットの状態を取得
	EventBits_t current_bits = xEventGroupGetBits(wifi_event_group);
	return current_bits;
}

// waitStatusBits メソッド
int WiFiGenericClass::waitStatusBits(int bits, uint32_t timeout_ms) {

	if (wifi_event_group == NULL) {
		return 0;
	}
	// 指定したビットが設定されるまで待機 (タイムアウト付き)
	EventBits_t result = xEventGroupWaitBits(
	    wifi_event_group,
	    bits,
	    pdFALSE,
	    pdTRUE,
	    timeout_ms / portTICK_PERIOD_MS) & bits;
	return result; // 設定されたビットの状態を返す
}

wifi_event_id_t WiFiGenericClass::onEvent(WiFiEventCb cbEvent, system_event_id_t event) {

	// コールバック関数が NULL の場合、処理を終了
	if (!cbEvent) {
		return 0;
	}

	// 新しいイベントハンドラを作成
	WiFiEventCbList_t newEventHandler;

	// コールバック関数を設定
	newEventHandler.cb = cbEvent;

	// 関数形式のコールバック関数を NULL に設定
	newEventHandler.fcb = NULL;

	// システム形式のコールバック関数を NULL に設定
	newEventHandler.scb = NULL;

	// 受信イベントのIDを設定
	newEventHandler.event = event;

	// イベントハンドラを event_callbacks に追加
	event_callbacks.push_back(newEventHandler);

	// 新しいイベントハンドラのIDを返す
	return newEventHandler.id;
}

wifi_event_id_t WiFiGenericClass::onEvent(WiFiEventFuncCb cbEvent, system_event_id_t event) {

	// コールバック関数が NULL の場合、処理を終了
	if (!cbEvent) {
		return 0;
	}

	// 新しいイベントハンドラを作成
	WiFiEventCbList_t newEventHandler;

	// 普通のコールバック関数ポインタを NULL に設定
	newEventHandler.cb = NULL;

	// 関数形式のコールバック関数ポインタを設定
	newEventHandler.fcb = cbEvent;

	// システム形式のコールバック関数ポインタを NULL に設定
	newEventHandler.scb = NULL;

	// 受信イベントのIDを設定
	newEventHandler.event = event;

	// イベントハンドラを event_callbacks に追加
	event_callbacks.push_back(newEventHandler);

	// 新しいイベントハンドラのIDを返す
	return newEventHandler.id;
}

wifi_event_id_t WiFiGenericClass::onEvent(WiFiEventSysCb cbEvent, system_event_id_t event) {

	// コールバック関数が NULL の場合、処理を終了
	if (!cbEvent) {
		return 0;
	}

	// 新しいイベントハンドラを作成
	WiFiEventCbList_t newEventHandler;

	// 普通のコールバック関数ポインタを NULL に設定
	newEventHandler.cb = NULL;

	// 関数形式のコールバック関数ポインタを NULL に設定
	newEventHandler.fcb = NULL;

	// システム形式のコールバック関数ポインタを設定
	newEventHandler.scb = cbEvent;

	// 受信イベントのIDを設定
	newEventHandler.event = event;

	// イベントハンドラを event_callbacks に追加
	event_callbacks.push_back(newEventHandler);

	// 新しいイベントハンドラのIDを返す
	return newEventHandler.id;
}

void WiFiGenericClass::removeEvent(WiFiEventCb cbEvent, system_event_id_t event) {

	// コールバック関数が NULL の場合、処理を終了
	if (!cbEvent) {
		return;
	}

	// 登録されているすべてのイベントハンドラに対してループ
	for (uint32_t i = 0; i < event_callbacks.size(); i++) {
		// 現在のイベントハンドラを取得
		WiFiEventCbList_t entry = event_callbacks[i];

		// イベントハンドラが指定されたコールバック関数とイベントIDに一致するか確認
		if (entry.cb == cbEvent && entry.event == event) {
			// 一致したハンドラをリストから削除
			event_callbacks.erase(event_callbacks.begin() + i);
			// 次の要素を処理するため、インデックス i をそのままにしておく
		}
	}
}

void WiFiGenericClass::removeEvent(WiFiEventSysCb cbEvent, system_event_id_t event) {

	// コールバック関数が NULL の場合、処理を終了
	if (!cbEvent) {
		return;
	}

	// 登録されているすべてのイベントハンドラに対してループ
	for (uint32_t i = 0; i < event_callbacks.size(); i++) {
		// 現在のイベントハンドラを取得
		WiFiEventCbList_t entry = event_callbacks[i];

		// イベントハンドラが指定されたシステム形式のコールバック関数とイベントIDに一致するか確認
		if (entry.scb == cbEvent && entry.event == event) {
			// 一致したハンドラをリストから削除
			event_callbacks.erase(event_callbacks.begin() + i);
			// 次の要素を処理するため、インデックス i をそのままにしておく
		}
	}
}

void WiFiGenericClass::removeEvent(wifi_event_id_t id) {

	// 登録されているすべてのイベントハンドラに対してループ
	for (uint32_t i = 0; i < event_callbacks.size(); i++) {
		// 現在のイベントハンドラを取得
		WiFiEventCbList_t entry = event_callbacks[i];

		// イベントハンドラが指定されたIDに一致するか確認
		if (entry.id == id) {
			// 一致したハンドラをリストから削除
			event_callbacks.erase(event_callbacks.begin() + i);
			// 次の要素を処理するため、インデックス i をそのままにしておく
		}
	}
}

long int WiFiGenericClass::_eventCallback(void *arg, system_event_t *event) {

	if (!event)
		return -1;

	// 基本的なイベントログ
	if (event->event_id < 26) {
		// debugLog("Event ID: %d", event->event_id);
	}

	switch (event->event_id) {
	case SYSTEM_EVENT_SCAN_DONE:
		WiFiScanClass::_scanDone();
		break;

	case SYSTEM_EVENT_STA_START:

		WiFiSTAClass::_setStatus(WL_DISCONNECTED);
		setStatusBits(STA_STARTED_BIT);
		break;

	case SYSTEM_EVENT_STA_STOP:
		WiFiSTAClass::_setStatus(WL_NO_SHIELD);
		clearStatusBits(STA_STARTED_BIT | STA_CONNECTED_BIT | STA_HAS_IP_BIT | STA_HAS_IP6_BIT);
		break;

	case SYSTEM_EVENT_STA_CONNECTED:

		WiFiSTAClass::_setStatus(WL_IDLE_STATUS);
		setStatusBits(STA_CONNECTED_BIT);
		break;

	case SYSTEM_EVENT_STA_DISCONNECTED: {
		uint8_t reason = event->event_info.disconnected.reason;
		// logDisconnectionReason(reason);

		switch (reason) {
		case WIFI_REASON_NO_AP_FOUND:
			WiFiSTAClass::_setStatus(WL_NO_SSID_AVAIL);
			break;
		case WIFI_REASON_AUTH_FAIL:
		case WIFI_REASON_ASSOC_FAIL:
			WiFiSTAClass::_setStatus(WL_CONNECT_FAILED);
			break;
		case WIFI_REASON_BEACON_TIMEOUT:
		case WIFI_REASON_HANDSHAKE_TIMEOUT:
			WiFiSTAClass::_setStatus(WL_CONNECTION_LOST);
			break;
		case WIFI_REASON_AUTH_EXPIRE:
			break;
		default:
			WiFiSTAClass::_setStatus(WL_DISCONNECTED);
			break;
		}

		clearStatusBits(STA_CONNECTED_BIT | STA_HAS_IP_BIT | STA_HAS_IP6_BIT);

		if ((reason == WIFI_REASON_AUTH_EXPIRE) ||
		    (reason == WIFI_REASON_BEACON_TIMEOUT && reason != WIFI_REASON_AUTH_FAIL) ||
		    WiFi.getAutoReconnect()) {
			WiFi.disconnect();
			WiFi.begin();
		}
		break;
	}

	case SYSTEM_EVENT_STA_GOT_IP:

		WiFiSTAClass::_setStatus(WL_CONNECTED);
		setStatusBits(STA_HAS_IP_BIT | STA_CONNECTED_BIT);
		break;

	case SYSTEM_EVENT_STA_LOST_IP:
		WiFiSTAClass::_setStatus(WL_IDLE_STATUS);
		clearStatusBits(STA_HAS_IP_BIT);
		break;

	case SYSTEM_EVENT_AP_START:
		setStatusBits(AP_STARTED_BIT);
		break;

	case SYSTEM_EVENT_AP_STOP:
		clearStatusBits(AP_STARTED_BIT | AP_HAS_CLIENT_BIT);
		break;

	case SYSTEM_EVENT_AP_STACONNECTED:
		setStatusBits(AP_HAS_CLIENT_BIT);
		break;

	case SYSTEM_EVENT_AP_STADISCONNECTED:
		// if (WiFi.softAPgetStationNum() == 0) {
		//     clearStatusBits(AP_HAS_CLIENT_BIT);
		// }
		break;

	case SYSTEM_EVENT_ETH_START:
		setStatusBits(ETH_STARTED_BIT);
		break;

	case SYSTEM_EVENT_ETH_STOP:
		clearStatusBits(ETH_STARTED_BIT | ETH_CONNECTED_BIT | ETH_HAS_IP_BIT | ETH_HAS_IP6_BIT);
		break;

	case SYSTEM_EVENT_ETH_CONNECTED:
		setStatusBits(ETH_CONNECTED_BIT);
		break;

	case SYSTEM_EVENT_ETH_DISCONNECTED:
		clearStatusBits(ETH_CONNECTED_BIT | ETH_HAS_IP_BIT | ETH_HAS_IP6_BIT);
		break;

	case SYSTEM_EVENT_ETH_GOT_IP:
		setStatusBits(ETH_CONNECTED_BIT | ETH_HAS_IP_BIT);
		break;

	case SYSTEM_EVENT_GOT_IP6: {
		uint8_t if_index = event->event_info.got_ip6.if_index;
		if (if_index == TCPIP_ADAPTER_IF_AP) {
			setStatusBits(AP_HAS_IP6_BIT);
		} else if (if_index == TCPIP_ADAPTER_IF_STA) {
			setStatusBits(STA_CONNECTED_BIT | STA_HAS_IP6_BIT);
		} else if (if_index == TCPIP_ADAPTER_IF_ETH) {
			setStatusBits(ETH_CONNECTED_BIT | ETH_HAS_IP6_BIT);
		}
		break;
	}
	}

	// ユーザー定義のコールバック呼び出し
	size_t listSize = event_callbacks.size();
	for (size_t i = 0; i < listSize; ++i) {
		auto entry = event_callbacks[i];
		if (entry.cb || entry.fcb || entry.scb) {
			if (entry.event == event->event_id || entry.event == SYSTEM_EVENT_MAX) {
				if (entry.cb) {
					entry.cb((system_event_id_t)event->event_id);
				}
				if (entry.fcb) {
					entry.fcb((system_event_id_t)event->event_id, event->event_info);
				}
				if (entry.scb) {
					entry.scb(event);
				}
			}
		}
	}

	return 0;
}

int32_t WiFiGenericClass::channel() {

	// Wi-Fiが初期化されていない場合、0を返す
	if (!wifi_initialized) {
		return 0;
	}

	// 現在使用しているWi-Fiチャンネルを取得
	int primaryChan = 0;
	wifi_get_channel(&primaryChan);

	// 取得したチャンネル番号を返す
	return primaryChan;
}

bool WiFiGenericClass::mode(wifi_mode_t m) {

	// 現在のWi-Fiモードを取得
	wifi_mode_t cm = getMode();

	// 既に指定されたモードである場合は何もしない
	if (cm == m) {
		return true;
	}

	// 現在のモードが無効で、新しいモードが指定された場合
	if (!cm && m) {
		if (!WiFiStart()) {
			return false;
		}
	}
	// 現在のモードが有効で、新しいモードが無効の場合
	else if (cm && !m) {
		return true; // Wi-Fiインタフェースを停止する処理は省略し、trueを返す
	}

	// Wi-Fiインタフェースをオフにしてから、新しいモードで再起動
	wifi_off();
	vTaskDelay(20);
	auto err = wifi_on(m);
	if (err) {
		return false;
	}

	// 新しいモードを内部状態に反映
	_wifi_mode = m;

	return true;
}

wifi_mode_t WiFiGenericClass::getMode() {

	// Wi-Fiが初期化されていない場合、WIFI_MODE_NULLを返す
	if (!wifi_started) {
		return WIFI_MODE_NULL;
	}

	// 現在のWi-Fiモードを返す
	return _wifi_mode;
}

bool WiFiGenericClass::enableSTA(bool enable) {

	wifi_mode_t currentMode = getMode();
	bool isEnabled          = ((currentMode & WIFI_MODE_STA) != 0);

	if (isEnabled != enable) {
		if (enable) {
			return mode((wifi_mode_t)(currentMode | WIFI_MODE_STA));
		} else {
			return mode((wifi_mode_t)(currentMode & (~WIFI_MODE_STA)));
		}
	}

	return true;
}

bool WiFiGenericClass::enableAP(bool enable) {

	wifi_mode_t currentMode = getMode();
	bool isEnabled          = ((currentMode & WIFI_MODE_AP) != 0);

	if (isEnabled == enable) {
		return true;
	}

	if (enable) {
		return mode((wifi_mode_t)(currentMode | WIFI_MODE_AP));
	}

	return mode((wifi_mode_t)(currentMode & (~WIFI_MODE_AP)));
}

bool WiFiGenericClass::setSleep(bool enable) {

	return false;
}

bool WiFiGenericClass::getSleep() {

	// 現在の仕様に従って常にfalseを返す
	return false;
}

bool WiFiGenericClass::setTxPower(wifi_power_t power) {

	// ステーションまたはアクセスポイントが未開始の場合、falseを返す
	if ((getStatusBits() & (STA_STARTED_BIT | AP_STARTED_BIT)) == 0) {
		return false;
	}

	// 送信電力を設定
	_wifi_power = power;

	// 成功した場合はtrueを返す
	return true;
}

wifi_power_t WiFiGenericClass::getTxPower() {

	// ステーションまたはアクセスポイントが未開始の場合、デフォルトの送信電力を返す
	if ((getStatusBits() & (STA_STARTED_BIT | AP_STARTED_BIT)) == 0) {
		return WIFI_POWER_19_5dBm; // デフォルト値
	}

	// 現在の送信電力レベルを返す
	return _wifi_power;
}

// -----------------------------------------------------------------------------------------------------------------------
// ------------------------------------------------ Generic Network function ---------------------------------------------
// -----------------------------------------------------------------------------------------------------------------------

void wifi_dns_found_callback(const char *name, const ip_addr_t *ipaddr, void *callback_arg) {
	if (ipaddr)
    {
        (*reinterpret_cast<IPAddress *>(callback_arg)) = ipaddr->u_addr.ip4.addr;
    }

	// DNS解決完了をイベントで通知
	xEventGroupSetBits(wifi_event_group, WIFI_DNS_DONE_BIT);
}

int WiFiGenericClass::hostByName(const char *aHostname, IPAddress &aResult) {

	ip_addr_t addr;
	aResult = IPAddress(0, 0, 0, 0); // 初期化

	// DNSの待機
	if (!waitStatusBits(WIFI_DNS_IDLE_BIT, 5000)) {
		return 0;
	}
	clearStatusBits(WIFI_DNS_IDLE_BIT);

	// DNS解決の実行
	err_t err = dns_gethostbyname(aHostname, &addr, &wifi_dns_found_callback, &aResult);

	// 即座に解決できた場合
	if (err == ERR_OK && addr.u_addr.ip4.addr != 0) {
		aResult = IPAddress(
		    (addr.u_addr.ip4.addr) & 0xFF,
		    (addr.u_addr.ip4.addr >> 8) & 0xFF,
		    (addr.u_addr.ip4.addr >> 16) & 0xFF,
		    (addr.u_addr.ip4.addr >> 24) & 0xFF);
	}
	else if (err == ERR_INPROGRESS) {
		// DNS解決中の場合、待機
		if (!waitStatusBits(WIFI_DNS_DONE_BIT, 4000)) {
			return 0;
		}
		clearStatusBits(WIFI_DNS_DONE_BIT);
	}

	setStatusBits(WIFI_DNS_IDLE_BIT);

	// 最終確認
	return (aResult != IPAddress(0, 0, 0, 0)) ? 1 : 0;
}

IPAddress WiFiGenericClass::calculateNetworkID(IPAddress ip, IPAddress subnet) {

	IPAddress networkID;
	for (size_t i = 0; i < 4; i++) {
		networkID[i] = ip[i] & subnet[i];
	}
	return networkID;
}

IPAddress WiFiGenericClass::calculateBroadcast(IPAddress ip, IPAddress subnet) {

	IPAddress broadcastIp;
	for (size_t i = 0; i < 4; i++) {
		broadcastIp[i] = (~subnet[i] | ip[i]);
	}
	return broadcastIp;
}

uint8_t WiFiGenericClass::calculateSubnetCIDR(IPAddress subnetMask) {

	uint8_t CIDR = 0;
	for (uint8_t i = 0; i < 4; i++) {
		uint8_t maskByte = subnetMask[i];
		while (maskByte) {
			CIDR += (maskByte & 0x80) ? 1 : 0; // 先頭ビットが1ならCIDRを増加
			maskByte <<= 1;                    // 左シフトで次のビットを確認
		}
	}
	return CIDR;
}