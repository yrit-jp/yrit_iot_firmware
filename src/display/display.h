
#include "../hal/hal.h"

#ifndef DISPLAY_H
#define DISPLAY_H
#include "display_timing_type_def.h"
#include <OpenFontRender.h>

struct SystemMessageRenderParam {
	uint16_t DISPLAY_WIDTH;
	uint16_t DISPLAY_HEIGHT;
	uint16_t MSGBOX_HEIGHT;
	uint16_t MSGBOX_WIDTH;
	uint16_t FONT_SIZE;
	uint16_t BOTTOM_OFFSET;
	uint16_t LEFT_OFFSET;
};

enum class CursorPosition : uint8_t {
	SINGLE_LINE, // 1行表示用（下寄せ）
	DUAL_LINE,   // 2行表示用（上寄せ）
};

class VaListGuard {
public:
	VaListGuard(va_list &ap) : _ap(ap) {}
	~VaListGuard() { va_end(_ap); }

	// コピーコンストラクタと代入演算子を削除
	VaListGuard(const VaListGuard &)            = delete;
	VaListGuard &operator=(const VaListGuard &) = delete;

private:
	va_list &_ap;
};

void initMsgBox(CursorPosition position = CursorPosition::SINGLE_LINE);
void updateDisplay(enum DisplayTiming dt, ...);
enum DisplayTiming updateDisplayOverrideTop(std::unique_ptr<OpenFontRender> &render,
                                            enum DisplayTiming dt,
                                            va_list ap);
void updateDisplayOverrideBottom(std::unique_ptr<OpenFontRender> &render,
                                 enum DisplayTiming first_dt,
                                 enum DisplayTiming current_dt,
                                 va_list ap);

#endif // MY_DISPLAY_H