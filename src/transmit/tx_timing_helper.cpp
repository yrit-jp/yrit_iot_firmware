#include "tx_timing_helper.h"

namespace TxTimingHelper {
	struct tm base_time;
	uint32_t next_step_sec;

	void begin(struct tm initial_time,
	           uint32_t tx_interval_sec,
	           bool need_first_interval) {

		base_time     = initial_time;
		next_step_sec = tx_interval_sec;
		if (need_first_interval) {
			base_time.tm_sec += next_step_sec;
		}
	}

	bool shouldTransmit(struct tm currentTime) {
		int remain_sec = static_cast<int>(difftime(mktime(&currentTime), mktime(&base_time)));
		if (remain_sec < 0) {
			return false;
		}
		base_time.tm_sec += next_step_sec;
		return true;
	}
}
