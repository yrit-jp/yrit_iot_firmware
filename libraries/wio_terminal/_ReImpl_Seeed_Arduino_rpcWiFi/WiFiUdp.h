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

#ifndef WiFiUDP_h
#define WiFiUDP_h

#include <Arduino.h>
#include <Udp.h>
#include "cbuf.h"

class WiFiUDP : public UDP {
public:
    // コンストラクタとデストラクタ
    WiFiUDP();
    ~WiFiUDP();

    // メソッド
    uint8_t begin(IPAddress address, uint16_t port);
    uint8_t begin(uint16_t port);
    uint8_t beginMulticast(IPAddress a, uint16_t p);
    void stop();
    int beginMulticastPacket();
    int beginPacket();
    int beginPacket(IPAddress ip, uint16_t port);
    int beginPacket(const char * host, uint16_t port);
    int endPacket();
    size_t write(uint8_t data);
    size_t write(const uint8_t *buffer, size_t size);
    int parsePacket();
    int available();
    int read();
    int read(unsigned char *buffer, size_t len);
    int read(char *buffer, size_t len);
    int peek();
    void flush();
    void clear();
    IPAddress remoteIP();
    uint16_t remotePort();

private:
    // プライベート変数
    int udp_server;
    IPAddress multicast_ip;
    IPAddress remote_ip;
    uint16_t server_port;
    uint16_t remote_port;
    char *tx_buffer;
    size_t tx_buffer_len;
    cbuf *rx_buffer;
};

#endif // WiFiUDP_h
