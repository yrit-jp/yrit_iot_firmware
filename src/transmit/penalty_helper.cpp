#include "penalty_helper.h"

PenaltyHelper::PenaltyHelper() : _enable(false), _last_failed_millis(0), _wait_ms(0) {}

bool PenaltyHelper::isActive() {
	if (_enable and (millis() - _last_failed_millis) > _wait_ms) {
		_enable = false;
	}
	return _enable;
}

void PenaltyHelper::activate(uint16_t wait_ms) {
	_enable             = true;
	_last_failed_millis = millis();
	_wait_ms            = wait_ms;
}

void PenaltyHelper::reset() {
	_enable = false;
}
