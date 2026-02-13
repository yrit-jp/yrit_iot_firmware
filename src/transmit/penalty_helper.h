#pragma once
#include <Arduino.h>

/**
 * @brief 送信失敗時のペナルティ待機を管理するクラス
 *
 * 連続した送信失敗を防ぐため、失敗後に一定時間の待機を強制する
 */
class PenaltyHelper {
public:
	PenaltyHelper();

	/**
	 * @brief ペナルティ期間中かどうかを確認（期限切れなら自動解除）
	 * @return true: 待機中（送信スキップすべき）, false: 送信可能
	 */
	bool isActive();

	/**
	 * @brief ペナルティを有効化
	 * @param wait_ms 待機時間（ミリ秒）
	 */
	void activate(uint16_t wait_ms);

	/// ペナルティを手動で解除
	void reset();

private:
	bool _enable;
	unsigned long _last_failed_millis;
	uint16_t _wait_ms;
};
