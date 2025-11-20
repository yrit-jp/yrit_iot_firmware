#ifndef ATLAS_SCIENTIFIC_EZO_DO_H
#define ATLAS_SCIENTIFIC_EZO_DO_H

#include <Ezo_i2c.h>

class EZO_DO {
public:
	// EZO Dissolved Oxygen Circuit
	EZO_DO(uint8_t address, TwoWire *_wire = &Wire) : _do_sensor(address, _wire){};
	bool begin();
	void reset();
	bool read();
	double getDissolvedOxygen();
	double getSaturation();

private:
	TwoWire *_wire;
	Ezo_board _do_sensor;
	const uint8_t MAX_BUFFER_LENGTH = 17;
	unsigned long long _timestamp   = 0;
	double _do                      = 0.0;
	double _saturation              = 0.0;

	void getDeviceInfo(char *module_type, char *fw_version);
	void enableSensing(bool mgL, bool saturation);
	void showError(Ezo_board::errors err);
};

#endif /* ATLAS_SCIENTIFIC_EZO_DO_H */