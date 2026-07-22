/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/
//
//  MARK: Display Override
//
/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/

#include "../../hal.h"

#ifdef TARGET__M5STACK_UNIT_C6L

#include "../../../display/display.h"
#include "../../../display/display_timing_type_def.h"
#include "../../../user/user_display_design.h"

extern struct SystemMessageRenderParam smrp;

volatile bool PlayLoadAnimation = false;
TaskHandle_t LoadAnimationTask;
void loadAnimationLoop(void *pvParameters) {
	const int16_t ANIM_Y   = 23;
	const int16_t BALL_R   = 2;
	const int16_t X_MIN    = BALL_R + 2;
	const int16_t X_MAX    = 63 - BALL_R - 2;
	const int16_t STEP     = 2;
	const uint8_t DELAY_MS = 20;

	int16_t x      = X_MIN;
	int16_t prev_x = x;
	int8_t dir     = 1;

	while (PlayLoadAnimation) {
		HAL.Display.takeSemaphoreMutex(portMAX_DELAY);
		HAL.Display.fillCircle(prev_x, ANIM_Y, BALL_R, TFT_BLACK);
		HAL.Display.fillCircle(x, ANIM_Y, BALL_R, TFT_WHITE);
		HAL.Display.giveSemaphoreMutex();

		prev_x = x;
		x += dir * STEP;
		if (x >= X_MAX) {
			x   = X_MAX;
			dir = -1;
		}
		if (x <= X_MIN) {
			x   = X_MIN;
			dir = 1;
		}

		HAL.delay(DELAY_MS);
	}

	LoadAnimationTask = NULL;
	vTaskDelete(NULL);
}

enum DisplayTiming updateDisplayOverrideTop(std::unique_ptr<OpenFontRender> &render,
                                            enum DisplayTiming dt,
                                            va_list ap) {
	if (HAL.isWakeFromSleep()) {
		dt = DisplayTiming::IGNORE_DEFAULT_PROCESS;
	}

	switch (dt) {
	case DisplayTiming::SYSTEM_INIT_BEGIN:
		HAL.Display.takeSemaphoreMutex(portMAX_DELAY);

		// Set parameters
		smrp.DISPLAY_WIDTH  = HAL.Display.width();
		smrp.DISPLAY_HEIGHT = HAL.Display.height();
		smrp.FONT_SIZE      = 1;
		smrp.BOTTOM_OFFSET  = 1;
		smrp.LEFT_OFFSET    = 0;
		smrp.MSGBOX_HEIGHT  = 15;
		smrp.MSGBOX_WIDTH   = smrp.DISPLAY_WIDTH;

		HAL.Display.giveSemaphoreMutex();

		dt = DisplayTiming::IGNORE_DEFAULT_PROCESS;
		break;
	case DisplayTiming::SS_INIT_BEGIN: {
		uint8_t wait_sec = va_arg(ap, int);

		HAL.Display.takeSemaphoreMutex(portMAX_DELAY); // Take mutex semaphore
		HAL.Display.fillRect(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET,
		                     smrp.DISPLAY_WIDTH, smrp.MSGBOX_HEIGHT + smrp.BOTTOM_OFFSET, TFT_BLACK);

		HAL.Display.setCursor(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET);
		HAL.Display.println("Press BTN");
		HAL.Display.printf("> config");
		HAL.Display.giveSemaphoreMutex(); // Give (release) mutex semaphore

		dt = DisplayTiming::IGNORE_DEFAULT_PROCESS;
		break;
	}
	case DisplayTiming::SS_MODE_ENTRY:

		HAL.Display.takeSemaphoreMutex(portMAX_DELAY); // Take mutex semaphore
		HAL.Display.fillRect(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET,
		                     smrp.DISPLAY_WIDTH, smrp.MSGBOX_HEIGHT + smrp.BOTTOM_OFFSET, TFT_BLACK);

		HAL.Display.setCursor(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET);
		HAL.Display.println("Press BTN");
		HAL.Display.printf("> init cfg");
		HAL.Display.giveSemaphoreMutex(); // Give (release) mutex semaphore

		dt = DisplayTiming::IGNORE_DEFAULT_PROCESS;
		break;
	case DisplayTiming::SS_CLEAR_ALL:

		HAL.Display.takeSemaphoreMutex(portMAX_DELAY); // Take mutex semaphore
		HAL.Display.fillRect(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET,
		                     smrp.DISPLAY_WIDTH, smrp.MSGBOX_HEIGHT + smrp.BOTTOM_OFFSET, TFT_BLACK);

		HAL.Display.setCursor(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET);
		HAL.Display.println("Initialize");
		HAL.Display.printf("FileSystem");
		HAL.Display.giveSemaphoreMutex(); // Give (release) mutex semaphore

		dt = DisplayTiming::IGNORE_DEFAULT_PROCESS;
		break;
	case DisplayTiming::SS_FORMAT:

		HAL.Display.takeSemaphoreMutex(portMAX_DELAY); // Take mutex semaphore
		HAL.Display.fillRect(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET,
		                     smrp.DISPLAY_WIDTH, smrp.MSGBOX_HEIGHT + smrp.BOTTOM_OFFSET, TFT_BLACK);

		HAL.Display.setCursor(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET);
		HAL.Display.println("Setup");
		HAL.Display.printf("FileSystem");
		HAL.Display.giveSemaphoreMutex(); // Give (release) mutex semaphore

		dt = DisplayTiming::IGNORE_DEFAULT_PROCESS;
		break;
	case DisplayTiming::SS_READY: {

		PlayLoadAnimation = false;
		while (LoadAnimationTask != NULL) {
			HAL.delay(1);
		}

		HAL.Display.takeSemaphoreMutex(portMAX_DELAY); // Take mutex semaphore
		HAL.Display.fillRect(0, 0, smrp.DISPLAY_WIDTH, smrp.DISPLAY_HEIGHT, TFT_BLACK);
		HAL.Display.giveSemaphoreMutex(); // Give (release) mutex semaphore

		HAL.delay(5);

		HAL.Display.takeSemaphoreMutex(portMAX_DELAY); // Take mutex semaphore
		HAL.Display.fillRect(0, 0, smrp.DISPLAY_WIDTH, smrp.DISPLAY_HEIGHT, TFT_BLACK);
		HAL.Display.giveSemaphoreMutex(); // Give (release) mutex semaphore

		HAL.delay(5);

		HAL.Display.takeSemaphoreMutex(portMAX_DELAY); // Take mutex semaphore

		HAL.Display.setCursor(0, 0);
		HAL.Display.println(" BLE Name");
		HAL.Display.println("----------");
		HAL.Display.printf("%s", HAL.getChipUUID().c_str());

		HAL.Display.giveSemaphoreMutex(); // Give (release) mutex semaphore

		cb_AfterSystemDisplayUpdate(render, dt);

		// render.reset();
		dt = DisplayTiming::IGNORE_DEFAULT_PROCESS;
		break;
	}
	case DisplayTiming::DISPLAY_FW_VERSION: {
		const char *dev_id      = va_arg(ap, const char *);
		int main_version        = va_arg(ap, int);
		int miner_version       = va_arg(ap, int);
		const char *release_tag = va_arg(ap, const char *);
		int hold_sec            = va_arg(ap, int);

		HAL.Display.takeSemaphoreMutex(portMAX_DELAY); // Take mutex semaphore
		HAL.Display.fillRect(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET,
		                     smrp.DISPLAY_WIDTH, smrp.MSGBOX_HEIGHT + smrp.BOTTOM_OFFSET, TFT_BLACK);

		HAL.Display.setCursor(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET);
		HAL.Display.printf("%.10s\n", dev_id);
		HAL.Display.printf("%d.%d %.5s", main_version, miner_version, release_tag);

		HAL.Display.giveSemaphoreMutex(); // Give (release) mutex semaphore
		HAL.delay(hold_sec * 1000);

		dt = DisplayTiming::IGNORE_DEFAULT_PROCESS;
		break;
	}
	case DisplayTiming::GPIO_INIT_BEGIN:

		HAL.Display.takeSemaphoreMutex(portMAX_DELAY); // Take mutex semaphore
		HAL.Display.fillRect(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET,
		                     smrp.DISPLAY_WIDTH, smrp.MSGBOX_HEIGHT + smrp.BOTTOM_OFFSET, TFT_BLACK);

		HAL.Display.setCursor(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET);
		HAL.Display.println("Initialize");
		HAL.Display.print("GPIO");
		HAL.Display.giveSemaphoreMutex(); // Give (release) mutex semaphore

		dt = DisplayTiming::IGNORE_DEFAULT_PROCESS;
		break;
	case DisplayTiming::PULSE_COUNTER_INIT_BEGIN:

		HAL.Display.takeSemaphoreMutex(portMAX_DELAY); // Take mutex semaphore
		HAL.Display.fillRect(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET,
		                     smrp.DISPLAY_WIDTH, smrp.MSGBOX_HEIGHT + smrp.BOTTOM_OFFSET, TFT_BLACK);

		HAL.Display.setCursor(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET);
		HAL.Display.println("Initialize");
		HAL.Display.print("Pluse cnt");
		HAL.Display.giveSemaphoreMutex(); // Give (release) mutex semaphore

		dt = DisplayTiming::IGNORE_DEFAULT_PROCESS;
		break;
	case DisplayTiming::PULSE_COUNTER_INIT_TARGET: {
		uint8_t pin = va_arg(ap, int);

		HAL.Display.takeSemaphoreMutex(portMAX_DELAY); // Take mutex semaphore
		HAL.Display.fillRect(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET,
		                     smrp.DISPLAY_WIDTH, smrp.MSGBOX_HEIGHT + smrp.BOTTOM_OFFSET, TFT_BLACK);

		HAL.Display.setCursor(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET);
		HAL.Display.println("Initialize");
		HAL.Display.printf("Pin %d", pin);
		HAL.Display.giveSemaphoreMutex(); // Give (release) mutex semaphore

		dt = DisplayTiming::IGNORE_DEFAULT_PROCESS;
		break;
	}
	case DisplayTiming::I2C_INIT_DEVICE_BEGIN:

		HAL.Display.takeSemaphoreMutex(portMAX_DELAY); // Take mutex semaphore
		HAL.Display.fillRect(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET,
		                     smrp.DISPLAY_WIDTH, smrp.MSGBOX_HEIGHT + smrp.BOTTOM_OFFSET, TFT_BLACK);

		HAL.Display.setCursor(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET);
		HAL.Display.println("Initialize");
		HAL.Display.print("I2C device");
		HAL.Display.giveSemaphoreMutex(); // Give (release) mutex semaphore

		dt = DisplayTiming::IGNORE_DEFAULT_PROCESS;
		break;
	case DisplayTiming::I2C_INIT_DEVICE_TARGET: {
		uint8_t address = va_arg(ap, int);
		int progress    = va_arg(ap, int);
		int i2c_dev_num = va_arg(ap, int);

		HAL.Display.takeSemaphoreMutex(portMAX_DELAY); // Take mutex semaphore
		HAL.Display.fillRect(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET,
		                     smrp.DISPLAY_WIDTH, smrp.MSGBOX_HEIGHT + smrp.BOTTOM_OFFSET, TFT_BLACK);

		HAL.Display.setCursor(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET);
		HAL.Display.printf("(%d/%d)\n", progress, i2c_dev_num);
		HAL.Display.printf("Init 0x%02X", address);
		HAL.Display.giveSemaphoreMutex(); // Give (release) mutex semaphore

		dt = DisplayTiming::IGNORE_DEFAULT_PROCESS;
		break;
	}
	case DisplayTiming::I2C_INIT_DEVICE_FAILED: {
		uint8_t address = va_arg(ap, int);

		HAL.Display.takeSemaphoreMutex(portMAX_DELAY); // Take mutex semaphore
		HAL.Display.fillRect(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET,
		                     smrp.DISPLAY_WIDTH, smrp.MSGBOX_HEIGHT + smrp.BOTTOM_OFFSET, TFT_BLACK);

		HAL.Display.setCursor(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET);
		HAL.Display.printf("Error 0x%02X", address);
		HAL.Display.print("Reboot now");
		HAL.Display.giveSemaphoreMutex(); // Give (release) mutex semaphore

		dt = DisplayTiming::IGNORE_DEFAULT_PROCESS;
		break;
	}
	case DisplayTiming::MODBUS_INIT_PERIPHERAL_BEGIN: {

		HAL.Display.takeSemaphoreMutex(portMAX_DELAY); // Take mutex semaphore
		HAL.Display.fillRect(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET,
		                     smrp.DISPLAY_WIDTH, smrp.MSGBOX_HEIGHT + smrp.BOTTOM_OFFSET, TFT_BLACK);

		HAL.Display.setCursor(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET);
		HAL.Display.println("Initialize");
		HAL.Display.printf("Modbus");
		HAL.Display.giveSemaphoreMutex(); // Give (release) mutex semaphore

		dt = DisplayTiming::IGNORE_DEFAULT_PROCESS;
		break;
	}
	case DisplayTiming::MODBUS_INIT_PERIPHERAL_TARGET: {
		uint8_t port = va_arg(ap, int);

		HAL.Display.takeSemaphoreMutex(portMAX_DELAY); // Take mutex semaphore
		HAL.Display.fillRect(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET,
		                     smrp.DISPLAY_WIDTH, smrp.MSGBOX_HEIGHT + smrp.BOTTOM_OFFSET, TFT_BLACK);

		HAL.Display.setCursor(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET);
		HAL.Display.println("Initialize");
		HAL.Display.printf("Port %d", port);
		HAL.Display.giveSemaphoreMutex(); // Give (release) mutex semaphore

		dt = DisplayTiming::IGNORE_DEFAULT_PROCESS;
		break;
	}
	case DisplayTiming::MODBUS_INIT_DEVICE_BEGIN: {

		HAL.Display.takeSemaphoreMutex(portMAX_DELAY); // Take mutex semaphore
		HAL.Display.fillRect(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET,
		                     smrp.DISPLAY_WIDTH, smrp.MSGBOX_HEIGHT + smrp.BOTTOM_OFFSET, TFT_BLACK);

		HAL.Display.setCursor(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET);
		HAL.Display.println("Initialize");
		HAL.Display.print("Modbus dev");
		HAL.Display.giveSemaphoreMutex(); // Give (release) mutex semaphore

		dt = DisplayTiming::IGNORE_DEFAULT_PROCESS;
		break;
	}
	case DisplayTiming::MODBUS_INIT_DEVICE_TARGET: {
		uint8_t port       = va_arg(ap, int);
		uint8_t id         = va_arg(ap, int);
		int progress       = va_arg(ap, int);
		int modbus_dev_num = va_arg(ap, int);

		HAL.Display.takeSemaphoreMutex(portMAX_DELAY); // Take mutex semaphore
		HAL.Display.fillRect(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET,
		                     smrp.DISPLAY_WIDTH, smrp.MSGBOX_HEIGHT + smrp.BOTTOM_OFFSET, TFT_BLACK);

		HAL.Display.setCursor(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET);
		HAL.Display.printf("(%d/%d) %d\n", progress, modbus_dev_num, port);
		HAL.Display.printf("ID:%d", id);
		HAL.Display.giveSemaphoreMutex(); // Give (release) mutex semaphore

		dt = DisplayTiming::IGNORE_DEFAULT_PROCESS;
		break;
	}
	case DisplayTiming::CAMERA_INIT_BEGIN: {
		HAL.Display.takeSemaphoreMutex(portMAX_DELAY);
		HAL.Display.fillRect(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET,
		                     smrp.DISPLAY_WIDTH, smrp.MSGBOX_HEIGHT + smrp.BOTTOM_OFFSET, TFT_BLACK);
		HAL.Display.setCursor(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET);
		HAL.Display.println("Initialize");
		HAL.Display.print("Camera");
		HAL.Display.giveSemaphoreMutex();
		dt = DisplayTiming::IGNORE_DEFAULT_PROCESS;
		break;
	}
	case DisplayTiming::CAMERA_INIT_FAILED: {
		HAL.Display.takeSemaphoreMutex(portMAX_DELAY);
		HAL.Display.fillRect(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET,
		                     smrp.DISPLAY_WIDTH, smrp.MSGBOX_HEIGHT + smrp.BOTTOM_OFFSET, TFT_BLACK);
		HAL.Display.setCursor(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET);
		HAL.Display.println("Error Cam");
		HAL.Display.print("Reboot now");
		HAL.Display.giveSemaphoreMutex();
		dt = DisplayTiming::IGNORE_DEFAULT_PROCESS;
		break;
	}
	case DisplayTiming::WIFI_TRY_CONNECT: {
		const char *target_ssid = va_arg(ap, const char *);

		HAL.Display.takeSemaphoreMutex(portMAX_DELAY); // Take mutex semaphore
		HAL.Display.fillRect(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET,
		                     smrp.DISPLAY_WIDTH, smrp.MSGBOX_HEIGHT + smrp.BOTTOM_OFFSET, TFT_BLACK);

		HAL.Display.setCursor(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET);
		HAL.Display.println("Try WiFi");
		HAL.Display.printf(">%.16s", target_ssid);
		HAL.Display.giveSemaphoreMutex(); // Give (release) mutex semaphore

		dt = DisplayTiming::IGNORE_DEFAULT_PROCESS;
		break;
	}
	case DisplayTiming::WIFI_SEARCH_TARGET:

		HAL.Display.takeSemaphoreMutex(portMAX_DELAY); // Take mutex semaphore
		HAL.Display.fillRect(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET,
		                     smrp.DISPLAY_WIDTH, smrp.MSGBOX_HEIGHT + smrp.BOTTOM_OFFSET, TFT_BLACK);

		HAL.Display.setCursor(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET);
		HAL.Display.println("Search");
		HAL.Display.print("SSID...");
		HAL.Display.giveSemaphoreMutex(); // Give (release) mutex semaphore

		dt = DisplayTiming::IGNORE_DEFAULT_PROCESS;
		break;
	case DisplayTiming::WIFI_CONNECT_SUCCESS:

		HAL.Display.takeSemaphoreMutex(portMAX_DELAY); // Take mutex semaphore
		HAL.Display.fillRect(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET,
		                     smrp.DISPLAY_WIDTH, smrp.MSGBOX_HEIGHT + smrp.BOTTOM_OFFSET, TFT_BLACK);

		HAL.Display.setCursor(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET);
		HAL.Display.println("WiFi");
		HAL.Display.print("Connected");
		HAL.Display.giveSemaphoreMutex(); // Give (release) mutex semaphore

		dt = DisplayTiming::IGNORE_DEFAULT_PROCESS;
		break;
	case DisplayTiming::WIFI_CONNECT_FAILED:

		HAL.Display.takeSemaphoreMutex(portMAX_DELAY); // Take mutex semaphore
		HAL.Display.fillRect(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET,
		                     smrp.DISPLAY_WIDTH, smrp.MSGBOX_HEIGHT + smrp.BOTTOM_OFFSET, TFT_BLACK);

		HAL.Display.setCursor(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET);
		HAL.Display.println("Error WiFi");
		HAL.Display.print("Reboot now");
		HAL.Display.giveSemaphoreMutex(); // Give (release) mutex semaphore

		dt = DisplayTiming::IGNORE_DEFAULT_PROCESS;
		break;
	case DisplayTiming::WIFI_TARGET_DETECTED: {
		const char *target_ssid = va_arg(ap, const char *);

		HAL.Display.takeSemaphoreMutex(portMAX_DELAY); // Take mutex semaphore
		HAL.Display.fillRect(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET,
		                     smrp.DISPLAY_WIDTH, smrp.MSGBOX_HEIGHT + smrp.BOTTOM_OFFSET, TFT_BLACK);

		HAL.Display.setCursor(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET);
		HAL.Display.println("Find SSID");
		HAL.Display.printf(">%.16s", target_ssid);
		HAL.Display.giveSemaphoreMutex(); // Give (release) mutex semaphore

		dt = DisplayTiming::IGNORE_DEFAULT_PROCESS;
		break;
	}
	case DisplayTiming::WIFI_TARGET_NOT_FOUND: {
		const char *target_ssid = va_arg(ap, const char *);

		HAL.Display.takeSemaphoreMutex(portMAX_DELAY); // Take mutex semaphore
		HAL.Display.fillRect(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET,
		                     smrp.DISPLAY_WIDTH, smrp.MSGBOX_HEIGHT + smrp.BOTTOM_OFFSET, TFT_BLACK);

		HAL.Display.setCursor(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET);
		HAL.Display.println("Miss SSID");
		HAL.Display.printf(">%.16s", target_ssid);
		HAL.Display.giveSemaphoreMutex(); // Give (release) mutex semaphore

		dt = DisplayTiming::IGNORE_DEFAULT_PROCESS;
		break;
	}
	case DisplayTiming::ETH_INIT_BEGIN: {
		HAL.Display.takeSemaphoreMutex(portMAX_DELAY);
		HAL.Display.fillRect(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET,
		                     smrp.DISPLAY_WIDTH, smrp.MSGBOX_HEIGHT + smrp.BOTTOM_OFFSET, TFT_BLACK);
		HAL.Display.setCursor(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET);
		HAL.Display.println("Try Eth");
		HAL.Display.print("LAN...");
		HAL.Display.giveSemaphoreMutex();
		dt = DisplayTiming::IGNORE_DEFAULT_PROCESS;
		break;
	}
	case DisplayTiming::ETH_INIT_SUCCESS: {
		HAL.Display.takeSemaphoreMutex(portMAX_DELAY);
		HAL.Display.fillRect(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET,
		                     smrp.DISPLAY_WIDTH, smrp.MSGBOX_HEIGHT + smrp.BOTTOM_OFFSET, TFT_BLACK);
		HAL.Display.setCursor(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET);
		HAL.Display.println("Eth");
		HAL.Display.print("Connected");
		HAL.Display.giveSemaphoreMutex();
		dt = DisplayTiming::IGNORE_DEFAULT_PROCESS;
		break;
	}
	case DisplayTiming::ETH_INIT_FAILED: {
		HAL.Display.takeSemaphoreMutex(portMAX_DELAY);
		HAL.Display.fillRect(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET,
		                     smrp.DISPLAY_WIDTH, smrp.MSGBOX_HEIGHT + smrp.BOTTOM_OFFSET, TFT_BLACK);
		HAL.Display.setCursor(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET);
		HAL.Display.println("Eth failed");
		HAL.Display.print("> Use WiFi");
		HAL.Display.giveSemaphoreMutex();
		dt = DisplayTiming::IGNORE_DEFAULT_PROCESS;
		break;
	}
	case DisplayTiming::NTP_INIT_BEGIN:

		HAL.Display.takeSemaphoreMutex(portMAX_DELAY); // Take mutex semaphore
		HAL.Display.fillRect(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET,
		                     smrp.DISPLAY_WIDTH, smrp.MSGBOX_HEIGHT + smrp.BOTTOM_OFFSET, TFT_BLACK);

		HAL.Display.setCursor(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET);
		HAL.Display.println("Initialize");
		HAL.Display.print("NTP");
		HAL.Display.giveSemaphoreMutex(); // Give (release) mutex semaphore

		dt = DisplayTiming::IGNORE_DEFAULT_PROCESS;
		break;
	case DisplayTiming::NTP_CONNECT_WIFI:

		HAL.Display.takeSemaphoreMutex(portMAX_DELAY); // Take mutex semaphore
		HAL.Display.fillRect(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET,
		                     smrp.DISPLAY_WIDTH, smrp.MSGBOX_HEIGHT + smrp.BOTTOM_OFFSET, TFT_BLACK);

		HAL.Display.setCursor(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET);
		HAL.Display.println("Try WiFi");
		HAL.Display.print("for NTP");
		HAL.Display.giveSemaphoreMutex(); // Give (release) mutex semaphore

		dt = DisplayTiming::IGNORE_DEFAULT_PROCESS;
		break;
	case DisplayTiming::NTP_SYNC_START:

		HAL.Display.takeSemaphoreMutex(portMAX_DELAY); // Take mutex semaphore
		HAL.Display.fillRect(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET,
		                     smrp.DISPLAY_WIDTH, smrp.MSGBOX_HEIGHT + smrp.BOTTOM_OFFSET, TFT_BLACK);

		HAL.Display.setCursor(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET);
		HAL.Display.println("Sync time");
		HAL.Display.print("to NTP");
		HAL.Display.giveSemaphoreMutex(); // Give (release) mutex semaphore

		dt = DisplayTiming::IGNORE_DEFAULT_PROCESS;
		break;
	case DisplayTiming::NTP_SYNC_FAILED:

		HAL.Display.takeSemaphoreMutex(portMAX_DELAY); // Take mutex semaphore
		HAL.Display.fillRect(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET,
		                     smrp.DISPLAY_WIDTH, smrp.MSGBOX_HEIGHT + smrp.BOTTOM_OFFSET, TFT_BLACK);

		HAL.Display.setCursor(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET);
		HAL.Display.println("Error NTP");
		HAL.Display.print("Reboot now");
		HAL.Display.giveSemaphoreMutex(); // Give (release) mutex semaphore

		dt = DisplayTiming::IGNORE_DEFAULT_PROCESS;
		break;
	case DisplayTiming::HEARTBEAT_FAILED:

		HAL.Display.takeSemaphoreMutex(portMAX_DELAY); // Take mutex semaphore
		HAL.Display.fillRect(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET,
		                     smrp.DISPLAY_WIDTH, smrp.MSGBOX_HEIGHT + smrp.BOTTOM_OFFSET, TFT_BLACK);

		HAL.Display.setCursor(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET);
		HAL.Display.println("Error NTP");
		HAL.Display.print("Reboot now");
		HAL.Display.giveSemaphoreMutex(); // Give (release) mutex semaphore

		dt = DisplayTiming::IGNORE_DEFAULT_PROCESS;
		break;
	case DisplayTiming::AMBIENT_INIT_BEGIN:

		HAL.Display.takeSemaphoreMutex(portMAX_DELAY); // Take mutex semaphore
		HAL.Display.fillRect(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET,
		                     smrp.DISPLAY_WIDTH, smrp.MSGBOX_HEIGHT + smrp.BOTTOM_OFFSET, TFT_BLACK);

		HAL.Display.setCursor(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET);
		HAL.Display.println("Initialize");
		HAL.Display.print("Ambient");
		HAL.Display.giveSemaphoreMutex(); // Give (release) mutex semaphore

		dt = DisplayTiming::IGNORE_DEFAULT_PROCESS;
		break;
	case DisplayTiming::AMBIENT_CONNECT_WIFI:

		HAL.Display.takeSemaphoreMutex(portMAX_DELAY); // Take mutex semaphore
		HAL.Display.fillRect(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET,
		                     smrp.DISPLAY_WIDTH, smrp.MSGBOX_HEIGHT + smrp.BOTTOM_OFFSET, TFT_BLACK);

		HAL.Display.setCursor(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET);
		HAL.Display.println("Try WiFi");
		HAL.Display.print("> Ambient");
		HAL.Display.giveSemaphoreMutex(); // Give (release) mutex semaphore

		dt = DisplayTiming::IGNORE_DEFAULT_PROCESS;
		break;
	case DisplayTiming::FTP_INIT_BEGIN:

		HAL.Display.takeSemaphoreMutex(portMAX_DELAY); // Take mutex semaphore
		HAL.Display.fillRect(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET,
		                     smrp.DISPLAY_WIDTH, smrp.MSGBOX_HEIGHT + smrp.BOTTOM_OFFSET, TFT_BLACK);

		HAL.Display.setCursor(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET);
		HAL.Display.println("Initialize");
		HAL.Display.print("FTP");
		HAL.Display.giveSemaphoreMutex(); // Give (release) mutex semaphore

		dt = DisplayTiming::IGNORE_DEFAULT_PROCESS;
		break;
	case DisplayTiming::FTP_CONNECT_WIFI:

		HAL.Display.takeSemaphoreMutex(portMAX_DELAY); // Take mutex semaphore
		HAL.Display.fillRect(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET,
		                     smrp.DISPLAY_WIDTH, smrp.MSGBOX_HEIGHT + smrp.BOTTOM_OFFSET, TFT_BLACK);

		HAL.Display.setCursor(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET);
		HAL.Display.println("Try WiFi");
		HAL.Display.print("> FTP");
		HAL.Display.giveSemaphoreMutex(); // Give (release) mutex semaphore

		dt = DisplayTiming::IGNORE_DEFAULT_PROCESS;
		break;
	case DisplayTiming::HTTP_INIT_BEGIN:

		HAL.Display.takeSemaphoreMutex(portMAX_DELAY); // Take mutex semaphore
		HAL.Display.fillRect(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET,
		                     smrp.DISPLAY_WIDTH, smrp.MSGBOX_HEIGHT + smrp.BOTTOM_OFFSET, TFT_BLACK);

		HAL.Display.setCursor(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET);
		HAL.Display.println("Initialize");
		HAL.Display.print("HTTP");
		HAL.Display.giveSemaphoreMutex(); // Give (release) mutex semaphore

		dt = DisplayTiming::IGNORE_DEFAULT_PROCESS;
		break;
	case DisplayTiming::HTTP_CONNECT_WIFI:

		HAL.Display.takeSemaphoreMutex(portMAX_DELAY); // Take mutex semaphore
		HAL.Display.fillRect(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET,
		                     smrp.DISPLAY_WIDTH, smrp.MSGBOX_HEIGHT + smrp.BOTTOM_OFFSET, TFT_BLACK);

		HAL.Display.setCursor(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET);
		HAL.Display.println("Try WiFi");
		HAL.Display.print("> HTTP");
		HAL.Display.giveSemaphoreMutex(); // Give (release) mutex semaphore

		dt = DisplayTiming::IGNORE_DEFAULT_PROCESS;
		break;
	case DisplayTiming::MQTT_INIT_BEGIN:

		HAL.Display.takeSemaphoreMutex(portMAX_DELAY); // Take mutex semaphore
		HAL.Display.fillRect(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET,
		                     smrp.DISPLAY_WIDTH, smrp.MSGBOX_HEIGHT + smrp.BOTTOM_OFFSET, TFT_BLACK);

		HAL.Display.setCursor(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET);
		HAL.Display.println("Initialize");
		HAL.Display.print("MQTT");
		HAL.Display.giveSemaphoreMutex(); // Give (release) mutex semaphore

		dt = DisplayTiming::IGNORE_DEFAULT_PROCESS;
		break;
	case DisplayTiming::MQTT_CONNECT_WIFI:

		HAL.Display.takeSemaphoreMutex(portMAX_DELAY); // Take mutex semaphore
		HAL.Display.fillRect(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET,
		                     smrp.DISPLAY_WIDTH, smrp.MSGBOX_HEIGHT + smrp.BOTTOM_OFFSET, TFT_BLACK);

		HAL.Display.setCursor(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET);
		HAL.Display.println("Try WiFi");
		HAL.Display.print("> MQTT");
		HAL.Display.giveSemaphoreMutex(); // Give (release) mutex semaphore

		dt = DisplayTiming::IGNORE_DEFAULT_PROCESS;
		break;
	case DisplayTiming::LORA_WAN_INIT_BEGIN: {

		HAL.Display.takeSemaphoreMutex(portMAX_DELAY); // Take mutex semaphore
		HAL.Display.fillRect(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET,
		                     smrp.DISPLAY_WIDTH, smrp.MSGBOX_HEIGHT + smrp.BOTTOM_OFFSET, TFT_BLACK);

		HAL.Display.setCursor(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET);
		HAL.Display.println("Initialize");
		HAL.Display.print("LoRaWAN");
		HAL.Display.giveSemaphoreMutex(); // Give (release) mutex semaphore

		dt = DisplayTiming::IGNORE_DEFAULT_PROCESS;
		break;
	}
	case DisplayTiming::LORA_WAN_JOIN_BEGIN: {
		HAL.Display.takeSemaphoreMutex(portMAX_DELAY); // Take mutex semaphore
		HAL.Display.fillRect(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET,
		                     smrp.DISPLAY_WIDTH, smrp.MSGBOX_HEIGHT + smrp.BOTTOM_OFFSET, TFT_BLACK);

		HAL.Display.setCursor(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET);
		HAL.Display.println("Try Join");
		HAL.Display.print("LoRaWAN");
		HAL.Display.giveSemaphoreMutex(); // Give (release) mutex semaphore

		dt = DisplayTiming::IGNORE_DEFAULT_PROCESS;
		break;
	}
	case DisplayTiming::LORA_WAN_INIT_FAILED: {
		const char *err_msg = va_arg(ap, const char *);

		HAL.Display.takeSemaphoreMutex(portMAX_DELAY); // Take mutex semaphore
		HAL.Display.fillRect(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET,
		                     smrp.DISPLAY_WIDTH, smrp.MSGBOX_HEIGHT + smrp.BOTTOM_OFFSET, TFT_BLACK);

		HAL.Display.setCursor(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET);
		HAL.Display.println("LoRa Error");
		HAL.Display.printf("%s", err_msg);
		HAL.Display.giveSemaphoreMutex(); // Give (release) mutex semaphore

		dt = DisplayTiming::IGNORE_DEFAULT_PROCESS;
		break;
	}
	case DisplayTiming::TI_INIT_BEGIN:

		HAL.Display.takeSemaphoreMutex(portMAX_DELAY); // Take mutex semaphore
		HAL.Display.fillRect(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET,
		                     smrp.DISPLAY_WIDTH, smrp.MSGBOX_HEIGHT + smrp.BOTTOM_OFFSET, TFT_BLACK);

		HAL.Display.setCursor(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET);
		HAL.Display.println("Initialize");
		HAL.Display.print("Timer");
		HAL.Display.giveSemaphoreMutex(); // Give (release) mutex semaphore

		dt = DisplayTiming::IGNORE_DEFAULT_PROCESS;
		break;
	case DisplayTiming::DISPLAY_OFFLINE_MODE:

		HAL.Display.takeSemaphoreMutex(portMAX_DELAY); // Take mutex semaphore
		HAL.Display.fillRect(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET,
		                     smrp.DISPLAY_WIDTH, smrp.MSGBOX_HEIGHT + smrp.BOTTOM_OFFSET, TFT_BLACK);

		HAL.Display.setCursor(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET);
		HAL.Display.println("Offline");
		HAL.Display.print("Mode");
		HAL.Display.giveSemaphoreMutex(); // Give (release) mutex semaphore

		dt = DisplayTiming::IGNORE_DEFAULT_PROCESS;
		break;
	case DisplayTiming::SYSTEM_INIT_DONE: {
		const char *dev_id = va_arg(ap, const char *);
		int sensor_num     = va_arg(ap, int);

		HAL.Display.takeSemaphoreMutex(portMAX_DELAY); // Take mutex semaphore
		HAL.Display.fillRect(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET,
		                     smrp.DISPLAY_WIDTH, smrp.MSGBOX_HEIGHT + smrp.BOTTOM_OFFSET, TFT_BLACK);

		HAL.Display.setCursor(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET);
		HAL.Display.println("System");
		HAL.Display.print("Ready!");

		HAL.Display.giveSemaphoreMutex();
		if (not HAL.isWakeFromSleep()) {
			HAL.delay(3000);
		}
		HAL.Display.takeSemaphoreMutex(portMAX_DELAY);

		HAL.Display.fillScreen(TFT_BLACK);

		// Apply user display design
		HAL.Display.giveSemaphoreMutex();
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
	if (HAL.isWakeFromSleep()) {
		return;
	}

	if (first_dt == DisplayTiming::SYSTEM_INIT_BEGIN) {
		/* Rendering Title */
		HAL.Display.takeSemaphoreMutex(portMAX_DELAY);
		HAL.Display.setCursor(0, 0);
		HAL.Display.printf("YRIT\nIoT Device");
		// render->setFontColor(TFT_GREEN);
		// render->cprintf("- M5Unit Edition -");
		// render->setFontColor(TFT_WHITE);
		HAL.Display.giveSemaphoreMutex();

		PlayLoadAnimation          = true;
		const uint8_t TOP_PRIORITY = 24;
		const int STACK_SIZE       = 2048;
		xTaskCreate(loadAnimationLoop, "LoadAnimation", STACK_SIZE, NULL, TOP_PRIORITY, &LoadAnimationTask);
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

#endif /* TARGET__M5ATOM_S3 */