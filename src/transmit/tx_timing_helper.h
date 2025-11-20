#ifndef TRANSMIT_TIMING_HELPER_H
#define TRANSMIT_TIMING_HELPER_H

#include "../hal/hal.h"

namespace TxTimingHelper {
	extern struct tm base_time;
	extern uint32_t next_step_sec;

	/*!
	 * @brief 送信タイミングの基準時間を設定する関数
	 * @param[in] (initial_time) 基準時間
	 * @param[in] (tx_interval_sec) 送信間隔
	 * @param[in] (need_first_interval) 次回の確認まで送信間隔を確保するか
	 */
	void begin(struct tm initial_time,
	           uint32_t tx_interval_sec,
	           bool need_first_interval = false);

	/*!
	 * @brief 送信タイミングを判定する関数
	 * @param[in] (currentTime) 判定する時間
	 * @return 送信すべきタイミングの場合は true, それ以外は false を返す
	 * @note true の場合は即座に次のタイミングを指すように処理される
	 */
	bool shouldTransmit(struct tm currentTime);
}

#endif /* TRANSMIT_TIMING_HELPER_H */