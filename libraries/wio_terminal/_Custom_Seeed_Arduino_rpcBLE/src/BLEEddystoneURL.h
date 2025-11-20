/*
 * BLEEddystoneURL.cpp
 *
 *  Created on: Mar 12, 2018
 *      Author: pcbreflux
 */

/*
 * SPDX-License-Identifier: Apache-2.0
 * 
 * References:
 * https://github.com/Seeed-Studio/Seeed_Arduino_rpcBLE
 * https://github.com/espressif/arduino-esp32/tree/master/libraries/BLE
 * https://github.com/espressif/arduino-esp32/pull/5261
 */

#ifndef _BLEEddystoneURL_H_
#define _BLEEddystoneURL_H_
#include "BLEUUID.h"

#define EDDYSTONE_URL_FRAME_TYPE 0x10

/**
 * @brief Representation of a beacon.
 * See:
 * * https://github.com/google/eddystone
 */
class BLEEddystoneURL {
public:
	BLEEddystoneURL();
	std::string getData();
	BLEUUID	 getUUID();
	int8_t	  getPower();
	std::string getURL();
	std::string getDecodedURL();
	void		setData(std::string data);
	void		setUUID(BLEUUID l_uuid);
	void		setPower(int8_t advertisedTxPower);
	void		setURL(std::string url);

private:
	uint16_t beaconUUID;
	uint8_t  lengthURL;
	struct {
		uint8_t frameType;
		int8_t  advertisedTxPower;
		uint8_t url[16];
	} __attribute__((packed)) m_eddystoneData;

}; // BLEEddystoneURL

#endif /* _BLEEddystoneURL_H_ */
