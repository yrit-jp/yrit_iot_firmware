/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/
//
//  MARK: Display Override
//
/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/

#include "../../hal.h"

#ifdef TARGET__M5STACK_BASIC

#include "../../../display/display.h"
#include "../../../display/display_timing_type_def.h"
#include "../../../user/user_display_design.h"
#include <math.h> // for sin, cons
#define deg_to_rad(deg) (((deg) / 360.0) * 2.0 * M_PI)

extern struct SystemMessageRenderParam smrp;

volatile bool PlayLoadAnimation = false;
TaskHandle_t LoadAnimationTask;
void loadAnimationLoop(void *pvParameters) {
	uint16_t center_x = 160; // Circle center X
	uint16_t center_y = 160; // Circle center Y
	uint16_t i        = 0;   // Degree
	uint8_t up        = 5;   // Round speed
	uint8_t r         = 20;  // Main circle radius
	uint8_t cr        = 2;   // Sub circle radius

	while (PlayLoadAnimation) {

		HAL.Display.takeSemaphoreMutex(portMAX_DELAY);
		for (int k = 0; k < 5; k++) {
			uint16_t color = HAL.Display.color565((51 * (k + 1)), (51 * (k + 1)), (51 * (k + 1)));
			HAL.Display.fillEllipse(center_x + r * cos(deg_to_rad(i) - k * 0.8), center_y - r * sin(deg_to_rad(i) - k * 0.8), cr, cr, color);
		}
		HAL.Display.giveSemaphoreMutex();

		HAL.delay(15 + 5 * (1 - sin(deg_to_rad(i))));

		HAL.Display.takeSemaphoreMutex(portMAX_DELAY);
		for (int k = 0; k < 5; k++) {
			HAL.Display.fillEllipse(center_x + r * cos(deg_to_rad(i) - k * 0.8), center_y - r * sin(deg_to_rad(i) - k * 0.8), cr, cr, TFT_BLACK);
		}
		HAL.Display.giveSemaphoreMutex();

		i = ((i - up) <= 0) ? 360 : (i - up);
	}
	LoadAnimationTask = NULL;
	vTaskDelete(NULL);
}

enum DisplayTiming updateDisplayOverrideTop(std::unique_ptr<OpenFontRender> &render,
                                            enum DisplayTiming dt,
                                            va_list ap) {
	switch (dt) {
	case DisplayTiming::SYSTEM_INIT_BEGIN:
		HAL.Display.takeSemaphoreMutex(portMAX_DELAY);

		// Set parameters
		smrp.DISPLAY_WIDTH  = HAL.Display.width();
		smrp.DISPLAY_HEIGHT = HAL.Display.height();
		smrp.FONT_SIZE      = 16;
		smrp.BOTTOM_OFFSET  = 15;
		smrp.LEFT_OFFSET    = 0;
		smrp.MSGBOX_HEIGHT  = 30;
		smrp.MSGBOX_WIDTH   = smrp.DISPLAY_WIDTH;

		HAL.Display.giveSemaphoreMutex();
		break;
	case DisplayTiming::SS_READY:

		PlayLoadAnimation = false;
		while (LoadAnimationTask != NULL) {
			HAL.delay(1);
		}

		break;
	default:
		break;
	}
	return dt;
}

void updateDisplayOverrideBottom(std::unique_ptr<OpenFontRender> &render,
                                 enum DisplayTiming first_dt,
                                 enum DisplayTiming current_dt,
                                 va_list ap) {

	if (first_dt == DisplayTiming::SYSTEM_INIT_BEGIN) {
		/* Rendering Title */
		HAL.Display.takeSemaphoreMutex(portMAX_DELAY);
		render->setFontSize(40);
		render->setCursor((smrp.DISPLAY_WIDTH) / 2, 0);
		render->setFontColor(TFT_WHITE);
		render->cprintf("YRIT\nIoT Device\n");

		render->setCursor(smrp.DISPLAY_WIDTH / 2, render->getCursorY());
		render->setFontSize(13);
		render->setFontColor(TFT_CYAN);
		render->cprintf("\n- M5Stack Edition -");
		render->setFontColor(TFT_WHITE);
		HAL.Display.giveSemaphoreMutex();

		PlayLoadAnimation = true;
		// SYSTEM_READY でタスクをDelete
		const uint8_t TOP_PRIORITY    = 25; // 0 ~ 25
		const int STACK_SIZE          = 2048;
		const uint8_t USE_CORE_NUMBER = 0;
		xTaskCreatePinnedToCore(loadAnimationLoop,
		                        "LoadAnimation",
		                        STACK_SIZE,
		                        NULL,
		                        TOP_PRIORITY,
		                        &LoadAnimationTask,
		                        USE_CORE_NUMBER);
	}

	if (first_dt == DisplayTiming::SYSTEM_INIT_DONE) {

		PlayLoadAnimation = false;
		while (LoadAnimationTask != NULL) {
			HAL.delay(1);
		}

		HAL.Display.takeSemaphoreMutex(portMAX_DELAY);
		HAL.Display.fillScreen(TFT_BLACK);
		HAL.Display.giveSemaphoreMutex();
		HAL.delay(1);
	}
}

#endif /* TARGET__M5STACK_BASIC */