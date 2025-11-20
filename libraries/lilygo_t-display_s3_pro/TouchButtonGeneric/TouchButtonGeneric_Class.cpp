// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "TouchButtonGeneric_Class.hpp"

void TouchButtonGeneric_Class::update(std::uint32_t msec, const int16_t *x, const int16_t *y, std::size_t count) {
	if (msec - _last_msec <= TOUCH_MIN_UPDATE_MSEC) /// Avoid high frequency updates
	{
		if (_detail_count == 0) {
			return;
		}
		std::size_t count = 0;
		for (std::size_t i = 0; i < TOUCH_MAX_POINTS; ++i) {
			count += update_detail(&_touch_detail[i], msec);
		}
		_detail_count = count;
		return;
	}

	_last_msec = msec;
	if (!(count || _detail_count)) {
		return;
	}

	uint32_t updated_id = 0;
	if (count) {
		for (std::size_t i = 0; i < count; ++i) {
			if (i < TOUCH_MAX_POINTS) {
				touch_point_t tp;
				tp.x  = x[i];
				tp.y  = y[i];
				tp.id = i;

				updated_id |= 1 << tp.id;
				update_detail(&_touch_detail[tp.id], msec, true, &tp);
			}
		}
	}
	{
		for (std::size_t i = 0; i < TOUCH_MAX_POINTS; ++i) {
			if ((!(updated_id & (1 << i))) && update_detail(&_touch_detail[i], msec, false, nullptr) && (count < TOUCH_MAX_POINTS)) {
				++count;
			}
		}
	}
	_detail_count = count;
}

bool TouchButtonGeneric_Class::update_detail(touch_detail_t *det, std::uint32_t msec, bool pressed, touch_point_t *tp) {
	touch_state_t::touch_state_t tm = det->state;
	if (tm == touch_state_t::none && !pressed) {
		return false;
	}
	tm = static_cast<touch_state_t::touch_state_t>(tm & ~touch_state_t::mask_change);
	if (pressed) {
		det->prev_x = det->x;
		det->prev_y = det->y;
		det->size   = tp->size;
		det->id     = tp->id;
		if (!(tm & touch_state_t::mask_moving)) { // Processing when not flicked.
			if (tm & touch_state_t::mask_touch) { // Not immediately after the touch.
				if (abs(det->base_x - tp->x) > _flickThresh || abs(det->base_y - tp->y) > _flickThresh) {
					det->prev = det->base;
					tm        = static_cast<touch_state_t::touch_state_t>(tm | touch_state_t::flick_begin);
				} else if ((tm == touch_state_t::touch) && (msec - det->base_msec > _msecHold)) {
					tm = touch_state_t::hold_begin;
				}
			} else {
				*(static_cast<touch_point_t *>(det)) = *tp;

				tm = touch_state_t::touch_begin;

				if (msec - det->base_msec > _msecHold || abs(det->base_x - tp->x) > ((_flickThresh + 1) << 2) // TODO:検討、タッチ座標が大きくずれた場合にカウントをクリアするか否か
				    || abs(det->base_y - tp->y) > ((_flickThresh + 1) << 2)) {
					det->click_count = 0;
				}
				det->base_msec = msec;
				det->base_x    = tp->x;
				det->base_y    = tp->y;
				det->prev      = det->base;
			}
		}
		if (tm & touch_state_t::mask_moving) {
			det->x = tp->x;
			det->y = tp->y;
		}
	} else {
		tm = (tm & touch_state_t::mask_touch)
		         ? static_cast<touch_state_t::touch_state_t>((tm | touch_state_t::mask_change) & ~touch_state_t::mask_touch)
		         : touch_state_t::none;

		if (tm == touch_state_t::touch_end) {
			// 連続タップ判定のためにbase_msecを更新
			det->base_msec = msec;
			det->click_count++;
		}
	}
	det->state = tm;
	return true;
}

bool TouchButtonGeneric_Class::update_detail(touch_detail_t *det, std::uint32_t msec) {
	touch_state_t::touch_state_t tm = det->state;
	if (tm == touch_state_t::none) {
		return false;
	}
	tm = static_cast<touch_state_t::touch_state_t>(tm & ~touch_state_t::mask_change);
	if (tm & touch_state_t::touch) {
		det->prev_x = det->x;
		det->prev_y = det->y;
		if ((tm == touch_state_t::touch) && (msec - det->base_msec > _msecHold)) {
			tm = touch_state_t::hold_begin;
		}
	} else {
		tm = touch_state_t::none;
	}
	det->state = tm;
	return true;
}
