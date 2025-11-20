#include "AtlasScientific_EZO_DO.h"

bool EZO_DO::begin() {
	char module_type[MAX_BUFFER_LENGTH] = {0};
	char fw_version[MAX_BUFFER_LENGTH]  = {0};

	reset();

	getDeviceInfo(module_type, fw_version);
	if (strcmp(module_type, "DO") != 0) {
		return false;
	}
	// Serial.printf("%s -- %s\n", module_type, fw_version);

	enableSensing(true, true);

	return true;
}

void EZO_DO::reset() {
	_do_sensor.send_cmd("Factory");
	delay(1700);
}

bool EZO_DO::read() {
	char *context                  = NULL;
	char buffer[MAX_BUFFER_LENGTH] = {0};

	_do_sensor.send_cmd("R");
	delay(600);
	Ezo_board::errors err = _do_sensor.receive_cmd(buffer, MAX_BUFFER_LENGTH);

	if (err != Ezo_board::SUCCESS) {
		return false;
	}

	char *_str_do  = strtok_r(buffer, ",", &context); // DO
	char *_str_sat = strtok_r(NULL, ",", &context);   // Saturation
	
	if (_str_do == NULL && _str_sat == NULL) {
		return false;
	}

	if (_str_do != NULL) {
		_do = strtod(_str_do, NULL);
		// Ignore error handling
	}

	if (_str_sat != NULL) {
		_saturation = strtod(_str_sat, NULL);
		// Ignore error handling
	}

	return true;
}

double EZO_DO::getDissolvedOxygen() {
	return _do;
}

double EZO_DO::getSaturation() {
	return _saturation;
}

void EZO_DO::getDeviceInfo(char *module_type, char *fw_version) {
	char *context                  = NULL;
	char buffer[MAX_BUFFER_LENGTH] = {0};

	_do_sensor.send_cmd("i");
	delay(300);
	Ezo_board::errors err = _do_sensor.receive_cmd(buffer, MAX_BUFFER_LENGTH);

	if (err != Ezo_board::SUCCESS) {
		showError(err);
		return;
	}
	strtok_r(buffer, ",", &context);                    //?I
	char *_module_type = strtok_r(NULL, ",", &context); // DO
	char *_fw_version  = strtok_r(NULL, ",", &context); // X.XX

	strncpy(module_type, _module_type, MAX_BUFFER_LENGTH);
	strncpy(fw_version, _fw_version, MAX_BUFFER_LENGTH);
}

void EZO_DO::enableSensing(bool mgL, bool saturation) {
	if (mgL) {
		_do_sensor.send_cmd("O,mg,1");
	} else {
		_do_sensor.send_cmd("O,mg,0");
	}

	delay(300);

	if (saturation) {
		_do_sensor.send_cmd("O,%,1");
	} else {
		_do_sensor.send_cmd("O,%,0");
	}

	delay(300);

	// delay(300);
	// char buffer[MAX_BUFFER_LENGTH] = {0};
	// _do_sensor.send_cmd("O,?");
	// delay(300);
	// _do_sensor.receive_cmd(buffer, MAX_BUFFER_LENGTH);
	// Serial.printf("%s\n", buffer);
}

void EZO_DO::showError(Ezo_board::errors err) {
	switch (err) {
	case Ezo_board::SUCCESS:
		Serial.println("SUCCESS");
		break;
	case Ezo_board::FAIL:
		Serial.print("Failed "); // means the command has failed.
		break;
	case Ezo_board::NOT_READY:
		Serial.print("Pending "); // the command has not yet been finished calculating.
		break;
	case Ezo_board::NO_DATA:
		Serial.print("No Data "); // the sensor has no data to send.
		break;
	}
}
