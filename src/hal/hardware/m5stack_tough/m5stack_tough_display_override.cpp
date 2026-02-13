/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/
//
//  MARK: Display Override
//
/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/

#include "../../hal.h"

#ifdef TARGET__M5STACK_TOUGH

#include "../../../display/display.h"
#include "../../../display/display_timing_type_def.h"
#include "../../../user/user_display_design.h"
#include <math.h> // for sin, cons
#define deg_to_rad(deg) (((deg) / 360.0) * 2.0 * M_PI)

extern struct SystemMessageRenderParam smrp;

volatile bool PlayLoadAnimation = false;
TaskHandle_t LoadAnimationTask;
void loadAnimationLoop(void *pvParameters) {
	// Animation parameters
	const uint16_t CENTER_X = 160;
	const uint16_t CENTER_Y = 160;
	const uint8_t ROTATION_SPEED = 5;
	const uint8_t ORBIT_RADIUS = 20;
	const uint8_t DOT_RADIUS = 2;
	const uint8_t NUM_DOTS = 5;
	const float DOT_SPACING_ANGLE = 0.8f; // radians between dots
	const uint8_t BASE_DELAY = 15;
	const uint8_t DELAY_VARIATION = 5;

	// Pre-calculate grayscale colors for each dot
	const uint16_t dot_colors[NUM_DOTS] = {
		HAL.Display.color565(51, 51, 51),
		HAL.Display.color565(102, 102, 102),
		HAL.Display.color565(153, 153, 153),
		HAL.Display.color565(204, 204, 204),
		HAL.Display.color565(255, 255, 255)
	};

	uint16_t angle_degrees = 0;

	while (PlayLoadAnimation) {
		float angle_rad = deg_to_rad(angle_degrees);

		// Pre-calculate positions for all dots
		int16_t dot_x[NUM_DOTS];
		int16_t dot_y[NUM_DOTS];
		for (int k = 0; k < NUM_DOTS; k++) {
			float dot_angle = angle_rad - k * DOT_SPACING_ANGLE;
			dot_x[k] = CENTER_X + ORBIT_RADIUS * cos(dot_angle);
			dot_y[k] = CENTER_Y - ORBIT_RADIUS * sin(dot_angle);
		}

		// Draw dots
		HAL.Display.takeSemaphoreMutex(portMAX_DELAY);
		for (int k = 0; k < NUM_DOTS; k++) {
			HAL.Display.fillEllipse(dot_x[k], dot_y[k], DOT_RADIUS, DOT_RADIUS, dot_colors[k]);
		}
		HAL.Display.giveSemaphoreMutex();

		// Variable delay for smooth animation
		uint8_t delay_time = BASE_DELAY + DELAY_VARIATION * (1 - sin(angle_rad));
		HAL.delay(delay_time);

		// Erase dots (draw in black)
		HAL.Display.takeSemaphoreMutex(portMAX_DELAY);
		for (int k = 0; k < NUM_DOTS; k++) {
			HAL.Display.fillEllipse(dot_x[k], dot_y[k], DOT_RADIUS, DOT_RADIUS, TFT_BLACK);
		}
		HAL.Display.giveSemaphoreMutex();

		// Update angle
		angle_degrees = (angle_degrees <= ROTATION_SPEED) ? 360 : (angle_degrees - ROTATION_SPEED);
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
		const uint8_t TOP_PRIORITY    = 24; // 0 ~ 24
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

#endif /* TARGET__M5STACK_TOUGH */