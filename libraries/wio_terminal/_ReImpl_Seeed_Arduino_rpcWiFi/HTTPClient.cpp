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

#include <Arduino.h>
#include <base64.h>
#include <HardwareSerial.h>
#include "HTTPClient.h"

HTTPClient::~HTTPClient() {
	if (_client) {
		_client->stop();
	}
	if (_currentHeaders) {
		delete[] _currentHeaders;
	}
}

void HTTPClient::clear() {
	_returnCode = 0;
	_size       = -1;
	_headers    = "";
}

bool HTTPClient::begin(WiFiClient &client, String url) {
	_client = &client;

	int colonIndex = url.indexOf(':');
	if (colonIndex == -1) {
		return false;
	}

	String protocol = url.substring(0, colonIndex);
	if (protocol != "http" && protocol != "https") {
		return false;
	}

	_port = (protocol == "https") ? 443 : 80;

	return beginInternal(url, protocol.c_str());
}

bool HTTPClient::begin(WiFiClient &client, String host, uint16_t port, String uri, bool https) {
	_client = &client;
	clear();

	_host     = host;
	_port     = port;
	_uri      = uri;
	_protocol = https ? "https" : "http";

	return true;
}

bool HTTPClient::beginInternal(String url, const char *expectedProtocol) {
	clear();

	int colonIndex = url.indexOf(':');
	if (colonIndex == -1) {
		return false;
	}

	_protocol = url.substring(0, colonIndex);
	if (_protocol != expectedProtocol) {
		return false;
	}

	url = url.substring(colonIndex + 3); // Remove "http://" or "https://"

	int slashIndex  = url.indexOf('/');
	String hostPart = (slashIndex == -1) ? url : url.substring(0, slashIndex);
	_uri            = (slashIndex == -1) ? "/" : url.substring(slashIndex);

	int atIndex = hostPart.indexOf('@');
	if (atIndex != -1) {
		String authInfo      = hostPart.substring(0, atIndex);
		hostPart             = hostPart.substring(atIndex + 1);
		_base64Authorization = base64::encode(authInfo);
	}

	int portIndex = hostPart.indexOf(':');
	if (portIndex != -1) {
		_host = hostPart.substring(0, portIndex);
		_port = hostPart.substring(portIndex + 1).toInt();
	} else {
		_host = hostPart;
	}

	return true;
}

void HTTPClient::end() {
	disconnect(false);
	clear();
}

void HTTPClient::disconnect(bool preserveClient) {
	if (connected()) {
		while (_client->available() > 0) {
			_client->read();
		}

		if (!(_reuse && _canReuse)) {
			_client->stop();
			if (!preserveClient) {
				_client = nullptr;
			}
		}
	}
}

bool HTTPClient::connected() {
	if (_client) {
		return (_client->available() > 0 || _client->connected());
	}
	return false;
}

void HTTPClient::setReuse(bool reuse) {
	_reuse = reuse;
}

void HTTPClient::setUserAgent(const String &userAgent) {
	_userAgent = userAgent;
}

void HTTPClient::setAuthorization(const char *user, const char *password) {
	if (user && password) {
		String auth          = String(user) + ":" + String(password);
		_base64Authorization = base64::encode(auth);
	}
}

void HTTPClient::setAuthorization(const char *auth) {
	if (auth) {
		_base64Authorization = auth;
	}
}

void HTTPClient::setConnectTimeout(int32_t connectTimeout) {
	_connectTimeout = connectTimeout;
}

void HTTPClient::setTimeout(uint16_t timeout) {
	_tcpTimeout = timeout;

	if (connected()) {
		_client->setTimeout((timeout + 500) / 1000); // Convert to seconds
	}
}

void HTTPClient::useHTTP10(bool useHTTP10) {
	_useHTTP10 = useHTTP10;
	_reuse     = !useHTTP10;
}

int HTTPClient::GET() {
	return sendRequest("GET");
}

// バージョン 1: uint8_t * payload と size を使った POST メソッド
int HTTPClient::POST(uint8_t *payload, size_t size) {
	return sendRequest("POST", payload, size);
}

// バージョン 2: String payload を使った POST メソッド
int HTTPClient::POST(String payload) {
	return POST((uint8_t *)payload.c_str(), payload.length());
}

// バージョン 1: uint8_t * payload と size を使った PATCH メソッド
int HTTPClient::PATCH(uint8_t *payload, size_t size) {
	return sendRequest("PATCH", payload, size);
}

// バージョン 2: String payload を使った PATCH メソッド
int HTTPClient::PATCH(String payload) {
	return PATCH((uint8_t *)payload.c_str(), payload.length());
}

// バージョン 1: uint8_t * payload と size を使った PUT メソッド
int HTTPClient::PUT(uint8_t *payload, size_t size) {
	return sendRequest("PUT", payload, size);
}

// バージョン 2: String payload を使った PUT メソッド
int HTTPClient::PUT(String payload) {
	return PUT((uint8_t *)payload.c_str(), payload.length());
}

int HTTPClient::sendRequest(const char *type, String payload) {
	return sendRequest(type, (uint8_t *)payload.c_str(), payload.length());
}

int HTTPClient::sendRequest(const char *type, uint8_t *payload, size_t size) {
	int code               = 0;
	bool redirect          = false;
	uint16_t redirectCount = 0;

	// 2. リダイレクト処理のメインループ
	do {
		// A. 既存ヘッダのクリア処理
		for (int i = 0; i < _headerKeysCount; i++) {
			if (_currentHeaders[i].value.length() > 0) {
				_currentHeaders[i].value.remove(i, sizeof(_currentHeaders[i].value));
			}
		}

		// B. サーバー接続処理
		if (!connect()) {
			return returnError(HTTPC_ERROR_CONNECTION_REFUSED);
		}

		// C. ヘッダ設定処理
		if (payload != nullptr && size > 0) {
			addHeader("Content-Length", String(size));
		}

		// D. リクエスト送信処理
		if (!sendHeader(type)) {
			return returnError(HTTPC_ERROR_SEND_HEADER_FAILED);
		}

		if (payload != nullptr && size > 0) {
			size_t pos = 0;
			while (pos < size) {
				size_t to_write = min(HTTP_TCP_BUFFER_SIZE, size - pos);
				if (_client->write(payload + pos, to_write) != to_write) {
					return returnError(HTTPC_ERROR_SEND_PAYLOAD_FAILED);
				}
				pos += to_write;
			}
		}

		// E. レスポンス処理
		code = handleHeaderResponse();

		// F. リダイレクト判定・処理
		if (_followRedirects != HTTPC_DISABLE_FOLLOW_REDIRECTS && redirectCount < _redirectLimit) {
			// リダイレクト先URLが存在する場合
			if (_location.length() > 0) {
				redirectCount++;

				// 1. 301(MOVED_PERMANENTLY)または307(TEMPORARY_REDIRECT)の場合
				if (code == HTTP_CODE_MOVED_PERMANENTLY || code == HTTP_CODE_TEMPORARY_REDIRECT) {
					// 同一メソッドでリダイレクト
					if (_followRedirects == HTTPC_FORCE_FOLLOW_REDIRECTS || strcmp(type, "GET") == 0 || strcmp(type, "HEAD") == 0) {
						if (!setURL(_location)) {
							continue;
						}
						redirect = true; // 再リクエストフラグを立てる
						continue;        // 同一メソッド・ペイロードで再リクエスト
					}
				}

				// 2. 302(FOUND)または303(SEE_OTHER)の場合
				if (code == HTTP_CODE_FOUND || code == HTTP_CODE_SEE_OTHER) {
					if (!setURL(_location)) {
						continue;
					}
					type     = "GET";   // メソッドをGETに変更
					payload  = nullptr; // ペイロードをnullに設定
					size     = 0;       // ペイロードサイズを0に設定
					redirect = true;    // 再リクエストフラグを立てる
					continue;           // GETメソッドで再リクエスト
				}
			}
		}

		break; // リダイレクトが不要またはリダイレクト回数が上限に達した場合
	} while (redirect);

	// 最終処理
	return returnError(code);
}

int HTTPClient::sendRequest(const char *type, Stream *stream, size_t size) {
	if (!stream) {
		return returnError(HTTPC_ERROR_NO_STREAM);
	}

	if (!connect()) {
		return returnError(HTTPC_ERROR_CONNECTION_REFUSED);
	}

	if (size > 0) {
		addHeader("Content-Length", String(size));
	}

	if (!sendHeader(type)) {
		return returnError(HTTPC_ERROR_SEND_HEADER_FAILED);
	}

	int buff_size    = HTTP_TCP_BUFFER_SIZE;
	int len          = size;
	int bytesWritten = 0;

	if (len == 0) {
		len = -1;
	}

	if (len > 0 && len < HTTP_TCP_BUFFER_SIZE) {
		buff_size = len;
	}

	uint8_t *buff = (uint8_t *)malloc(buff_size);
	if (!buff) {
		return returnError(HTTPC_ERROR_TOO_LESS_RAM);
	}

	int sizeAvailable = 0;
	int readBytes     = 0;
	int bytesRead     = 0;

	// データ送信処理のループ
	while (connected() && (stream->available() > -1) && (len > 0 || len == -1)) {
		sizeAvailable = stream->available(); // availableの結果を格納

		// sizeAvailableが0の場合はdelay(1)を実行して次のループに移動
		if (sizeAvailable == 0) {
			delay(1);
			continue;
		}

		// readBytesにsizeAvailableを代入、ただしlenより大きい場合はlenを代入
		readBytes = (len > 0 && sizeAvailable > len) ? len : sizeAvailable;

		// readBytesがbuff_sizeより大きい場合はbuff_sizeを代入
		if (readBytes > buff_size) {
			readBytes = buff_size;
		}

		// bytesReadにstream->readBytesの結果を格納
		bytesRead = stream->readBytes(buff, readBytes);

		int bytesWrite = _client->write((const uint8_t *)buff, bytesRead); // _client->writeの結果を格納
		bytesWritten += bytesWrite;

		if (bytesWrite != bytesRead) { // bytesWriteとbytesReadが一致しない場合
			// エラーチェックとエラーのクリア
			if (_client->getWriteError()) {
				_client->clearWriteError();
			}
			delay(1); // 少し遅延を挟む

			int leftBytes = readBytes - bytesWrite;                                          // 残りの書き込みバイト数
			bytesWrite    = _client->write((const uint8_t *)(buff + bytesWrite), leftBytes); // 残りのデータを書き込み

			bytesWritten += bytesWrite;

			if (bytesWrite != leftBytes) {                           // まだ書き込みが完了しない場合
				free(buff);                                          // buffを開放
				return returnError(HTTPC_ERROR_SEND_PAYLOAD_FAILED); // エラーを返す
			}
		}

		// エラーが発生した場合、buffを開放してエラーを返す
		if (_client->getWriteError() != 0) {
			free(buff);                                          // エラーが発生した場合はbuffを開放
			return returnError(HTTPC_ERROR_SEND_PAYLOAD_FAILED); // エラーを返す
		}

		// lenが0より大きい場合は len から readBytes を引き、lenに代入する
		if (len > 0) {
			len -= readBytes;
		}
	}

	free(buff);

	// sizeが正かつsizeとbytesWrittenが不一致の場合、エラーを返す
	if (size > 0 && size != bytesWritten) {
		return returnError(HTTPC_ERROR_SEND_PAYLOAD_FAILED);
	}

	return returnError(handleHeaderResponse()); // handleHeaderResponse()の結果を返す
}

int HTTPClient::getSize(void) {
	return _size;
}

WiFiClient &HTTPClient::getStream() {
	static WiFiClient empty; // 静的な空のWiFiClientオブジェクト
	if (connected()) {
		return *_client; // 接続されている場合、内部のWiFiClientオブジェクトを参照返す
	}
	return empty; // 接続されていない場合、空のWiFiClientオブジェクトを返す
}

WiFiClient *HTTPClient::getStreamPtr() {
	if (connected()) {
		return _client; // 接続されている場合、内部のWiFiClientオブジェクトのポインタを返す
	}
	return nullptr; // 接続されていない場合、nullポインタを返す
}

int HTTPClient::writeToStream(Stream *stream) {
	// 1. 初期検証
	if (!stream) {
		return returnError(HTTPC_ERROR_NO_STREAM); // NULLの場合
	}

	if (!connected()) {
		return returnError(HTTPC_ERROR_NOT_CONNECTED); // 未接続の場合
	}

	size_t len = _size;
	int ret    = 0;

	// 2. Transfer-Encoding: Identity の処理
	if (_transferEncoding == HTTPC_TE_IDENTITY) {
		ret = writeToStreamDataBlock(stream, len);
		if (ret < 0) {
			return returnError(ret); // エラーの場合
		}
	}
	// 3. Transfer-Encoding: Chunked の処理
	else if (_transferEncoding == HTTPC_TE_CHUNKED) {
		size_t size = 0;
		while (true) {
			if (!connected()) {
				return returnError(HTTPC_ERROR_CONNECTION_LOST); // 接続切れ
			}

			String chunkHeader = _client->readStringUntil('\n');
			if (chunkHeader.length() <= 0) {
				return returnError(HTTPC_ERROR_READ_TIMEOUT); // 読み取り失敗
			}

			chunkHeader.trim(); // 末尾の\rを削除
			long chunkSize = strtol(chunkHeader.c_str(), nullptr, 16);
			size += chunkSize; // strtolの直後にサイズ加算

			if (chunkSize > 0) {
				int blockRet = writeToStreamDataBlock(stream, chunkSize); // 戻り値を新しい変数に代入
				if (blockRet < 0) {
					return returnError(blockRet); // エラーの場合
				}
				ret += blockRet;
			} else {
				// 最終チャンク
				if (_size < 0) {
					_size = size; // Content-Length未設定の場合
				}

				if (ret != _size) {
					return returnError(HTTPC_ERROR_STREAM_WRITE); // サイズ不一致
				}
				break; // チャンク終了
			}

			// チャンク終端処理
			uint8_t end[2];
			int bytesRead = _client->readBytes(end, 2);
			if (bytesRead != 2 || end[0] != '\r' || end[1] != '\n') {
				return returnError(HTTPC_ERROR_READ_TIMEOUT); // 終端不正
			}

			delay(0); // 他のタスクに実行機会を与える
		}
	}
	// 4. 未対応エンコーディングの処理
	else {
		return returnError(HTTPC_ERROR_ENCODING); // 未対応の場合
	}

	// 5. 終了処理
	disconnect(true); // クライアントを切断
	return ret;       // 書き込んだバイト数
}

StreamString HTTPClient::getString(void) {
	StreamString sstring;

	// _sizeが0より大きいか、または-1の場合は処理を行う
	if (_size > 0 || _size == -1) {
		// メモリ予約
		if (sstring.reserve(_size + 1)) { // 終端のNULL文字分を+1
			writeToStream(&sstring);      // データ書き込み
			return sstring;               // 書き込んだデータを返す
		}
	}

	// メモリ予約失敗時は空のStreamStringを返す
	return StreamString();
}

String HTTPClient::errorToString(int error) {
	switch (error) {
	case HTTPC_ERROR_CONNECTION_REFUSED:
		return F("connection refused");
	case HTTPC_ERROR_SEND_HEADER_FAILED:
		return F("send header failed");
	case HTTPC_ERROR_SEND_PAYLOAD_FAILED:
		return F("send payload failed");
	case HTTPC_ERROR_NOT_CONNECTED:
		return F("not connected");
	case HTTPC_ERROR_CONNECTION_LOST:
		return F("connection lost");
	case HTTPC_ERROR_NO_STREAM:
		return F("no stream");
	case HTTPC_ERROR_NO_HTTP_SERVER:
		return F("no HTTP server");
	case HTTPC_ERROR_TOO_LESS_RAM:
		return F("too less ram");
	case HTTPC_ERROR_ENCODING:
		return F("Transfer-Encoding not supported");
	case HTTPC_ERROR_STREAM_WRITE:
		return F("Stream write error");
	case HTTPC_ERROR_READ_TIMEOUT:
		return F("read Timeout");
	default:
		return String(); // 値が一致しない場合、空のStringを返す
	}
}

void HTTPClient::addHeader(const String &name, const String &value, bool first, bool replace) {
	// 予約済みヘッダーのチェック
	if (!name.equalsIgnoreCase(F("Connection")) &&
	    !name.equalsIgnoreCase(F("User-Agent")) &&
	    !name.equalsIgnoreCase(F("Host")) &&
	    !(name.equalsIgnoreCase(F("Authorization")) && _base64Authorization.length() != 0)) {

		// ヘッダー行の基本部分を作成
		String headerLine = name + ": ";

		// replaceフラグがtrueの場合、既存ヘッダーを置換
		if (replace) {
			// ヘッダー行の位置を検索
			int pos = _headers.indexOf(headerLine);
			if (pos >= 0) {
				// ヘッダー行が見つかった場合、前に改行があるかを確認
				if (pos == 0 || _headers[pos - 1] == '\n') {
					// ヘッダーの終わりを検索
					int endPos = _headers.indexOf("\n", pos);
					if (endPos >= 0) {
						// 見つかった部分を削除し、残りの部分を結合
						_headers = _headers.substring(0, pos) + _headers.substring(endPos + 1);
					}
				}
			}
		}

		// ヘッダー行に値を追加
		headerLine += value + "\r\n";

		// firstフラグに応じて追加位置を決定
		if (first) {
			// 先頭に追加
			_headers = headerLine + _headers;
		} else {
			// 末尾に追加
			_headers += headerLine;
		}
	}
}

void HTTPClient::collectHeaders(const char *headerKeys[], const size_t headerKeysCount) {
	// _headerKeysCountを設定
	_headerKeysCount = headerKeysCount;

	// _currentHeadersが既に存在する場合、メモリを解放
	if (_currentHeaders != nullptr) {
		delete[] _currentHeaders;
	}

	// 新しいメモリブロックを確保し、_currentHeadersに代入
	_currentHeaders = new RequestArgument[_headerKeysCount];

	// ヘッダーキー配列から各ヘッダーのキーを_currentHeadersに設定
	for (size_t i = 0; i < _headerKeysCount; ++i) {
		_currentHeaders[i].key = headerKeys[i];
	}
}

String HTTPClient::header(const char *name) {
	// _headerKeysCount回繰り返し、各ヘッダーを確認
	for (size_t i = 0; i < _headerKeysCount; ++i) {
		// 現在のヘッダー名が引数のnameと一致しているか確認
		if (strcmp(_currentHeaders[i].key.c_str(), name) == 0) {
			// 一致する場合は、_currentHeaders[i].valueを返す
			return _currentHeaders[i].value;
		}
	}

	// 一致しなかった場合は空のStringを返す
	return String();
}

String HTTPClient::header(size_t i) {
	// iが_headerKeysCountより小さい場合
	if (i < _headerKeysCount) {
		// 指定されたインデックスのヘッダー値を返す
		return _currentHeaders[i].value;
	}

	// iが_headerKeysCount以上の場合は空のStringを返す
	return String();
}

String HTTPClient::headerName(size_t i) {
	// iが_headerKeysCountより小さい場合
	if (i < _headerKeysCount) {
		// 指定されたインデックスのヘッダー名を返す
		return _currentHeaders[i].key;
	}

	// iが_headerKeysCount以上の場合は空のStringを返す
	return String();
}

int HTTPClient::headers() {
	return _headerKeysCount;
}

bool HTTPClient::hasHeader(const char *name) {
	// _headerKeysCount回繰り返す
	for (size_t i = 0; i < _headerKeysCount; i++) {
		// ヘッダー名が一致し、ヘッダー値の長さが0より大きい場合
		if (strcmp(_currentHeaders[i].key.c_str(), name) == 0 && _currentHeaders[i].value.length() > 0) {
			return true;
		}
	}

	// 一致しない場合、またはヘッダー値が空の場合はfalse
	return false;
}

bool HTTPClient::connect() {
	// 既存接続が存在する場合、処理を行う
	if (connected()) {
		// 受信バッファ内の全データを読み出し破棄
		while (_client->available() > 0) {
			_client->read();
		}
		return true; // バッファクリア後、接続状態を維持
	}

	// _clientがnullの場合、接続できないのでfalseを返す
	if (!_client) {
		return false;
	}

	// 新規接続を試みる
	if (!_client->connect(_host.c_str(), _port, _connectTimeout)) {
		return false; // 接続に失敗した場合、falseを返す
	}

	// 接続成功後、タイムアウトの設定
	_client->setTimeout((_tcpTimeout + 500) / 1000);

	// 最終的に接続状態を確認してtrue/falseを返す
	return connected();
}

bool HTTPClient::sendHeader(const char *type) {
	// 接続確認
	if (!connected()) {
		return false;
	}

	// リクエストラインの構築
	String header = String(type) + " " + _uri + " HTTP/1.";
	header += (_useHTTP10 ? "0" : "1");
	header += "\r\n";

	// Hostヘッダー
	header += "Host: " + _host;
	if (_port != 80 && _port != 443) {
		header += ":" + String(_port);
	}
	header += "\r\n";

	// User-Agentヘッダー
	header += "User-Agent: " + _userAgent + "\r\n";

	// Connectionヘッダー
	header += "Connection: ";
	header += (_reuse ? "keep-alive" : "close");
	header += "\r\n";

	// Accept-Encodingヘッダー (HTTP/1.1の場合のみ)
	if (!_useHTTP10) {
		header += "Accept-Encoding: identity;q=1,chunked;q=0.1,*;q=0\r\n";
	}

	// Authorizationヘッダー (Basic認証)
	if (_base64Authorization.length() > 0) {
		header += "Authorization: Basic " + _base64Authorization + "\r\n";
	}

	// カスタムヘッダー
	header += _headers + "\r\n";

	// 最後に空行を追加
	header += "\r\n";

	// ヘッダー送信
	size_t len = header.length();
	if (_client->write((const uint8_t *)header.c_str(), len) != len) {
		return false; // 送信失敗
	}

	return true; // 送信成功
}

int HTTPClient::handleHeaderResponse(void) {
	if (!connected()) {
		return HTTPC_ERROR_NOT_CONNECTED;
	}

	clear();
	_canReuse                  = _reuse;
	_transferEncoding          = HTTPC_TE_IDENTITY;
	unsigned long lastDataTime = millis();
	bool firstLine             = true;
	String transferEncoding; // transferEncoding の宣言

	while (connected()) {
		if (_client->available() > 0) {
			String headerLine = _client->readStringUntil('\n');
			headerLine.trim();
			lastDataTime = millis();

			if (firstLine) {
				firstLine = false;
				if (_canReuse && headerLine.startsWith("HTTP/1.")) {
					_canReuse = (headerLine[sizeof("HTTP/1.") - 1] != '0');
				}
				_returnCode = headerLine.substring(9, 12).toInt();
			} else if (headerLine.indexOf(':') != -1) {
				int separatorIndex = headerLine.indexOf(':');
				String headerName  = headerLine.substring(0, separatorIndex);
				String headerValue = headerLine.substring(separatorIndex + 1);

				if (headerName.equalsIgnoreCase("Content-Length")) {
					_size = headerValue.toInt();
				}

				if (_canReuse && headerName.equalsIgnoreCase("Connection")) {
					if (headerValue.indexOf("close") != -1 && headerValue.indexOf("keep-alive") == -1) {
						_canReuse = false;
					}
				}

				if (headerName.equalsIgnoreCase("Transfer-Encoding")) {
					transferEncoding = headerValue;
				}

				if (headerName.equalsIgnoreCase("Location")) {
					_location = headerValue;
				}

				for (size_t i = 0; i < _headerKeysCount; i++) {
					if (headerName.equalsIgnoreCase(_currentHeaders[i].key)) {
						_currentHeaders[i].value = headerValue;
					}
				}
			} else if (headerLine.length() == 0) {
				if (transferEncoding.length() > 0) {
					if (transferEncoding.equalsIgnoreCase("chunked")) {
						_transferEncoding = HTTPC_TE_CHUNKED;
					} else {
						return HTTPC_ERROR_ENCODING;
					}
				} else {
					_transferEncoding = HTTPC_TE_IDENTITY;
				}

				if (_returnCode > 0) {
					return _returnCode;
				} else {
					return HTTPC_ERROR_NO_HTTP_SERVER;
				}
			}
		} else {
			if (millis() - lastDataTime > _tcpTimeout) {
				return HTTPC_ERROR_READ_TIMEOUT;
			}
			delay(10);
		}
	}

	return HTTPC_ERROR_READ_TIMEOUT;
}

int HTTPClient::writeToStreamDataBlock(Stream *stream, int size) {
	int buff_size    = HTTP_TCP_BUFFER_SIZE;
	int len          = size;
	int bytesWritten = 0;

	// len > 0 かつ len < HTTP_TCP_BUFFER_SIZE の場合、buff_size を len に設定
	if ((len > 0) && (len < HTTP_TCP_BUFFER_SIZE)) {
		buff_size = len;
	}

	// メモリ確保
	uint8_t *buff = (uint8_t *)malloc(buff_size);
	if (buff == NULL) {
		return HTTPC_ERROR_TOO_LESS_RAM; // メモリ確保に失敗した場合は早期 return
	}

	while (connected() && (len > 0 || len == -1)) {
		int sizeAvailable = _client->available();

		// sizeAvailable が 0 の場合は delay(1) を実行して、次のループへ進む
		if (sizeAvailable == 0) {
			delay(1);
			continue;
		}

		int readBytes = sizeAvailable;

		// len が 0 より大きければ、それに合わせて readBytes を制限
		if (len > 0 && readBytes > len) {
			readBytes = len;
		}
		// buff_size より大きくならないように制限
		if (readBytes > buff_size) {
			readBytes = buff_size;
		}

		// readBytes が 0 の場合、ループを抜ける
		if (readBytes == 0) {
			break;
		}

		// データを読み取る
		int bytesRead = _client->readBytes(buff, readBytes);

		// ストリームにデータを書き込む
		int bytesWrite = stream->write(buff, bytesRead);
		bytesWritten += bytesWrite;

		// 書き込みエラーが発生した場合
		if (bytesWrite != bytesRead) {
			if (stream->getWriteError()) {
				stream->clearWriteError();
			}

			delay(1); // 少し待機

			// 残りのバイトを再送信
			int leftBytes = bytesRead - bytesWrite;
			bytesWrite    = stream->write((buff + bytesWrite), leftBytes);
			bytesWritten += bytesWrite;

			// 書き込みが残っている場合のエラーチェック
			if (bytesWrite != leftBytes) {
				free(buff);
				return HTTPC_ERROR_STREAM_WRITE;
			}
		}

		// 書き込みエラーが発生した場合は早期リターン
		if (stream->getWriteError()) {
			free(buff);
			return HTTPC_ERROR_STREAM_WRITE;
		}

		// len が指定されていれば、残りのデータを減らす
		if (len > 0) {
			len -= bytesRead;
		}

		// 少し待機
		delay(0);
	}

	free(buff);

	// 最後に bytesWritten と size が一致しない場合エラー
	if (size > 0 && bytesWritten != size) {
		return HTTPC_ERROR_STREAM_WRITE;
	}

	return bytesWritten;
}

int HTTPClient::returnError(int error) {
	if (error < 0 && connected()) {
		_client->stop(); // 接続を停止
	}
	return error; // エラーコードを返す
}

void HTTPClient::setFollowRedirects(followRedirects_t follow) {
	_followRedirects = follow;
}

void HTTPClient::setRedirectLimit(uint16_t limit) {
	_redirectLimit = limit;
}

bool HTTPClient::setURL(const String &url) {
	// URLが空でないか、先頭が'/'であるかを確認
	if (url.length() > 0 && url.charAt(0) == '/') {
		_uri = url;  // _uri に url を設定
		clear();     // 内部状態をクリア
		return true; // 成功した場合は true を返す
	}

	// url が _protocol + ':' で始まらない場合は失敗
	if (!url.startsWith(_protocol + ":")) {
		return false;
	}

	// URLの6文字目以降から':'の位置を探す
	int indexPort = url.indexOf(':', 6); // 6文字目以降から':'を検索

	// URLの7文字目以降から'/'の位置を探す
	int indexURI = url.indexOf('/', 7); // 7文字目以降から'/'を検索

	// ':'の位置が-1ではないかつ、':'が'/'より後ろの場合
	if (indexPort != -1 && indexPort > indexURI) {
		if (_protocol == "https") {
			_port = 443; // httpsの場合はポート443
		} else {
			_port = 80; // https以外はポート80
		}
	}

	// _canReuse に true をセット
	_canReuse = true;

	// 接続を切断
	disconnect();

	// beginInternal関数を呼び出し、url と _protocol.c_str() を引数に渡す
	return beginInternal(url, _protocol.c_str());
}

const String &HTTPClient::getLocation(void) {
	return _location;
}
