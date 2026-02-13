/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/
//
//  MARK: Display Override
//
/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/

#include "../../hal.h"

#ifdef TARGET__M5STICK_C_PLUS2

#include "../../../display/display.h"
#include "../../../display/display_timing_type_def.h"
#include "../../../user/user_display_design.h"
#include <math.h> // for sin, cons
#define deg_to_rad(deg) (((deg) / 360.0) * 2.0 * M_PI)

extern std::unique_ptr<OpenFontRender> render;
extern struct SystemMessageRenderParam smrp;

const uint16_t MY_DARKGRAY = 12678;

volatile bool PlayLoadAnimation = false;
TaskHandle_t LoadAnimationTask;
void loadAnimationLoop(void *pvParameters) {
	// Animation parameters
	const uint16_t START_X = 60;
	const uint16_t START_Y = 70;
	const uint8_t ROTATION_SPEED = 2;
	const uint8_t DOT_RADIUS = 3;
	const uint8_t DOT_SPACING = 30;
	const uint8_t NUM_DOTS = 5;
	const uint8_t PHASE_ANGLE_STEP = 90 / NUM_DOTS;
	const uint8_t BASE_DELAY = 15;
	const uint8_t DELAY_VARIATION = 5;
	const uint16_t MAX_ANGLE = 180;
	const uint8_t COLOR_BASE = 10;
	const uint8_t MAX_BRIGHTNESS = 255;

	int16_t angle_degrees = 0;

	while (PlayLoadAnimation) {
		float angle_rad = deg_to_rad(angle_degrees);

		// Pre-calculate brightness values for all dots
		uint8_t brightness[NUM_DOTS];
		uint16_t dot_colors[NUM_DOTS];
		for (int k = 0; k < NUM_DOTS; k++) {
			brightness[k] = abs(sin(deg_to_rad(k * PHASE_ANGLE_STEP + angle_degrees))) * MAX_BRIGHTNESS;
			dot_colors[k] = HAL.Display.color565(COLOR_BASE, brightness[k] / 2, brightness[k]);
		}

		// Draw dots
		HAL.Display.takeSemaphoreMutex(portMAX_DELAY);
		for (int k = 0; k < NUM_DOTS; k++) {
			HAL.Display.fillEllipse(START_X + k * DOT_SPACING, START_Y, DOT_RADIUS, DOT_RADIUS, dot_colors[k]);
		}
		HAL.Display.giveSemaphoreMutex();

		HAL.update();

		// Variable delay for smooth animation
		uint8_t delay_time = BASE_DELAY + DELAY_VARIATION * (1 - sin(angle_rad));
		HAL.delay(delay_time);

		// Update angle
		angle_degrees = (angle_degrees <= ROTATION_SPEED) ? MAX_ANGLE : (angle_degrees - ROTATION_SPEED);

		HAL.update();
	}

	LoadAnimationTask = NULL;
	vTaskDelete(NULL);
}

/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/
//
//  Override Functions
//
/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/

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
		smrp.LEFT_OFFSET    = 3;
		smrp.MSGBOX_HEIGHT  = 30;
		smrp.MSGBOX_WIDTH   = smrp.DISPLAY_WIDTH;

		HAL.Display.giveSemaphoreMutex();
		break;
	case DisplayTiming::SS_INIT_BEGIN: {
		uint8_t wait_sec = va_arg(ap, int);

		HAL.Display.takeSemaphoreMutex(portMAX_DELAY); // Take mutex semaphore
		initMsgBox(CursorPosition::DUAL_LINE);
		render->printf("起動モードの選択 （%d秒）\n", wait_sec);
		render->setFontColor(TFT_YELLOW);
		render->printf("← 設定モード");
		render->setCursor(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET);
		render->setFontColor(TFT_WHITE);
		render->printf("\n");
		render->seekCursor(115, 0);
		render->printf("↓ 通常モード");
		HAL.Display.giveSemaphoreMutex(); // Give (release) mutex semaphore

		dt = DisplayTiming::IGNORE_DEFAULT_PROCESS;
		break;
	}
	case DisplayTiming::SS_MODE_ENTRY: {

		HAL.Display.takeSemaphoreMutex(portMAX_DELAY); // Take mutex semaphore
		initMsgBox(CursorPosition::DUAL_LINE);
		render->printf("設定を初期化しますか?\n");
		render->printf("← No");
		render->setCursor(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET);
		render->setFontColor(TFT_WHITE);
		render->printf("\n");
		render->seekCursor(115, 0);
		render->setFontColor(TFT_YELLOW);
		render->printf("↓ Yes");
		render->setFontColor(TFT_WHITE);
		HAL.Display.giveSemaphoreMutex(); // Give (release) mutex semaphore

		dt = DisplayTiming::IGNORE_DEFAULT_PROCESS;
		break;
	}
	case DisplayTiming::SS_READY: {

		PlayLoadAnimation = false;
		while (LoadAnimationTask != NULL) {
			HAL.delay(1);
		}

		break;
	}
	case DisplayTiming::DISPLAY_FW_VERSION: {
		const char *dev_id      = va_arg(ap, const char *);
		int main_version        = va_arg(ap, int);
		int miner_version       = va_arg(ap, int);
		const char *release_tag = va_arg(ap, const char *);
		int hold_sec            = va_arg(ap, int);

		HAL.Display.takeSemaphoreMutex(portMAX_DELAY); // Take mutex semaphore
		initMsgBox(CursorPosition::DUAL_LINE);
		for (uint8_t c = hold_sec; c != 0; c--) {
			render->setCursor(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET);
			render->printf("デバイスID: %s\nFW バージョン: %d.%d ", dev_id, main_version, miner_version);

			render->setFontColor(TFT_YELLOW);
			render->printf("%.6s", release_tag);
			render->setFontColor(TFT_WHITE);

			render->printf("  [");
			render->setFontColor(TFT_BLACK);
			int32_t x = render->getCursorX();
			int32_t y = render->getCursorY();
			render->printf("%d", c + 1);
			render->setCursor(x, y);
			render->setFontColor(TFT_WHITE);
			render->printf("%d]", c);

			HAL.Display.giveSemaphoreMutex();
			HAL.delay(1000);
			HAL.Display.takeSemaphoreMutex(portMAX_DELAY);
		}
		initMsgBox();
		HAL.Display.giveSemaphoreMutex(); // Give (release) mutex semaphore

		dt = DisplayTiming::IGNORE_DEFAULT_PROCESS;
		break;
	}
	case DisplayTiming::SYSTEM_INIT_DONE: {
		const char *dev_id = va_arg(ap, const char *);
		int sensor_num     = va_arg(ap, int);

		HAL.Display.takeSemaphoreMutex(portMAX_DELAY); // Take mutex semaphore
		initMsgBox(CursorPosition::DUAL_LINE);
		render->setFontColor(TFT_GREENYELLOW);
		render->printf("システムを起動する準備が\n完了しました!");
		render->setFontColor(TFT_WHITE);
		HAL.Display.giveSemaphoreMutex(); // Give (release) mutex semaphore

		HAL.delay(3000);

		HAL.Display.takeSemaphoreMutex(portMAX_DELAY); // Take mutex semaphore
		HAL.Display.fillScreen(TFT_BLACK);
		HAL.Display.giveSemaphoreMutex(); // Give (release) mutex semaphore

		// Apply user display design
		cb_SystemReady(render, dev_id, sensor_num);

		dt = DisplayTiming::IGNORE_DEFAULT_PROCESS;
		break;
	}
	default:
		break;
	}
	return dt;
}

void updateDisplayOverrideBottom(std::unique_ptr<OpenFontRender> &render,
                                 enum DisplayTiming first_dt,
                                 enum DisplayTiming current_dt,
                                 va_list ap) {
	switch (first_dt) {
	case DisplayTiming::SYSTEM_INIT_BEGIN: {
		HAL.Display.takeSemaphoreMutex(portMAX_DELAY); // Take mutex semaphore
		render->setFontSize(25);
		render->setCursor(10, 0);
		render->setFontColor(TFT_WHITE);
		render->printf("YRIT IoT Device\n");

		render->setFontSize(13);
		render->setFontColor(TFT_ORANGE);
		render->printf("- M5Stick C Plus Edition -\n");
		render->setFontColor(TFT_WHITE);
		HAL.Display.giveSemaphoreMutex(); // Give (release) mutex semaphore

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
		break;
	}
	case DisplayTiming::SYSTEM_INIT_DONE: {

		PlayLoadAnimation = false;
		while (LoadAnimationTask != NULL) {
			HAL.delay(1);
		}
		HAL.Display.takeSemaphoreMutex(portMAX_DELAY); // Take mutex semaphore
		HAL.Display.fillScreen(TFT_BLACK);
		HAL.Display.giveSemaphoreMutex(); // Give (release) mutex semaphore

		HAL.delay(1);
		break;
	}
	default:
		break;
	}
}

#endif /* TARGET__M5STICK_C_PLUS */