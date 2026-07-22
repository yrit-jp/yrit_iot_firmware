#ifndef ICON_H
#define ICON_H

#include "../../hal/hal.h"

extern const uint16_t BluetoothIconWidth;
extern const uint16_t BluetoothIconHeight;
extern const uint16_t BluetoothIcon_RLE_RunCount;
extern const uint16_t BluetoothIcon_Palette[];
extern const uint8_t BluetoothIcon_RLE_Packed[];

extern const uint16_t DynamicspidIconWidth;
extern const uint16_t DynamicspidIconHeight;
extern const uint16_t DynamicspidIcon_RLE_RunCount;
extern const uint16_t DynamicspidIcon_Palette[];
extern const uint8_t DynamicspidIcon_RLE_Packed[];

static constexpr float deg_to_rad = 0.017453292519943295769236907684886;

inline void fill_arc_helper(int32_t cx, int32_t cy, int32_t oradius_x, int32_t iradius_x, int32_t oradius_y, int32_t iradius_y, float start, float end, uint16_t color) {
	float s_cos  = (cosf(start * deg_to_rad));
	float e_cos  = (cosf(end * deg_to_rad));
	float sslope = s_cos / (sinf(start * deg_to_rad));
	float eslope = -1000000;
	if (end != 360.0)
		eslope = e_cos / (sinf(end * deg_to_rad));
	float swidth = 0.5 / s_cos;
	float ewidth = -0.5 / e_cos;

	bool start180 = !(start < 180);
	bool end180   = end < 180;
	bool reversed = start + 180 < end or (end < start and start < end + 180);

	int32_t xleft  = -oradius_x;
	int32_t xright = oradius_x + 1;
	int32_t y      = -oradius_y;
	int32_t ye     = oradius_y;
	if (not reversed) {
		if ((end >= 270 or end < 90) and (start >= 270 or start < 90))
			xleft = 0;
		else if (end < 270 and end >= 90 and start < 270 and start >= 90)
			xright = 1;
		if (end >= 180 and start >= 180)
			ye = 0;
		else if (end < 180 and start < 180)
			y = 0;
	}
	/*
	if (y < _clip_t - cy) y = _clip_t - cy;
	if (ye > _clip_b - cy + 1) ye = _clip_b - cy + 1;

	if (xleft < _clip_l - cx) xleft = _clip_l - cx;
	if (xright > _clip_r - cx + 1) xright = _clip_r - cx + 1;
	*/

	bool trueCircle = (oradius_x == oradius_y) and (iradius_x == iradius_y);

	int32_t iradius_y2 = iradius_y * (iradius_y - 1);
	int32_t iradius_x2 = iradius_x * (iradius_x - 1);
	float irad_rate    = iradius_x2 and iradius_y2 ? (float)iradius_x2 / (float)iradius_y2 : 0;

	int32_t oradius_y2 = oradius_y * (oradius_y + 1);
	int32_t oradius_x2 = oradius_x * (oradius_x + 1);
	float orad_rate    = oradius_x2 and oradius_y2 ? (float)oradius_x2 / (float)oradius_y2 : 0;

	do {
		int32_t y2        = y * y;
		int32_t compare_o = oradius_y2 - y2;
		int32_t compare_i = iradius_y2 - y2;
		if (not trueCircle) {
			compare_i = floorf(compare_i * irad_rate);
			compare_o = ceilf(compare_o * orad_rate);
		}
		int32_t xe = ceilf(sqrtf(compare_o));
		int32_t x  = 1 - xe;

		if (x < xleft)
			x = xleft;
		if (xe > xright)
			xe = xright;
		float ysslope = (y + swidth) * sslope;
		float yeslope = (y + ewidth) * eslope;
		int len       = 0;
		do {
			bool flg1  = start180 != (x <= ysslope);
			bool flg2  = end180 != (x <= yeslope);
			int32_t x2 = x * x;
			if (x2 >= compare_i and ((flg1 and flg2) or (reversed and (flg1 or flg2))) and x != xe and x2 < compare_o) {
				++len;
			} else {
				if (len) {
					HAL.Display.drawFastHLine(cx + x - len, cy + y, len, color);
					len = 0;
				}
				if (x2 >= compare_o)
					break;
				if (x < 0 and x2 < compare_i) {
					x = -x;
				}
			}
		} while (++x <= xe);
	} while (++y <= ye);
}

inline void fillEllipseArc(int32_t x, int32_t y, int32_t r0x, int32_t r1x, int32_t r0y, int32_t r1y, float start, float end, uint16_t color) {
	if (r0x < r1x)
		std::swap(r0x, r1x);
	if (r0y < r1y)
		std::swap(r0y, r1y);
	if (r1x < 0)
		return;
	if (r1y < 0)
		return;

	bool equal = fabsf(start - end) < std::numeric_limits<float>::epsilon();
	start      = fmodf(start, 360);
	end        = fmodf(end, 360);
	if (start < 0)
		start += 360.0;
	if (end < 0)
		end += 360.0;
	if (not equal and (fabsf(start - end) <= 0.0001)) {
		start = .0;
		end   = 360.0;
	}

	// startWrite();
	fill_arc_helper(x, y, r0x, r1x, r0y, r1y, start, end, color);
	// endWrite();
}

inline void fillArc(int32_t x, int32_t y, int32_t r0, int32_t r1, float angle0, float angle1, uint16_t color) {
	fillEllipseArc(x, y, r0, r1, r0, r1, angle0, angle1, color);
}

enum class TfCardStatusT {
	UNKNOWN,
	LOGGING,
	NO_LOGGING,
	UNMOUNTED
};

inline void Icon_TfCardStatus(int x, int y, TfCardStatusT status, uint16_t bgcolor = 0x3186) {
	static enum TfCardStatusT last_status = TfCardStatusT::UNKNOWN;
	if (status == last_status) {
		return;
	}

	uint16_t card_color, font_color;

	switch (status) {
	case TfCardStatusT::LOGGING:
		card_color = HAL.Display.color565(0, 168, 236);
		font_color = 0xFFFF;
		break;
	case TfCardStatusT::NO_LOGGING:
		card_color = 0xD69A;
		font_color = 0x0000;
		break;
	case TfCardStatusT::UNMOUNTED:
		card_color = bgcolor;
		font_color = bgcolor;
		break;
	default:
		card_color = bgcolor;
		font_color = bgcolor;
		break;
	}

	HAL.Display.fillRect(x, y, 15, 20, card_color);
	HAL.Display.fillRect(x, y + 5, 1, 4, bgcolor);
	HAL.Display.fillTriangle(x + 12, y, x + 14, y, x + 14, y + 2, bgcolor);

	// S
	HAL.Display.drawPixel(x + 3, y + 10, font_color);
	HAL.Display.drawPixel(x + 4, y + 10, font_color);
	HAL.Display.drawPixel(x + 5, y + 10, font_color);

	HAL.Display.drawPixel(x + 6, y + 11, font_color);
	HAL.Display.drawPixel(x + 2, y + 11, font_color);
	HAL.Display.drawPixel(x + 2, y + 12, font_color);

	HAL.Display.drawPixel(x + 3, y + 13, font_color);
	HAL.Display.drawPixel(x + 4, y + 13, font_color);
	HAL.Display.drawPixel(x + 5, y + 13, font_color);

	HAL.Display.drawPixel(x + 6, y + 14, font_color);
	HAL.Display.drawPixel(x + 6, y + 15, font_color);
	HAL.Display.drawPixel(x + 2, y + 15, font_color);

	HAL.Display.drawPixel(x + 3, y + 16, font_color);
	HAL.Display.drawPixel(x + 4, y + 16, font_color);
	HAL.Display.drawPixel(x + 5, y + 16, font_color);

	// D
	HAL.Display.drawFastVLine(x + 8, y + 10, 7, font_color);
	HAL.Display.drawFastVLine(x + 12, y + 11, 5, font_color);
	HAL.Display.drawFastHLine(x + 9, y + 10, 3, font_color);
	HAL.Display.drawFastHLine(x + 9, y + 16, 3, font_color);
}

inline void Icon_WiFiStatus(int x, int y, int rssi, uint16_t bgcolor = 0x3186) {
	static bool clear_slash_line = false;
	int span                     = 2;

	if (rssi >= -100 and clear_slash_line) {
		for (int i = -2; i < 6; i++) {
			if (i < 0 or 3 < i) {
				HAL.Display.drawLine(x - 3 * span + i - 1, y - (7 * span) - 3, x - 3 * span + i + 11, y + 1, bgcolor);
			} else {
				HAL.Display.drawLine(x - 3 * span + i - 1, y - (7 * span) - 3, x - 3 * span + i + 11, y + 1, bgcolor);
			}
		}
		clear_slash_line = false;
	}

	fillArc(x, y, 6 * span, 7 * span, 228, 313, ((rssi > -67) ? 1055 : 0xD69A)); // out
	fillArc(x, y, 3 * span, 4 * span, 228, 313, ((rssi > -75) ? 1055 : 0xD69A)); // middle
	fillArc(x, y, 0, span, 228, 313, ((rssi > -80) ? 1055 : 0xD69A));            // min

	if (rssi < -100) {
		for (int i = -2; i < 6; i++) {
			if (i < 0 or 3 < i) {
				HAL.Display.drawLine(x - 3 * span + i - 1, y - (7 * span) - 3, x - 3 * span + i + 11, y + 1, TFT_BLACK);
			} else {
				HAL.Display.drawLine(x - 3 * span + i - 1, y - (7 * span) - 3, x - 3 * span + i + 11, y + 1, TFT_RED);
			}
		}
		clear_slash_line = true;
	}
}

inline void Icon_EthernetStatus(int x, int y, bool isConnected) {
	constexpr uint8_t xOffset = 3;
	constexpr uint8_t yOffset = 2;

	HAL.Display.fillRect(x, y, 22, 20, TFT_WHITE);
	HAL.Display.fillRect(x - 1 + xOffset, y + yOffset, 18, 16, TFT_DARKGREY);
	for (int i = 0; i < 5; i++) {
		HAL.Display.fillRect(x + 1 + (i * 3) + xOffset, y + 2 + yOffset, 2, 5, TFT_YELLOW);
	}
	HAL.Display.fillRect(x - 1 + xOffset, y + 1 + yOffset + 12, 4, 3, TFT_WHITE);
	HAL.Display.fillRect(x - 1 + xOffset + 14, y + 1 + yOffset + 12, 4, 3, TFT_WHITE);

	if (isConnected) {
		return;
	}

	x += 1;
	HAL.Display.drawLine(x - 2, y + 19, x + 19 - 2, y, TFT_BLACK);
	HAL.Display.drawLine(x - 1, y + 19, x + 19 - 1, y, TFT_RED);
	HAL.Display.drawLine(x, y + 19, x + 19, y, TFT_RED);
	HAL.Display.drawLine(x + 1, y + 19, x + 19 + 1, y, TFT_RED);
	HAL.Display.drawLine(x + 2, y + 19, x + 19 + 2, y, TFT_BLACK);
}

inline void Icon_BatteryStatus(int x, int y, int level) {
	// draw outline
	HAL.Display.fillRect(x - 3, y + 5, 3, 10, 0xD69A);
	HAL.Display.fillRect(x, y, 27, 20, 0xD69A);
	HAL.Display.fillRect(x + 2, y + 2, 23, 16, 0x0000);

	HAL.Display.fillRect(x + 4, y + 4, 5, 12, ((level >= 75) ? 0x07E0 : 0x0000));
	HAL.Display.fillRect(x + 4 + 7, y + 4, 5, 12, ((level >= 25) ? 0x07E0 : 0x0000));
	HAL.Display.fillRect(x + 4 + 14, y + 4, 5, 12, ((level < 25) ? 0xF800 : 0x07E0));
}

// パレット化RLE(ニブルパッキング)デコーダ。
// 1バイト = [透過フラグ1bit][パレットindex 3bit][count 4bit]。
// count==0(下位4bit)はエスケープを意味し、次の1バイトに実countを格納する。
// 透過ランは描画をスキップし、カーソルだけ進める。
inline void drawRLEIcon(int32_t x, int32_t y, int32_t w,
                        const uint16_t *palette,
                        const uint8_t *packed,
                        uint16_t runCount) {
	int32_t px = x, py = y;
	uint32_t bytePos = 0;

	for (uint16_t run = 0; run < runCount; run++) {
		uint8_t b = pgm_read_byte(&packed[bytePos++]);

		bool isTransparent = (b & 0x80) != 0;
		uint8_t paletteIdx = (b >> 4) & 0x07;
		uint8_t countLow   = b & 0x0F;

		uint8_t count;
		if (countLow == 0) {
			count = pgm_read_byte(&packed[bytePos++]); // エスケープ: 実カウントは次バイト
		} else {
			count = countLow;
		}

		if (not isTransparent) {
			uint16_t color = pgm_read_word(&palette[paletteIdx]);
			HAL.Display.drawFastHLine(px, py, count, color);
		}
		// 透過ランは描画せずカーソルのみ進める

		px += count;
		if (px >= x + w) {
			px = x;
			py++;
		}
	}
}

#endif