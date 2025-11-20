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

#include "WiFiClient.h"
#include "WiFi.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"

#define WIFI_CLIENT_MAX_WRITE_RETRY (10)
#define WIFI_CLIENT_SELECT_TIMEOUT_US (1000000)
#define WIFI_CLIENT_FLUSH_BUFFER_SIZE (1024)
#define WIFI_CLIENT_KEEPALIVE_TIMEOUT (500)

#undef connect
#undef write
#undef read

class WiFiClientRxBuffer {
private:
	size_t _size;
	uint8_t *_buffer;
	size_t _pos;
	size_t _fill;
	int _fd;
	bool _failed;

	size_t r_available() {
		if (_fd < 0) {
			return 0;
		}
		size_t received = fillBuffer();
		if (received < 0) {
			_failed = true;
			return 0;
		}
		return received;
	}

	size_t fillBuffer() {
		if (!_buffer) {
			_buffer = (uint8_t *)malloc(_size);
			if (!_buffer) {
				_failed = true;
				return 0;
			}
		}

		if (_fill != 0 && _pos == _fill) {
			_fill = 0;
			_pos  = 0;
		}

		if (!_buffer || _size <= _fill) {
			return 0;
		}

		int received = recv(_fd, _buffer + _fill, _size - _fill, MSG_DONTWAIT);
		if (received < 0) {
			if (errno != EWOULDBLOCK) {
				_failed = true;
			}
			return 0;
		}

		_fill += received;
		return received;
	}

public:
	WiFiClientRxBuffer(int fd, size_t size = 1024)
	    : _size(size), _buffer(nullptr), _pos(0), _fill(0), _fd(fd), _failed(false) {}

	~WiFiClientRxBuffer() {
		free(_buffer);
	}

	bool failed() const {
		return _failed;
	}

	int read(uint8_t *dst, size_t len) {
		// 入力パラメータの検証
		if (dst == nullptr || len == 0) {
			return -1;
		}

		if (_pos == _fill && fillBuffer() == 0) {
			return -1;
		}

		// 現在のバッファ内データ量の計算
		size_t a = _fill - _pos; // 現在の読み出し可能バイト数

		// 一括読み出し可能か判定
		if (len <= a || (len - a) <= (_size - _fill) && fillBuffer() >= (len - a)) {
			// 一括読み出し処理
			if (len == 1) {
				// 1バイトの場合、直接代入
				*dst = _buffer[_pos];
			} else {
				// 複数バイトの場合、一括コピー
				memcpy(dst, _buffer + _pos, len);
			}
			_pos += len;
			return len;
		} else {
			// 分割読み出し処理
			size_t left   = len; // 残り読み出しバイト数
			size_t toRead = a;   // 初回の読み出し量
			uint8_t *buf  = dst; // バッファの先頭アドレス保存

			// 初回データコピー
			memcpy(buf, _buffer + _pos, toRead);
			_pos += toRead;
			left -= toRead;
			buf += toRead;

			// 残り読み出しがある間ループ
			while (left > 0) {
				if (fillBuffer() == 0) {
					// データ補充に失敗した場合、部分的に読み出した量を返す
					return len - left;
				}

				a      = _fill - _pos;      // 新しい読み出し可能量
				toRead = std::min(a, left); // 読み出し量を決定

				// データコピー
				memcpy(buf, _buffer + _pos, toRead);
				_pos += toRead;
				left -= toRead;
				buf += toRead;
			}

			// 全てのデータを読み出し完了したので要求長lenを返す
			return len;
		}
	}

	int peek() {
		// エラー判定: バッファが空かつfillBuffer()が失敗した場合
		if (_pos == _fill && fillBuffer() == 0) {
			return -1; // エラー
		}

		// 正常時: 現在位置(_pos)のバッファデータを返す
		return _buffer[_pos]; // バッファ位置は進めない
	}

	size_t available() {
		return (_fill - _pos) ? (_fill - _pos) : r_available();
	}

	void clear() {
		if (r_available()) {
			fillBuffer();
		}
		_pos = _fill;
	}
};

class WiFiClientSocketHandle {
private:
	int sockfd; // ソケットファイルディスクリプタ

public:
	// コンストラクタ: fdでsockfdを初期化
	WiFiClientSocketHandle(int fd) : sockfd(fd) {}

	// デストラクタ: lwip_closeでsockfdをクローズ
	~WiFiClientSocketHandle() {
		lwip_close(sockfd); // lwIPのclose関数を呼び出してソケットをクローズ
	}

	// メソッド: fdを返す
	int fd() {
		return sockfd; // sockfdの値を返す
	}
};

WiFiClient::WiFiClient() : _connected(false), next(NULL) {
	// 初期化処理はコンストラクタ内で行います
}

// ファイルディスクリプタを受け取るコンストラクタ
WiFiClient::WiFiClient(int fd) : _connected(true), next(NULL) {
	// 既存のshared_ptrをresetして新しいWiFiClientSocketHandleを設定
	clientSocketHandle.reset(new WiFiClientSocketHandle(fd));

	// 同様に、WiFiClientRxBufferをresetして新しいインスタンスを設定
	_rxBuffer.reset(new WiFiClientRxBuffer(fd));
}

// デストラクタ
WiFiClient::~WiFiClient() {
	// stopメソッドを呼び出して接続を閉じる
	stop();
}

// コピー代入演算子の実装
WiFiClient &WiFiClient::operator=(const WiFiClient &other) {
	stop(); // 現在の接続を切断

	// コピー元からのメンバ変数のコピー
	clientSocketHandle = other.clientSocketHandle;
	_rxBuffer          = other._rxBuffer;
	_connected         = other._connected;

	return *this; // 連鎖代入を可能にするため、*thisを返す
}

// stopメソッドの実装
void WiFiClient::stop() {
	// clientSocketHandleのリソースを解放し、NULLに設定
	clientSocketHandle.reset();
	clientSocketHandle = NULL;

	// _rxBufferのリソースを解放し、NULLに設定
	_rxBuffer.reset();
	_rxBuffer = NULL;

	// 接続状態を切断として設定
	_connected = false;
}

int WiFiClient::connect(IPAddress ip, uint16_t port, int32_t timeout) {
	// ソケットの作成
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		return 0; // ソケット作成失敗
	}

	// ソケットをノンブロッキングモードに設定
	fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFL, 0) | O_NONBLOCK);

	// 接続先アドレスの設定
	uint32_t ip_addr = ip;
	struct sockaddr_in serveraddr;
	bzero((char *)&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	bcopy((const void *)(&ip_addr), (void *)&serveraddr.sin_addr.s_addr, 4);
	serveraddr.sin_port = htons(port);

	// 接続試行
	int res = lwip_connect_r(sockfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr));
	if (res < 0 && errno != EINPROGRESS) {
		lwip_close(sockfd); // 接続失敗
		return 0;
	}

	// 接続完了待ち
	fd_set fdset;
	FD_ZERO(&fdset);
	FD_SET(sockfd, &fdset);

	struct timeval tv;
	tv.tv_sec  = 0;
	tv.tv_usec = timeout * 1000;

	res = lwip_select(sockfd + 1, nullptr, &fdset, nullptr, timeout < 0 ? nullptr : &tv);
	if (res <= 0) {
		lwip_close(sockfd); // タイムアウトまたはエラー
		return 0;
	}

	// エラー確認
	int sockerr   = 0;
	socklen_t len = (socklen_t)sizeof(int);
	res           = getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &sockerr, &len);
	if (res < 0 || sockerr != 0) {
		lwip_close(sockfd); // 接続エラー
		return 0;
	}

	// ソケットモードの復帰
	fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFL, 0) & (~O_NONBLOCK));

	// 接続情報の保存
	clientSocketHandle.reset(new WiFiClientSocketHandle(sockfd));
	_rxBuffer.reset(new WiFiClientRxBuffer(sockfd));
	conn_status = millis();
	_connected  = true;

	return 1; // 接続成功
}

int WiFiClient::connect(IPAddress ip, uint16_t port) {
	// タイムアウト無期限（-1）で別の connect メソッドを呼び出す
	return connect(ip, port, -1);
}

int WiFiClient::connect(const char *host, uint16_t port) {
	// タイムアウト無期限（-1）で別の connect メソッドを呼び出す
	return connect(host, port, -1);
}

int WiFiClient::connect(const char *host, uint16_t port, int32_t timeout) {
	// IPAddress 型の変数 srv を宣言し、0で初期化
	IPAddress srv((uint32_t)0);

	// ホスト名をIPアドレスに解決
	if (!WiFiGenericClass::hostByName(host, srv)) {
		// 解決に失敗した場合は接続失敗を返す
		return 0;
	}

	// 解決されたIPアドレス、ポート番号、タイムアウトを指定して接続を試みる
	return connect(srv, port, timeout);
}

int WiFiClient::setSocketOption(int option, char *value, size_t len) {
	// このWiFiClientオブジェクトに関連付けられたソケットのファイルディスクリプタを取得
	int sockfd = fd();

	// setsockopt を呼び出して、指定されたソケットオプションを設定
	int res = setsockopt(sockfd, SOL_SOCKET, option, value, len);

	return res;
}

int WiFiClient::setTimeout(uint32_t seconds) {
	// 基底クラス Client の setTimeout をミリ秒単位で呼び出す
	Client::setTimeout(seconds * 1000);

	// timeval 構造体にタイムアウト時間を設定
	struct timeval tv;
	tv.tv_sec  = seconds;
	tv.tv_usec = 0;

	// 受信タイムアウトを設定
	if (setSocketOption(SO_RCVTIMEO, (char *)&tv, sizeof(struct timeval)) < 0) {
		return -1;
	}

	// 送信タイムアウトを設定
	return setSocketOption(SO_SNDTIMEO, (char *)&tv, sizeof(struct timeval));
}

int WiFiClient::setOption(int option, int *value) {
	// ソケットのファイルディスクリプタを取得
	int sock = fd();

	// TCP ソケットオプションを設定
	int res = setsockopt(sock, IPPROTO_TCP, option, (char *)value, sizeof(int));
	return res;
}

int WiFiClient::getOption(int option, int *value) {
	// ソケットのファイルディスクリプタを取得
	int sock = fd();

	// オプション値のサイズを指定
	socklen_t size = (socklen_t)sizeof(int);

	// TCP ソケットオプションを取得
	int res = getsockopt(sock, IPPROTO_TCP, option, (char *)value, &size);
	return res;
}

int WiFiClient::setNoDelay(bool nodelay) {
	int flag = nodelay;
	return setOption(TCP_NODELAY, &flag);
}

bool WiFiClient::getNoDelay() {
	int flag = 0;
	getOption(TCP_NODELAY, &flag);
	return flag;
}

size_t WiFiClient::write(uint8_t data) {
	return write(&data, 1);
}

int WiFiClient::read() {
	uint8_t data = 0;
	int res      = read(&data, 1);
	if (res < 0) {
		return res;
	}
	return data;
}

size_t WiFiClient::write(const uint8_t *buf, size_t size) {
	int res                  = 0;
	int retry                = WIFI_CLIENT_MAX_WRITE_RETRY;
	int socketFileDescriptor = fd();
	size_t totalBytesSent    = 0;
	size_t bytesRemaining    = size;

	if (!_connected || (socketFileDescriptor < 0)) {
		return 0;
	}

	while (retry) {
		fd_set set;
		struct timeval tv;

		FD_ZERO(&set);
		FD_SET(socketFileDescriptor, &set);

		tv.tv_sec  = 0;
		tv.tv_usec = WIFI_CLIENT_SELECT_TIMEOUT_US;

		retry--;

		if (lwip_select(socketFileDescriptor + 1, NULL, &set, NULL, &tv) < 0) {
			return 0;
		}

		if (FD_ISSET(socketFileDescriptor, &set)) {
			res = send(socketFileDescriptor, (void *)buf, bytesRemaining, MSG_DONTWAIT);
			if (res > 0) {
				totalBytesSent += res;
				if (totalBytesSent >= size) {
					retry = 0;
				} else {
					buf += res;
					bytesRemaining -= res;
					retry = WIFI_CLIENT_MAX_WRITE_RETRY;
				}
			} else if (res < 0) {
				if (errno != EAGAIN) {
					stop();
					res   = 0;
					retry = 0;
				}
			}
		}
	}

	return totalBytesSent;
}

size_t WiFiClient::write_P(PGM_P buf, size_t size) {
	return write(buf, size);
}

size_t WiFiClient::write(Stream &stream) {
	uint8_t *buf = (uint8_t *)malloc(1360);
	if (!buf) {
		return 0;
	}

	size_t toRead    = 0;
	size_t toWrite   = 0;
	size_t written   = 0;
	size_t available = stream.available();

	while (available) {
		if (available > 1360) {
			toRead = 1360;
		} else {
			toRead = available;
		}

		toWrite = stream.readBytes(buf, toRead);
		written += write(buf, toWrite);
		available = stream.available();
	}

	free(buf);
	return written;
}

int WiFiClient::read(uint8_t *buf, size_t size) {
	int res = -1;

	res = _rxBuffer->read(buf, size);

	if (_rxBuffer->failed()) {
		stop();
	}

	return res;
}

int WiFiClient::peek() {
	int res = _rxBuffer->peek();

	if (_rxBuffer->failed()) {
		stop();
	}

	return res;
}

int WiFiClient::available() {
	if (!_rxBuffer) {
		return 0;
	}

	int res = _rxBuffer->available();

	if (_rxBuffer->failed()) {
		stop();
	}

	return res;
}

void WiFiClient::flush() {
	if (_rxBuffer != nullptr) {
		_rxBuffer->clear();
	}
}

void WiFiClient::clear() {
	if (_rxBuffer != nullptr) {
		_rxBuffer->clear();
	}
}

uint8_t WiFiClient::connected() {
	uint32_t interval = millis() - conn_status;

	if (_connected && interval > WIFI_CLIENT_KEEPALIVE_TIMEOUT) {
		uint8_t dummy;
		int res = recv(fd(), &dummy, 0, MSG_DONTWAIT);
		(void)res;

		switch (errno) {
		case EWOULDBLOCK:
		case ENOENT:
			conn_status = millis();
			_connected  = true;
			break;
		case ENOTCONN:
		case EPIPE:
		case ECONNRESET:
		case ECONNREFUSED:
		case ECONNABORTED:
			_connected = false;
			break;
		default:
			conn_status = millis();
			_connected  = true;
			break;
		}
	}

	return _connected;
}

IPAddress WiFiClient::remoteIP(int fd) const {
	struct sockaddr_storage addr;
	socklen_t len = sizeof(addr);

	// リモートアドレス情報を取得
	getpeername(fd, (struct sockaddr *)&addr, &len);

	// IPv4 アドレスの場合
	struct sockaddr_in *s = (struct sockaddr_in *)&addr;
	return IPAddress((uint32_t)(s->sin_addr.s_addr));
}

uint16_t WiFiClient::remotePort(int fd) const {
	struct sockaddr_storage addr;
	socklen_t len = sizeof(addr);

	// リモートアドレス情報を取得
	getpeername(fd, (struct sockaddr *)&addr, &len);

	// IPv4 アドレスの場合
	struct sockaddr_in *s = (struct sockaddr_in *)&addr;
	return ntohs(s->sin_port);
}

IPAddress WiFiClient::remoteIP() const {
	return remoteIP(fd());
}

uint16_t WiFiClient::remotePort() const {
	return remotePort(fd());
}

IPAddress WiFiClient::localIP(int fd) const {
	struct sockaddr_storage addr;
	socklen_t len = sizeof(addr);

	// ローカルアドレス情報を取得
	getsockname(fd, (struct sockaddr *)&addr, &len);

	// IPv4 アドレスの場合
	struct sockaddr_in *s = (struct sockaddr_in *)&addr;
	return IPAddress((uint32_t)(s->sin_addr.s_addr));
}

uint16_t WiFiClient::localPort(int fd) const {
	struct sockaddr_storage addr;
	socklen_t len = sizeof(addr);

	// ローカルアドレス情報を取得
	getsockname(fd, (struct sockaddr *)&addr, &len);

	// IPv4 アドレスの場合
	struct sockaddr_in *s = (struct sockaddr_in *)&addr;
	return ntohs(s->sin_port);
}

IPAddress WiFiClient::localIP() const {
	return localIP(fd());
}

uint16_t WiFiClient::localPort() const {
	return localPort(fd());
}

bool WiFiClient::operator==(const WiFiClient &rhs) {
	return clientSocketHandle == rhs.clientSocketHandle &&
	       remotePort() == rhs.remotePort() &&
	       remoteIP() == rhs.remoteIP();
}

int WiFiClient::fd() const {
	return (clientSocketHandle == NULL) ? -1 : clientSocketHandle->fd();
}

WiFiClient::operator bool() {
	return connected();
}

bool WiFiClient::operator==(const bool value) {
	return bool() == value;
}

bool WiFiClient::operator!=(const bool value) {
	return bool() != value;
}

bool WiFiClient::operator!=(const WiFiClient &rhs) {
	return !this->operator==(rhs);
}
