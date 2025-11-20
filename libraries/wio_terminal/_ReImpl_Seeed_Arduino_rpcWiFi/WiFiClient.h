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

#ifndef WIFICLIENT_H
#define WIFICLIENT_H

#include <Arduino.h>
#include <Client.h>
#include <memory>

#undef max
#undef min

// 前方宣言
class WiFiClientSocketHandle;
class WiFiClientRxBuffer;

class WiFiClient : public Client {
protected:
    std::shared_ptr<WiFiClientSocketHandle> clientSocketHandle;
    std::shared_ptr<WiFiClientRxBuffer> _rxBuffer;
    bool _connected;

public:
    using Print::write;
    WiFiClient *next;
    uint32_t conn_status;

    WiFiClient();
    WiFiClient(int fd);
    ~WiFiClient();

    WiFiClient& operator=(const WiFiClient &other);

    void stop();
    int connect(IPAddress ip, uint16_t port, int32_t timeout);
    int connect(IPAddress ip, uint16_t port);
    int connect(const char *host, uint16_t port);
    int connect(const char *host, uint16_t port, int32_t timeout);
    int setSocketOption(int option, char* value, size_t len);
    int setTimeout(uint32_t seconds);
    int setOption(int option, int *value);
    int getOption(int option, int *value);
    int setNoDelay(bool nodelay);
    bool getNoDelay();
    size_t write(uint8_t data);
    int read();
    size_t write(const uint8_t *buf, size_t size);
    size_t write_P(PGM_P buf, size_t size);
    size_t write(Stream &stream);
    int read(uint8_t *buf, size_t size);
    int peek();
    int available();
    void flush();
    void clear();
    uint8_t connected();
    IPAddress remoteIP(int fd) const;
    uint16_t remotePort(int fd) const;
    IPAddress remoteIP() const;
    uint16_t remotePort() const;
    IPAddress localIP(int fd) const;
    uint16_t localPort(int fd) const;
    IPAddress localIP() const;
    uint16_t localPort() const;
    bool operator==(const WiFiClient& rhs);
    int fd() const;

    // 追加された演算子
    operator bool();
    bool operator==(const bool value);
    bool operator!=(const bool value);
    bool operator!=(const WiFiClient& rhs);
};

#endif
