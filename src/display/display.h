
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

void updateDisplay(enum DisplayTiming dt, ...);
enum DisplayTiming updateDisplayOverrideTop(std::unique_ptr<OpenFontRender> &render,
                                            enum DisplayTiming dt,
                                            va_list ap);
void updateDisplayOverrideBottom(std::unique_ptr<OpenFontRender> &render,
                                 enum DisplayTiming first_dt,
                                 enum DisplayTiming current_dt,
                                 va_list ap);

#endif // MY_DISPLAY_H