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

#ifndef HTTPCLIENT_H
#define HTTPCLIENT_H

#include "HTTPClient.h" // 必要に応じて追加
#include <Arduino.h>
#include <StreamString.h>
#include "WiFiClient.h"
#include <memory>
#include <rpcWiFi.h>

// 定数
#define HTTPCLIENT_DEFAULT_TCP_TIMEOUT (5000)

#define HTTPC_ERROR_CONNECTION_REFUSED (-1)
#define HTTPC_ERROR_SEND_HEADER_FAILED (-2)
#define HTTPC_ERROR_SEND_PAYLOAD_FAILED (-3)
#define HTTPC_ERROR_NOT_CONNECTED (-4)
#define HTTPC_ERROR_CONNECTION_LOST (-5)
#define HTTPC_ERROR_NO_STREAM (-6)
#define HTTPC_ERROR_NO_HTTP_SERVER (-7)
#define HTTPC_ERROR_TOO_LESS_RAM (-8)
#define HTTPC_ERROR_ENCODING (-9)
#define HTTPC_ERROR_STREAM_WRITE (-10)
#define HTTPC_ERROR_READ_TIMEOUT (-11)

#define HTTP_TCP_BUFFER_SIZE (1460)

// HTTPコードの列挙型
typedef enum {
	HTTP_CODE_CONTINUE                        = 100,
	HTTP_CODE_SWITCHING_PROTOCOLS             = 101,
	HTTP_CODE_PROCESSING                      = 102,
	HTTP_CODE_OK                              = 200,
	HTTP_CODE_CREATED                         = 201,
	HTTP_CODE_ACCEPTED                        = 202,
	HTTP_CODE_NON_AUTHORITATIVE_INFORMATION   = 203,
	HTTP_CODE_NO_CONTENT                      = 204,
	HTTP_CODE_RESET_CONTENT                   = 205,
	HTTP_CODE_PARTIAL_CONTENT                 = 206,
	HTTP_CODE_MULTI_STATUS                    = 207,
	HTTP_CODE_ALREADY_REPORTED                = 208,
	HTTP_CODE_IM_USED                         = 226,
	HTTP_CODE_MULTIPLE_CHOICES                = 300,
	HTTP_CODE_MOVED_PERMANENTLY               = 301,
	HTTP_CODE_FOUND                           = 302,
	HTTP_CODE_SEE_OTHER                       = 303,
	HTTP_CODE_NOT_MODIFIED                    = 304,
	HTTP_CODE_USE_PROXY                       = 305,
	HTTP_CODE_TEMPORARY_REDIRECT              = 307,
	HTTP_CODE_PERMANENT_REDIRECT              = 308,
	HTTP_CODE_BAD_REQUEST                     = 400,
	HTTP_CODE_UNAUTHORIZED                    = 401,
	HTTP_CODE_PAYMENT_REQUIRED                = 402,
	HTTP_CODE_FORBIDDEN                       = 403,
	HTTP_CODE_NOT_FOUND                       = 404,
	HTTP_CODE_METHOD_NOT_ALLOWED              = 405,
	HTTP_CODE_NOT_ACCEPTABLE                  = 406,
	HTTP_CODE_PROXY_AUTHENTICATION_REQUIRED   = 407,
	HTTP_CODE_REQUEST_TIMEOUT                 = 408,
	HTTP_CODE_CONFLICT                        = 409,
	HTTP_CODE_GONE                            = 410,
	HTTP_CODE_LENGTH_REQUIRED                 = 411,
	HTTP_CODE_PRECONDITION_FAILED             = 412,
	HTTP_CODE_PAYLOAD_TOO_LARGE               = 413,
	HTTP_CODE_URI_TOO_LONG                    = 414,
	HTTP_CODE_UNSUPPORTED_MEDIA_TYPE          = 415,
	HTTP_CODE_RANGE_NOT_SATISFIABLE           = 416,
	HTTP_CODE_EXPECTATION_FAILED              = 417,
	HTTP_CODE_MISDIRECTED_REQUEST             = 421,
	HTTP_CODE_UNPROCESSABLE_ENTITY            = 422,
	HTTP_CODE_LOCKED                          = 423,
	HTTP_CODE_FAILED_DEPENDENCY               = 424,
	HTTP_CODE_UPGRADE_REQUIRED                = 426,
	HTTP_CODE_PRECONDITION_REQUIRED           = 428,
	HTTP_CODE_TOO_MANY_REQUESTS               = 429,
	HTTP_CODE_REQUEST_HEADER_FIELDS_TOO_LARGE = 431,
	HTTP_CODE_INTERNAL_SERVER_ERROR           = 500,
	HTTP_CODE_NOT_IMPLEMENTED                 = 501,
	HTTP_CODE_BAD_GATEWAY                     = 502,
	HTTP_CODE_SERVICE_UNAVAILABLE             = 503,
	HTTP_CODE_GATEWAY_TIMEOUT                 = 504,
	HTTP_CODE_HTTP_VERSION_NOT_SUPPORTED      = 505,
	HTTP_CODE_VARIANT_ALSO_NEGOTIATES         = 506,
	HTTP_CODE_INSUFFICIENT_STORAGE            = 507,
	HTTP_CODE_LOOP_DETECTED                   = 508,
	HTTP_CODE_NOT_EXTENDED                    = 510,
	HTTP_CODE_NETWORK_AUTHENTICATION_REQUIRED = 511
} t_http_codes;

// Transfer Encodingの列挙型
typedef enum {
	HTTPC_TE_IDENTITY,
	HTTPC_TE_CHUNKED
} transferEncoding_t;

// Follow Redirectsの列挙型
typedef enum {
	HTTPC_DISABLE_FOLLOW_REDIRECTS,
	HTTPC_STRICT_FOLLOW_REDIRECTS,
	HTTPC_FORCE_FOLLOW_REDIRECTS
} followRedirects_t;

// HTTPClientクラス
class HTTPClient {
public:
	~HTTPClient();

	bool begin(WiFiClient &client, String url);
	bool begin(WiFiClient &client, String host, uint16_t port, String uri = "/", bool https = false);

	void end(void);

	bool connected(void);

	void setReuse(bool reuse); // keep-alive
	void setUserAgent(const String &userAgent);
	void setAuthorization(const char *user, const char *password);
	void setAuthorization(const char *auth);
	void setConnectTimeout(int32_t connectTimeout);
	void setTimeout(uint16_t timeout);

	void setFollowRedirects(followRedirects_t follow);
	void setRedirectLimit(uint16_t limit); // max redirects to follow for a single request

	bool setURL(const String &url);
	void useHTTP10(bool usehttp10 = true);

	int GET();
	int PATCH(uint8_t *payload, size_t size);
	int PATCH(String payload);
	int POST(uint8_t *payload, size_t size);
	int POST(String payload);
	int PUT(uint8_t *payload, size_t size);
	int PUT(String payload);
	int sendRequest(const char *type, String payload);
	int sendRequest(const char *type, uint8_t *payload = NULL, size_t size = 0);
	int sendRequest(const char *type, Stream *stream, size_t size = 0);

	void addHeader(const String &name, const String &value, bool first = false, bool replace = true);

	void collectHeaders(const char *headerKeys[], const size_t headerKeysCount);
	String header(const char *name);  // get request header value by name
	String header(size_t i);          // get request header value by number
	String headerName(size_t i);      // get request header name by number
	int headers();                    // get header count
	bool hasHeader(const char *name); // check if header exists

	int getSize(void);
	const String &getLocation(void);

	WiFiClient &getStream(void);
	WiFiClient *getStreamPtr(void);
	int writeToStream(Stream *stream);
	StreamString getString(void);

	static String errorToString(int error);

protected:
	// プライベートメソッド
	bool beginInternal(String url, const char *expectedProtocol);
	void disconnect(bool preserveClient = false);
	void clear();
	int returnError(int error);
	bool connect(void);
	bool sendHeader(const char *type);
	int handleHeaderResponse();
	int writeToStreamDataBlock(Stream *stream, int len);

protected:
	// プライベート変数
	struct RequestArgument {
		String key;
		String value;
	};

	WiFiClient *_client = nullptr;

	// リクエスト関連
	String _host;
	uint16_t _port          = 0;
	int32_t _connectTimeout = 2000; // Default timeout: 2000ms
	bool _reuse             = true;
	uint16_t _tcpTimeout    = HTTPCLIENT_DEFAULT_TCP_TIMEOUT;
	bool _useHTTP10         = false;
	bool _secure            = false;

	String _uri;
	String _protocol;
	String _headers;
	String _userAgent = "WioTerminalHTTPClient";
	String _base64Authorization;

	// レスポンス関連
	RequestArgument *_currentHeaders = nullptr;
	size_t _headerKeysCount          = 0;

	int _returnCode                    = 0;
	int _size                          = -1;
	bool _canReuse                     = false;
	followRedirects_t _followRedirects = HTTPC_DISABLE_FOLLOW_REDIRECTS;
	uint16_t _redirectLimit            = 10;
	String _location;
	transferEncoding_t _transferEncoding = HTTPC_TE_IDENTITY;
};

#endif // HTTPCLIENT_H
