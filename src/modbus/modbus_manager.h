#ifndef MODBUS_MANAGER_H
#define MODBUS_MANAGER_H

#include "../hal/hal.h"
#include "../user/user_modbus_sensor.h"
#include "modbus_type_def.h"

class Modbus_Manager {
public:
	Modbus_Manager();
	void addPort(uint8_t port);
	void clearPorts();
	void clearDevices();

	MODBUS_INIT_RESULT initializeDevice(uint8_t port, uint8_t id);
	String getDeviceName(uint8_t port,
	                     uint8_t id,
	                     int16_t sub_id);
	String getDeviceUnit(uint8_t port,
	                     uint8_t id,
	                     int16_t sub_id);
	MODBUS_DATA_RESULT fetchData(uint8_t port,
	                             uint8_t id,
	                             int16_t sub_id,
	                             bool tx_buffered,
	                             double *d_data,
	                             int32_t *i_data,
	                             String *s_data);
	std::vector<ModbusActiveDevice> getIdList();

	void scanDevice(uint8_t port,
	                uint8_t min_id = 1,
	                uint8_t max_id = 10);
	void displayScanResult();

protected:
	std::set<uint8_t> _ports;
	std::vector<ModbusActiveDevice> _active_device;
};

#endif /* MODBUS_MANAGER_H */