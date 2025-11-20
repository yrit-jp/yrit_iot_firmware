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

#include "WiFiUdp.h"
#include "WiFi.h"
#include <lwip/sockets.h>
#include <lwip/netdb.h>

#ifdef write
#undef write
#endif

#ifdef read
#undef read
#endif

WiFiUDP::WiFiUDP() : udp_server(-1), server_port(0), remote_port(0),
                     tx_buffer(nullptr), tx_buffer_len(0), rx_buffer(nullptr) {}

WiFiUDP::~WiFiUDP() {
	stop();
}

uint8_t WiFiUDP::begin(IPAddress address, uint16_t port) {
	stop();
	server_port = port;

	tx_buffer = new (std::nothrow) char[1460]; // char* 型を使用
	if (!tx_buffer) {
		return 0;
	}

	udp_server = socket(AF_INET, SOCK_DGRAM, 0);
	if (udp_server < 0) {
		delete[] tx_buffer;
		tx_buffer = nullptr;
		return 0;
	}

	int optval = 1;
	if (setsockopt(udp_server, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
		close(udp_server);
		udp_server = -1;
		delete[] tx_buffer;
		tx_buffer = nullptr;
		return 0;
	}

	struct sockaddr_in server_addr = {};
	server_addr.sin_family         = AF_INET;
	server_addr.sin_addr.s_addr    = address;
	server_addr.sin_port           = htons(port);

	if (bind(udp_server, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
		close(udp_server);
		udp_server = -1;
		delete[] tx_buffer;
		tx_buffer = nullptr;
		return 0;
	}

	fcntl(udp_server, F_SETFL, O_NONBLOCK);
	return 1;
}

uint8_t WiFiUDP::begin(uint16_t p) {
	return begin(INADDR_ANY, p);
}

uint8_t WiFiUDP::beginMulticast(IPAddress a, uint16_t p) {
	if (!begin(INADDR_ANY, p)) {
		return 0;
	}

	if (a != 0) {
		struct ip_mreq mreq       = {};
		mreq.imr_multiaddr.s_addr = a;
		mreq.imr_interface.s_addr = INADDR_ANY;

		if (setsockopt(udp_server, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
			stop();
			return 0;
		}
		multicast_ip = a;
	}

	return 1;
}

void WiFiUDP::stop() {
	// 1. 送信バッファの解放
	if (tx_buffer != nullptr) {
		delete[] tx_buffer;
		tx_buffer = nullptr;
	}
	tx_buffer_len = 0;

	// 2. 受信バッファの解放
	if (rx_buffer != nullptr) {
		cbuf *b   = rx_buffer; // 一時変数に保存
		rx_buffer = nullptr;
		delete[] b; // メモリを解放
	}

	// 3. UDPソケットの処理
	if (udp_server == -1) {
		// ソケットが未初期化の場合は関数を終了
		return;
	}

	// 4. マルチキャスト設定の解除
	if (multicast_ip != 0) {
		struct ip_mreq mreq;
		memset(&mreq, 0, sizeof(mreq));
		mreq.imr_multiaddr.s_addr = multicast_ip; // マルチキャストアドレスを設定
		mreq.imr_interface.s_addr = 0;            // インターフェースアドレスに0を設定

		// マルチキャストグループから離脱
		setsockopt(udp_server, IPPROTO_IP, IP_DROP_MEMBERSHIP, (const void *)&mreq, sizeof(mreq));
		multicast_ip = INADDR_ANY; // マルチキャストIPをリセット
	}

	// 5. ソケットのクローズ
	closesocket(udp_server);
	udp_server = -1; // ソケットを無効化
}

int WiFiUDP::beginMulticastPacket() {
	// 1. 前提条件の確認
	if (server_port == 0 || multicast_ip == IPAddress(INADDR_ANY)) {
		// サーバーポートが設定されていないか、マルチキャストIPが未設定の場合
		return 0; // 失敗
	}

	// 2. リモートIPとリモートポートの設定
	remote_ip   = multicast_ip; // リモートIPアドレスにmulticast_ipを設定
	remote_port = server_port;  // リモートポートにserver_portを設定

	// 3. パケット送信開始
	return beginPacket();
}

int WiFiUDP::beginPacket() {
	// 1. リモートポートチェック
	if (remote_port == 0) {
		// リモートポートが設定されていない場合
		return 0; // 失敗
	}

	// 2. 送信バッファ割り当て
	if (tx_buffer == nullptr) {
		// tx_bufferが割り当てられていない場合、1460バイトのバッファを割り当て
		tx_buffer = new char[1460];
		if (tx_buffer == nullptr) {
			// バッファ割り当てに失敗
			return 0; // 失敗
		}
	}

	// 3. 送信バッファ長初期化
	tx_buffer_len = 0;

	// 4. ソケット状態確認
	if (udp_server != -1) {
		// ソケットが既に開かれている場合
		return 1; // 成功
	}

	// 5. ソケットオープン
	udp_server = socket(AF_INET, SOCK_DGRAM, 0);
	if (udp_server == -1) {
		// ソケットオープンに失敗
		return 0; // 失敗
	}

	// 6. ノンブロッキング設定
	fcntl(udp_server, F_SETFL, O_NONBLOCK);

	// 7. 成功
	return 1; // 成功
}

int WiFiUDP::beginPacket(IPAddress ip, uint16_t port) {
	// 1. リモートIP/ポート設定
	remote_ip   = ip;   // 引数ipをリモートIPに設定
	remote_port = port; // 引数portをリモートポートに設定

	// 2. パケット送信開始
	return beginPacket(); // 引数なしバージョンのbeginPacket()を呼び出して結果をそのまま返す
}

int WiFiUDP::beginPacket(const char *host, uint16_t port) {
	// 1. ホスト名解決
	struct hostent *server = gethostbyname(host);
	if (server == NULL) {
		// ホスト名の解決に失敗した場合
		return 0; // 失敗
	}

	// 2. IPアドレス取得
	remote_ip   = IPAddress((const uint8_t *)(server->h_addr_list[0]));
	remote_port = port; // ポート番号を設定

	// 3. パケット送信開始 (IPアドレス版)
	return beginPacket(remote_ip, remote_port); // IPアドレス版beginPacketを呼び出す
}

int WiFiUDP::endPacket() {
	// 1. 送信先アドレス設定
	struct sockaddr_in recipient;
	recipient.sin_addr.s_addr = static_cast<uint32_t>(remote_ip);   // remote_ipをネットワークバイトオーダーに変換して設定
	recipient.sin_family      = AF_INET;            // IPv4
	recipient.sin_port        = htons(remote_port); // remote_portをネットワークバイトオーダーに変換して設定

	// 2. パケット送信
	ssize_t sent = sendto(udp_server, tx_buffer, tx_buffer_len, 0, (struct sockaddr *)&recipient, sizeof(recipient));

	// 3. 送信結果判定
	if (sent < 0) {
		// 送信に失敗した場合
		return 0; // 失敗
	}

	// 4. 成功
	return 1; // 成功
}

size_t WiFiUDP::write(uint8_t data) {
	// 1. 送信バッファ満杯チェック
	if (tx_buffer_len >= 1460) {
		// バッファが満杯の場合、endPacket() を呼び出して送信
		if (endPacket() == 0) {
			// 送信に失敗した場合
			return 0; // 失敗
		}
		// 送信後、バッファをリセット
		tx_buffer_len = 0;
	}

	// 2. データ書き込み
	tx_buffer[tx_buffer_len++] = data;

	// 3. 常に1を返す
	return 1;
}

size_t WiFiUDP::write(const uint8_t *buffer, size_t size) {
	// 1. データ送信ループ
	for (size_t i = 0; i < size; i++) {
		// 1バイト書き込み
		if (write(buffer[i]) == 0) {
			// 送信に失敗した場合は0を返す
			return 0;
		}
	}

	// 2. 戻り値
	return size; // 実際に書き込まれたバイト数（sizeと等しい）
}

int WiFiUDP::parsePacket() {
	// 1. 既に rx_buffer が存在する場合は 0 を返す
	if (rx_buffer) {
		return 0;
	}

	// 2. 受信処理
	struct sockaddr_in sender_addr;
	socklen_t sender_addr_len = sizeof(sender_addr);

	char *temp_buffer = new (std::nothrow) char[1460];
	if (!temp_buffer) {
		return 0; // メモリ確保失敗
	}

	int received_size = recvfrom(udp_server, temp_buffer, 1460, MSG_DONTWAIT,
	                             (struct sockaddr *)&sender_addr, &sender_addr_len);

	if (received_size < 0) {
		delete[] temp_buffer;
		if (errno == EWOULDBLOCK) {
			return 0; // 受信データなし
		}
		return 0; // その他のエラー
	}

	// 3. リモート情報の保存
	remote_ip   = IPAddress(sender_addr.sin_addr.s_addr);
	remote_port = ntohs(sender_addr.sin_port);

	// 4. 受信データの処理
	if (received_size > 0) {
		rx_buffer = new (std::nothrow) cbuf(received_size);
		if (!rx_buffer) {
			delete[] temp_buffer;
			return 0; // メモリ確保失敗
		}
		rx_buffer->write(temp_buffer, received_size);
	}

	// 5. 一時バッファの解放
	delete[] temp_buffer;
	return received_size;
}

int WiFiUDP::available() {
	if (!rx_buffer) {
		return 0;
	}
	return rx_buffer->available();
}

int WiFiUDP::read() {
	if (!rx_buffer) {
		return -1;
	}

	int data = rx_buffer->read();

	if (rx_buffer->available() == 0) {
		cbuf *temp = rx_buffer;
		rx_buffer  = nullptr;
		delete temp;
	}

	return data;
}

int WiFiUDP::read(unsigned char *buffer, size_t len) {
	return read(reinterpret_cast<char *>(buffer), len);
}

int WiFiUDP::read(char *buffer, size_t len) {
	if (!rx_buffer) {
		return 0;
	}

	int bytesRead = rx_buffer->read(buffer, len);

	if (rx_buffer->available() == 0) {
		cbuf *temp = rx_buffer;
		rx_buffer  = nullptr;
		delete temp;
	}

	return bytesRead;
}

int WiFiUDP::peek() {
	if (!rx_buffer)
		return -1;
	return rx_buffer->peek();
}

void WiFiUDP::flush() {
	if (!rx_buffer) {
		return;
	}

	cbuf *b   = rx_buffer;
	rx_buffer = nullptr;
	delete b;
}

void WiFiUDP::clear() {
	if (!rx_buffer) {
		return;
	}

	cbuf *b   = rx_buffer;
	rx_buffer = nullptr;
	delete b;
}

IPAddress WiFiUDP::remoteIP() {
	return remote_ip;
}

uint16_t WiFiUDP::remotePort() {
	return remote_port;
}
