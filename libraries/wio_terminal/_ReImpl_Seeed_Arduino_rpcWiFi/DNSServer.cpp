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

#include "DNSServer.h"
#include <Arduino.h>
#include <lwip/def.h>

DNSServer::DNSServer() {
	// 1. _ttlにDNS_DEFAULT_TTL（デフォルトのTTL値）をhtonl()関数を利用してネットワークバイトオーダーに変換して代入
	_ttl = htonl(DNS_DEFAULT_TTL);

	// 2. _errorReplyCodeにDNSReplyCode::NonExistentDomain（非存在ドメインエラーコード）を代入
	_errorReplyCode = DNSReplyCode::NonExistentDomain;

	// 3. _dnsHeaderのメモリ確保
	_dnsHeader = static_cast<DNSHeader *>(malloc(sizeof(DNSHeader)));

	// 4. _dnsQuestionのメモリ確保
	_dnsQuestion = static_cast<DNSQuestion *>(malloc(sizeof(DNSQuestion)));

	// 5. _bufferにNULLを代入
	_buffer = NULL;

	// 6. _currentPacketSizeに0を代入
	_currentPacketSize = 0;

	// 7. _portに0を代入
	_port = 0;
}

bool DNSServer::start(const uint16_t &port, const String &domainName, const IPAddress &resolvedIP) {
	// 1. _portに与えられたポート番号を設定
	_port = port;

	// 2. _bufferをNULLとして初期化
	_buffer = NULL;

	// 3. _domainNameに与えられたドメイン名を設定
	_domainName = domainName;

	// 4. _resolvedIPに与えられたIPv4アドレスの各バイトをコピー
	for (int i = 0; i < 4; i++) {
		_resolvedIP[i] = resolvedIP[i];
	}

	// 5. _domainNameから"www."プレフィックスを削除し、小文字に変換
	downcaseAndRemoveWwwPrefix(_domainName);

	// 6. UDPサーバーを開始し、1を返した場合にtrue、その他はfalseを返す
	return (_udp.begin(_port) == 1);
}

void DNSServer::setErrorReplyCode(const DNSReplyCode &replyCode) {
	_errorReplyCode = replyCode;
}

void DNSServer::setTTL(const uint32_t &ttl) {
	_ttl = htonl(ttl);
}

void DNSServer::stop() {
	// 1. ネットワーク通信用のUDPサーバーを停止
	_udp.stop();

	// 2. _bufferで確保されたメモリを解放
	free(_buffer);

	// 3. _bufferをNULLに設定してメモリリークを防ぐ
	_buffer = NULL;
}

void DNSServer::downcaseAndRemoveWwwPrefix(String &domainName) {
	// 1. すべての文字を小文字に変換
	domainName.toLowerCase();

	// 2. "www." プレフィックスを削除
	domainName.replace("www.", "");
}

void DNSServer::processNextRequest() {
	// 1. 受信したデータパケットのサイズを取得
	_currentPacketSize = _udp.parsePacket();
	if (_currentPacketSize == 0) {
		return;
	}

	// 2. バッファの確保
	if (_buffer != NULL) {
		free(_buffer);
	}
	_buffer = (unsigned char *)malloc(_currentPacketSize * sizeof(char));
	if (_buffer == NULL) {
		return;
	}

	// 3. データの取得
	_udp.read(_buffer, _currentPacketSize);

	// 4. DNSヘッダ情報のコピー
	memcpy(_dnsHeader, _buffer, DNS_HEADER_SIZE);

	// 5. クエリが1つのみの場合に処理を行う
	if (requestIncludesOnlyOneQuestion()) {
		_dnsQuestion->QNameLength = 0;

		// 6. ラベルの処理
		while (_buffer[DNS_HEADER_SIZE + _dnsQuestion->QNameLength] != 0) {
			// ラベルのデータをQNameにコピー
			memcpy((void *)&_dnsQuestion->QName[_dnsQuestion->QNameLength],
			       (void *)&_buffer[DNS_HEADER_SIZE + _dnsQuestion->QNameLength],
			       _buffer[DNS_HEADER_SIZE + _dnsQuestion->QNameLength] + 1);

			// QNameの長さを更新
			_dnsQuestion->QNameLength += _buffer[DNS_HEADER_SIZE + _dnsQuestion->QNameLength] + 1;
		}

		// 7. QNameの終端を設定
		_dnsQuestion->QName[_dnsQuestion->QNameLength] = 0;

		// 8. QNameLengthを更新
		_dnsQuestion->QNameLength++;

		// 9. QTypeとQClassの取得
		memcpy(&_dnsQuestion->QType,
		       (void *)&_buffer[DNS_HEADER_SIZE + _dnsQuestion->QNameLength],
		       sizeof(_dnsQuestion->QType));
		memcpy(&_dnsQuestion->QClass,
		       (void *)&_buffer[DNS_HEADER_SIZE + _dnsQuestion->QNameLength + sizeof(_dnsQuestion->QType)],
		       sizeof(_dnsQuestion->QClass));
	}

	// 10. レスポンスの生成
	if (_dnsHeader->QR == DNS_QR_QUERY &&
	    _dnsHeader->OPCode == DNS_OPCODE_QUERY &&
	    requestIncludesOnlyOneQuestion() &&
	    (_domainName == "*" || getDomainNameWithoutWwwPrefix() == _domainName)) {
		replyWithIP();
	} else if (_dnsHeader->QR == DNS_QR_QUERY) {
		replyWithCustomCode();
	}

	// 11. バッファの解放
	free(_buffer);
	_buffer = NULL;
}

bool DNSServer::requestIncludesOnlyOneQuestion() {
	return ntohs(_dnsHeader->QDCount) == 1 &&
	_dnsHeader->ANCount == 0 &&
	_dnsHeader->NSCount == 0 &&
	_dnsHeader->ARCount == 0;
}

String DNSServer::getDomainNameWithoutWwwPrefix() {
	String parsedDomainName = "";

	if (_buffer == NULL) {
		return parsedDomainName;
	}

	// ドメイン名の開始位置
	unsigned char *start = _buffer + DNS_OFFSET_DOMAIN_NAME;

	if (*start == 0) {
		return parsedDomainName;
	}

	unsigned int pos = 0;
	while (true) {
		// ラベル長の取得
		unsigned char labelLength = *(start + pos);

		// ラベル内容の読み取り（順番逆にした処理）
		for (int i = 0; i < labelLength; i++) {
			pos++;                                    // posをインクリメント
			parsedDomainName += (char)*(start + pos); // posを使って次の文字を追加
		}

		// posをインクリメントして次のラベル位置に移動
		pos++;

		// 次のラベルの処理
		if (*(start + pos) == 0) {
			break;
		} else {
			parsedDomainName += ".";
		}
	}

	// 文字列処理
	downcaseAndRemoveWwwPrefix(parsedDomainName);

	return parsedDomainName;
}

void DNSServer::replyWithIP() {
	// 初期検証: バッファがNULLの場合は処理を終了
	if (_buffer == NULL) {
		return; // バッファがNULLなら何もしないで戻る
	}

	// UDP応答パケット準備
	_udp.beginPacket(_udp.remoteIP(), _udp.remotePort()); // リモートIPアドレスとポートへの送信開始

	// DNSヘッダー修正
	_dnsHeader->QR      = DNS_QR_RESPONSE;                    // QRフラグを応答に設定
	_dnsHeader->ANCount = _dnsHeader->QDCount;                // 回答数を問い合わせ数と同じに設定
	_udp.write((unsigned char *)_dnsHeader, DNS_HEADER_SIZE); // 修正したDNSヘッダーをUDPパケットに書き込む

	// DNS問い合わせ情報の書き込み
	_udp.write(_dnsQuestion->QName, _dnsQuestion->QNameLength); // ドメイン名をパケットに書き込む
	_udp.write((unsigned char *)&_dnsQuestion->QType, 2);       // 質問タイプ（QType）をパケットに書き込む
	_udp.write((unsigned char *)&_dnsQuestion->QClass, 2);      // 質問クラス（QClass）をパケットに書き込む

	// DNS応答レコード書き込み
	// 名前圧縮を使用
	_udp.write((uint8_t)0xC0);                   // 圧縮フラグ（最上位ビットを1に設定）
	_udp.write((uint8_t)DNS_OFFSET_DOMAIN_NAME); // ドメイン名オフセットを設定

	// レコードタイプ（A）の書き込み
	unsigned short dnsTypeA = htons(DNS_TYPE_A); // DNS_TYPE_A をネットワークバイトオーダーに変換
	_udp.write((unsigned char *)&dnsTypeA, 2);   // 2バイト送信

	// クラス（IN）の書き込み
	unsigned short dnsClassIN = htons(DNS_CLASS_IN); // DNS_CLASS_IN をネットワークバイトオーダーに変換
	_udp.write((unsigned char *)&dnsClassIN, 2);     // 2バイト送信

	// TTL（生存時間）の書き込み
	unsigned long ttl = _ttl;             // TTLをそのまま送信
	_udp.write((unsigned char *)&ttl, 4); // TTLは4バイトで送信

	// RDLENGTH（レコードデータ長）の書き込み
	unsigned short rdlength = htons(DNS_RDLENGTH_IPV4); // DNS_RDLENGTH_IPV4 をネットワークバイトオーダーに変換
	_udp.write((unsigned char *)&rdlength, 2);          // 2バイト送信

	// 解決したIPアドレスの書き込み
	_udp.write(_resolvedIP, sizeof(_resolvedIP)); // 解決したIPv4アドレスの書き込み

	// UDP応答パケットの送信を終了
	_udp.endPacket(); // パケット送信完了
}

void DNSServer::replyWithCustomCode() {
	// 初期検証: バッファがNULLの場合は処理を終了
	if (_buffer == NULL) {
		return; // バッファがNULLなら何もしないで戻る
	}

	// DNSヘッダー修正
	_dnsHeader->QR      = DNS_QR_RESPONSE;                             // QRフラグを応答に設定
	_dnsHeader->RCode   = static_cast<unsigned char>(_errorReplyCode); // エラーコードをunsigned charにキャストして設定
	_dnsHeader->QDCount = 0;                                           // 問い合わせ数を0に設定（クエスチョンセクションは送信しない）

	// UDP応答パケット準備
	_udp.beginPacket(_udp.remoteIP(), _udp.remotePort()); // リモートIPアドレスとポートへの送信開始

	// バッファ内容をUDPパケットに書き込み
	_udp.write(_buffer, sizeof(DNSHeader)); // _bufferの内容を送信

	// UDPパケットの送信を終了
	_udp.endPacket(); // パケット送信完了
}
