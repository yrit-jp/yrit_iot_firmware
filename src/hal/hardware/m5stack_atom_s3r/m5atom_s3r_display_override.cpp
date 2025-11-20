/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/
//
//  MARK: Display Override
//
/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/

#include "../../hal.h"

#ifdef TARGET__M5ATOM_S3R

#include "../../../display/display.h"
#include "../../../display/display_timing_type_def.h"
#include "../../../user/user_display_design.h"
#include <math.h> // for sin, cons
#define deg_to_rad(deg) (((deg) / 360.0) * 2.0 * M_PI)

extern struct SystemMessageRenderParam smrp;

volatile bool PlayLoadAnimation = false;
TaskHandle_t LoadAnimationTask;
void loadAnimationLoop(void *pvParameters) {
	const uint8_t DOTS_SPACE_PX = 20;
	const uint8_t DOTS          = 5;
	const uint8_t PER_ANGLE     = 90 / DOTS;
	uint16_t color[DOTS]        = {0};

	uint16_t left_x = (HAL.Display.width() / 2) - (DOTS_SPACE_PX * 2); // Circle center X
	uint16_t left_y = 70;                                              // Circle center Y
	int16_t i       = 0;                                               // Degree
	int8_t up       = 2;                                               // Round speed
	uint8_t cr      = 3;                                               // Sub circle radius

	while (PlayLoadAnimation) {
		HAL.Display.takeSemaphoreMutex(portMAX_DELAY); // Take mutex semaphore
		for (int k = 0; k < DOTS; k++) {
			uint8_t rgb = abs(sin(deg_to_rad(k * PER_ANGLE + i))) * 255;
			color[k]    = HAL.Display.color565(rgb, rgb / 2, 10);
			HAL.Display.fillEllipse(left_x + k * DOTS_SPACE_PX, left_y, cr, cr, color[k]);
			// cout << abs(sin(deg_to_rad(k * 45 + i))) << endl;
		}
		HAL.Display.giveSemaphoreMutex(); // Give (release) mutex semaphore

		HAL.update();
		HAL.delay(15 + 5 * (1 - sin(deg_to_rad(i))));
		i = ((i - up) <= 0) ? 180 : (i - up);
		HAL.update();
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
		smrp.FONT_SIZE      = 12;
		smrp.BOTTOM_OFFSET  = 15;
		smrp.LEFT_OFFSET    = 0;
		smrp.MSGBOX_HEIGHT  = 30;
		smrp.MSGBOX_WIDTH   = smrp.DISPLAY_WIDTH;

		HAL.Display.giveSemaphoreMutex();
		break;
	case DisplayTiming::SS_INIT_BEGIN: {
		uint8_t wait_sec = va_arg(ap, int);

		HAL.Display.takeSemaphoreMutex(portMAX_DELAY); // Take mutex semaphore
		HAL.Display.fillRect(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET,
		                     smrp.DISPLAY_WIDTH, smrp.MSGBOX_HEIGHT + smrp.BOTTOM_OFFSET, TFT_BLACK);
		render->setAlignment(Align::TopLeft);
		render->setFontSize(smrp.FONT_SIZE);
		render->setCursor(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT);
		render->printf("ボタンを押すと\n");
		render->setFontColor(TFT_YELLOW);
		render->printf("設定モードで起動 （%d秒）\n", wait_sec);
		render->setFontColor(TFT_WHITE);
		HAL.Display.giveSemaphoreMutex(); // Give (release) mutex semaphore

		dt = DisplayTiming::IGNORE_DEFAULT_PROCESS;
		break;
	}
	case DisplayTiming::SS_MODE_ENTRY:

		HAL.Display.takeSemaphoreMutex(portMAX_DELAY); // Take mutex semaphore
		HAL.Display.fillRect(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET,
		                     smrp.DISPLAY_WIDTH, smrp.MSGBOX_HEIGHT + smrp.BOTTOM_OFFSET, TFT_BLACK);
		render->setAlignment(Align::TopLeft);
		render->setFontSize(smrp.FONT_SIZE);
		render->setCursor(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT);
		render->printf("ボタンを押すと\n");
		render->setFontColor(TFT_RED);
		render->printf("パラメータを初期化");
		render->setFontColor(TFT_WHITE);
		HAL.Display.giveSemaphoreMutex(); // Give (release) mutex semaphore

		dt = DisplayTiming::IGNORE_DEFAULT_PROCESS;
		break;
	case DisplayTiming::SS_CLEAR_ALL:

		HAL.Display.takeSemaphoreMutex(portMAX_DELAY); // Take mutex semaphore
		HAL.Display.fillRect(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET,
		                     smrp.DISPLAY_WIDTH, smrp.MSGBOX_HEIGHT + smrp.BOTTOM_OFFSET, TFT_BLACK);
		render->setFontSize(smrp.FONT_SIZE);
		render->setCursor(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT);
		render->printf("ファイルシステムの\n初期化中 ...");
		HAL.Display.giveSemaphoreMutex(); // Give (release) mutex semaphore

		dt = DisplayTiming::IGNORE_DEFAULT_PROCESS;
		break;
	case DisplayTiming::SS_FORMAT:

		HAL.Display.takeSemaphoreMutex(portMAX_DELAY); // Take mutex semaphore
		HAL.Display.fillRect(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET,
		                     smrp.DISPLAY_WIDTH, smrp.MSGBOX_HEIGHT + smrp.BOTTOM_OFFSET, TFT_BLACK);
		render->setFontSize(smrp.FONT_SIZE);
		render->setCursor(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT);
		render->printf("ファイルシステムの\n準備中 ...");
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
		HAL.Display.fillRect(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET,
		                     smrp.DISPLAY_WIDTH, smrp.MSGBOX_HEIGHT + smrp.BOTTOM_OFFSET, TFT_BLACK);
		render->setFontSize(smrp.FONT_SIZE);
		render->setCursor(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT);
		render->printf("Bluetooth LE 接続名\n");
		render->printf("%s", HAL.getChipUUID().c_str());
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
		render->setFontSize(smrp.FONT_SIZE);
		for (uint8_t c = hold_sec; c != 0; c--) {
			render->setCursor(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT);
			render->printf("ID: %s\nFWv: %d.%d ", dev_id, main_version, miner_version);

			render->setFontColor(TFT_YELLOW);
			render->printf("%.6s", release_tag);
			render->setFontColor(TFT_WHITE);

			render->printf(" [");
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
		HAL.Display.fillRect(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET,
		                     smrp.DISPLAY_WIDTH, smrp.MSGBOX_HEIGHT + smrp.BOTTOM_OFFSET, TFT_BLACK);
		HAL.Display.giveSemaphoreMutex(); // Give (release) mutex semaphore

		dt = DisplayTiming::IGNORE_DEFAULT_PROCESS;
		break;
	}
	case DisplayTiming::GPIO_INIT_BEGIN:

		HAL.Display.takeSemaphoreMutex(portMAX_DELAY); // Take mutex semaphore
		HAL.Display.fillRect(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET,
		                     smrp.DISPLAY_WIDTH, smrp.MSGBOX_HEIGHT + smrp.BOTTOM_OFFSET, TFT_BLACK);
		render->setFontSize(smrp.FONT_SIZE);
		render->setCursor(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT);
		render->printf("GPIOデバイス\n初期化準備中 ...");
		HAL.Display.giveSemaphoreMutex(); // Give (release) mutex semaphore

		dt = DisplayTiming::IGNORE_DEFAULT_PROCESS;
		break;
	case DisplayTiming::PULSE_COUNTER_INIT_BEGIN:

		HAL.Display.takeSemaphoreMutex(portMAX_DELAY); // Take mutex semaphore
		HAL.Display.fillRect(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET,
		                     smrp.DISPLAY_WIDTH, smrp.MSGBOX_HEIGHT + smrp.BOTTOM_OFFSET, TFT_BLACK);
		render->setFontSize(smrp.FONT_SIZE);
		render->setCursor(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT);
		render->printf("パルスカウンタ\n初期化準備中 ...");
		HAL.Display.giveSemaphoreMutex(); // Give (release) mutex semaphore

		dt = DisplayTiming::IGNORE_DEFAULT_PROCESS;
		break;
	case DisplayTiming::PULSE_COUNTER_INIT_TARGET: {
		uint8_t pin = va_arg(ap, int);

		HAL.Display.takeSemaphoreMutex(portMAX_DELAY); // Take mutex semaphore
		HAL.Display.fillRect(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET,
		                     smrp.DISPLAY_WIDTH, smrp.MSGBOX_HEIGHT + smrp.BOTTOM_OFFSET, TFT_BLACK);
		render->setFontSize(smrp.FONT_SIZE);
		render->setCursor(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT);
		render->printf("ピン %d の\n初期化中", pin);
		HAL.Display.giveSemaphoreMutex(); // Give (release) mutex semaphore

		dt = DisplayTiming::IGNORE_DEFAULT_PROCESS;
		break;
	}
	case DisplayTiming::I2C_INIT_DEVICE_BEGIN:

		HAL.Display.takeSemaphoreMutex(portMAX_DELAY); // Take mutex semaphore
		HAL.Display.fillRect(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET,
		                     smrp.DISPLAY_WIDTH, smrp.MSGBOX_HEIGHT + smrp.BOTTOM_OFFSET, TFT_BLACK);
		render->setFontSize(smrp.FONT_SIZE);
		render->setCursor(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT);
		render->printf("I2Cデバイス\n初期化準備中 ...");
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
		render->setFontSize(smrp.FONT_SIZE);
		render->setCursor(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET);
		render->printf("（%d/%d）\nI2Cデバイス（0x%02X）の\n初期化中", progress, i2c_dev_num, address);
		HAL.Display.giveSemaphoreMutex(); // Give (release) mutex semaphore

		dt = DisplayTiming::IGNORE_DEFAULT_PROCESS;
		break;
	}
	case DisplayTiming::I2C_INIT_DEVICE_FAILED: {
		uint8_t address = va_arg(ap, int);

		HAL.Display.takeSemaphoreMutex(portMAX_DELAY); // Take mutex semaphore
		HAL.Display.fillRect(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET,
		                     smrp.DISPLAY_WIDTH, smrp.MSGBOX_HEIGHT + smrp.BOTTOM_OFFSET, TFT_BLACK);
		render->setFontSize(smrp.FONT_SIZE);
		render->setCursor(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET);
		render->setFontColor(TFT_RED);
		render->printf("I2Cデバイス（0x%02X）\nの初期化に失敗しました\n10秒後に再起動します...", address);
		HAL.Display.giveSemaphoreMutex(); // Give (release) mutex semaphore

		dt = DisplayTiming::IGNORE_DEFAULT_PROCESS;
		break;
	}
	case DisplayTiming::MODBUS_INIT_PERIPHERAL_BEGIN: {

		HAL.Display.takeSemaphoreMutex(portMAX_DELAY); // Take mutex semaphore
		HAL.Display.fillRect(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET,
		                     smrp.DISPLAY_WIDTH, smrp.MSGBOX_HEIGHT + smrp.BOTTOM_OFFSET, TFT_BLACK);
		render->setFontSize(smrp.FONT_SIZE);
		render->setCursor(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT);
		render->printf("Modbus 初期化準備中 ...");
		HAL.Display.giveSemaphoreMutex(); // Give (release) mutex semaphore

		dt = DisplayTiming::IGNORE_DEFAULT_PROCESS;
		break;
	}
	case DisplayTiming::MODBUS_INIT_PERIPHERAL_TARGET: {
		uint8_t port = va_arg(ap, int);

		HAL.Display.takeSemaphoreMutex(portMAX_DELAY); // Take mutex semaphore
		HAL.Display.fillRect(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET,
		                     smrp.DISPLAY_WIDTH, smrp.MSGBOX_HEIGHT + smrp.BOTTOM_OFFSET, TFT_BLACK);
		render->setFontSize(smrp.FONT_SIZE);
		render->setCursor(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT);
		render->printf("ポート %d\n初期化中 ...", port);
		HAL.Display.giveSemaphoreMutex(); // Give (release) mutex semaphore

		dt = DisplayTiming::IGNORE_DEFAULT_PROCESS;
		break;
	}
	case DisplayTiming::MODBUS_INIT_DEVICE_BEGIN: {

		HAL.Display.takeSemaphoreMutex(portMAX_DELAY); // Take mutex semaphore
		HAL.Display.fillRect(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET,
		                     smrp.DISPLAY_WIDTH, smrp.MSGBOX_HEIGHT + smrp.BOTTOM_OFFSET, TFT_BLACK);
		render->setFontSize(smrp.FONT_SIZE);
		render->setCursor(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT);
		render->printf("Modbusデバイス\n初期化準備中 ...");
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
		render->setFontSize(smrp.FONT_SIZE);
		render->setCursor(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET);
		render->printf("（%d/%d）ID:%d / ポート:%d\nModbus デバイスの\n初期化中", progress, modbus_dev_num, id, port);
		HAL.Display.giveSemaphoreMutex(); // Give (release) mutex semaphore

		dt = DisplayTiming::IGNORE_DEFAULT_PROCESS;
		break;
	}
	case DisplayTiming::WIFI_TRY_CONNECT: {
		const char *target_ssid = va_arg(ap, const char *);

		HAL.Display.takeSemaphoreMutex(portMAX_DELAY); // Take mutex semaphore
		HAL.Display.fillRect(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET,
		                     smrp.DISPLAY_WIDTH, smrp.MSGBOX_HEIGHT + smrp.BOTTOM_OFFSET, TFT_BLACK);
		render->setFontSize(smrp.FONT_SIZE);
		render->setCursor(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT);
		render->printf("WiFi 接続中\n(%.16s)", target_ssid);
		HAL.Display.giveSemaphoreMutex(); // Give (release) mutex semaphore

		dt = DisplayTiming::IGNORE_DEFAULT_PROCESS;
		break;
	}
	case DisplayTiming::WIFI_SEARCH_TARGET:

		HAL.Display.takeSemaphoreMutex(portMAX_DELAY); // Take mutex semaphore
		HAL.Display.fillRect(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET,
		                     smrp.DISPLAY_WIDTH, smrp.MSGBOX_HEIGHT + smrp.BOTTOM_OFFSET, TFT_BLACK);
		render->setFontSize(smrp.FONT_SIZE);
		render->setCursor(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT);
		render->printf("SSID 検索中 ...");
		HAL.Display.giveSemaphoreMutex(); // Give (release) mutex semaphore

		dt = DisplayTiming::IGNORE_DEFAULT_PROCESS;
		break;
	case DisplayTiming::WIFI_CONNECT_SUCCESS:

		HAL.Display.takeSemaphoreMutex(portMAX_DELAY); // Take mutex semaphore
		HAL.Display.fillRect(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET,
		                     smrp.DISPLAY_WIDTH, smrp.MSGBOX_HEIGHT + smrp.BOTTOM_OFFSET, TFT_BLACK);
		render->setFontSize(smrp.FONT_SIZE);
		render->setCursor(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT);
		render->printf("WiFi の接続完了");
		HAL.Display.giveSemaphoreMutex(); // Give (release) mutex semaphore

		dt = DisplayTiming::IGNORE_DEFAULT_PROCESS;
		break;
	case DisplayTiming::WIFI_CONNECT_FAILED:

		HAL.Display.takeSemaphoreMutex(portMAX_DELAY); // Take mutex semaphore
		HAL.Display.fillRect(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET,
		                     smrp.DISPLAY_WIDTH, smrp.MSGBOX_HEIGHT + smrp.BOTTOM_OFFSET, TFT_BLACK);
		render->setFontSize(smrp.FONT_SIZE);
		render->setCursor(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT);
		render->setFontColor(TFT_RED);
		render->printf("WiFi の接続に失敗\n10秒後に再起動します...");
		HAL.Display.giveSemaphoreMutex(); // Give (release) mutex semaphore

		dt = DisplayTiming::IGNORE_DEFAULT_PROCESS;
		break;
	case DisplayTiming::WIFI_TARGET_DETECTED: {
		const char *target_ssid = va_arg(ap, const char *);

		HAL.Display.takeSemaphoreMutex(portMAX_DELAY); // Take mutex semaphore
		HAL.Display.fillRect(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET,
		                     smrp.DISPLAY_WIDTH, smrp.MSGBOX_HEIGHT + smrp.BOTTOM_OFFSET, TFT_BLACK);
		render->setFontSize(smrp.FONT_SIZE);
		render->setCursor(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT);
		render->printf("SSID を発見しました\n (%.16s)", target_ssid);
		HAL.Display.giveSemaphoreMutex(); // Give (release) mutex semaphore

		dt = DisplayTiming::IGNORE_DEFAULT_PROCESS;
		break;
	}
	case DisplayTiming::WIFI_TARGET_NOT_FOUND: {
		const char *target_ssid = va_arg(ap, const char *);

		HAL.Display.takeSemaphoreMutex(portMAX_DELAY); // Take mutex semaphore
		HAL.Display.fillRect(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET,
		                     smrp.DISPLAY_WIDTH, smrp.MSGBOX_HEIGHT + smrp.BOTTOM_OFFSET, TFT_BLACK);
		render->setFontSize(smrp.FONT_SIZE);
		render->setCursor(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET);
		render->setFontColor(TFT_YELLOW);
		render->printf("SSID が\n見つかりませんでした\n(%.16s)", target_ssid);
		render->setFontColor(TFT_WHITE);
		HAL.Display.giveSemaphoreMutex(); // Give (release) mutex semaphore

		dt = DisplayTiming::IGNORE_DEFAULT_PROCESS;
		break;
	}
	case DisplayTiming::NTP_INIT_BEGIN:

		HAL.Display.takeSemaphoreMutex(portMAX_DELAY); // Take mutex semaphore
		HAL.Display.fillRect(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET,
		                     smrp.DISPLAY_WIDTH, smrp.MSGBOX_HEIGHT + smrp.BOTTOM_OFFSET, TFT_BLACK);
		render->setFontSize(smrp.FONT_SIZE);
		render->setCursor(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT);
		render->printf("時刻同期\n初期化準備中 ...");
		HAL.Display.giveSemaphoreMutex(); // Give (release) mutex semaphore

		dt = DisplayTiming::IGNORE_DEFAULT_PROCESS;
		break;
	case DisplayTiming::NTP_CONNECT_WIFI:

		HAL.Display.takeSemaphoreMutex(portMAX_DELAY); // Take mutex semaphore
		HAL.Display.fillRect(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET,
		                     smrp.DISPLAY_WIDTH, smrp.MSGBOX_HEIGHT + smrp.BOTTOM_OFFSET, TFT_BLACK);
		render->setFontSize(smrp.FONT_SIZE);
		render->setCursor(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT);
		render->printf("WiFi 接続\n（NTP） ...");
		HAL.Display.giveSemaphoreMutex(); // Give (release) mutex semaphore

		dt = DisplayTiming::IGNORE_DEFAULT_PROCESS;
		break;
	case DisplayTiming::NTP_SYNC_START:

		HAL.Display.takeSemaphoreMutex(portMAX_DELAY); // Take mutex semaphore
		HAL.Display.fillRect(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET,
		                     smrp.DISPLAY_WIDTH, smrp.MSGBOX_HEIGHT + smrp.BOTTOM_OFFSET, TFT_BLACK);
		render->setFontSize(smrp.FONT_SIZE);
		render->setCursor(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT);
		render->printf("NTP サーバと\n時刻を同期中 ...");
		HAL.Display.giveSemaphoreMutex(); // Give (release) mutex semaphore

		dt = DisplayTiming::IGNORE_DEFAULT_PROCESS;
		break;
	case DisplayTiming::NTP_SYNC_FAILED:

		HAL.Display.takeSemaphoreMutex(portMAX_DELAY); // Take mutex semaphore
		HAL.Display.fillRect(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET,
		                     smrp.DISPLAY_WIDTH, smrp.MSGBOX_HEIGHT + smrp.BOTTOM_OFFSET, TFT_BLACK);
		render->setFontSize(smrp.FONT_SIZE);
		render->setCursor(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT);
		render->setFontColor(TFT_RED);
		render->printf("時刻の同期に失敗\n10秒後に再起動します...");
		HAL.Display.giveSemaphoreMutex(); // Give (release) mutex semaphore

		dt = DisplayTiming::IGNORE_DEFAULT_PROCESS;
		break;
	case DisplayTiming::HEARTBEAT_FAILED:

		HAL.Display.takeSemaphoreMutex(portMAX_DELAY); // Take mutex semaphore
		HAL.Display.fillRect(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET,
		                     smrp.DISPLAY_WIDTH, smrp.MSGBOX_HEIGHT + smrp.BOTTOM_OFFSET, TFT_BLACK);
		render->setFontSize(smrp.FONT_SIZE);
		render->setCursor(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT);
		render->setFontColor(TFT_RED);
		render->printf("時刻の取得に失敗\n5秒後に再起動します...");
		HAL.Display.giveSemaphoreMutex(); // Give (release) mutex semaphore

		dt = DisplayTiming::IGNORE_DEFAULT_PROCESS;
		break;
	case DisplayTiming::AMBIENT_CONNECT_WIFI:

		HAL.Display.takeSemaphoreMutex(portMAX_DELAY); // Take mutex semaphore
		HAL.Display.fillRect(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET,
		                     smrp.DISPLAY_WIDTH, smrp.MSGBOX_HEIGHT + smrp.BOTTOM_OFFSET, TFT_BLACK);
		render->setFontSize(smrp.FONT_SIZE);
		render->setCursor(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT);
		render->printf("WiFi 接続中\n（Ambient） ...");
		HAL.Display.giveSemaphoreMutex(); // Give (release) mutex semaphore

		dt = DisplayTiming::IGNORE_DEFAULT_PROCESS;
		break;
	case DisplayTiming::FTP_CONNECT_WIFI:

		HAL.Display.takeSemaphoreMutex(portMAX_DELAY); // Take mutex semaphore
		HAL.Display.fillRect(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET,
		                     smrp.DISPLAY_WIDTH, smrp.MSGBOX_HEIGHT + smrp.BOTTOM_OFFSET, TFT_BLACK);
		render->setFontSize(smrp.FONT_SIZE);
		render->setCursor(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT);
		render->printf("WiFi 接続中\n（FTP） ...");
		HAL.Display.giveSemaphoreMutex(); // Give (release) mutex semaphore

		dt = DisplayTiming::IGNORE_DEFAULT_PROCESS;
		break;
	case DisplayTiming::HTTP_CONNECT_WIFI:

		HAL.Display.takeSemaphoreMutex(portMAX_DELAY); // Take mutex semaphore
		HAL.Display.fillRect(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET,
		                     smrp.DISPLAY_WIDTH, smrp.MSGBOX_HEIGHT + smrp.BOTTOM_OFFSET, TFT_BLACK);
		render->setFontSize(smrp.FONT_SIZE);
		render->setCursor(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT);
		render->printf("WiFi 接続中（HTTP） ...");
		HAL.Display.giveSemaphoreMutex(); // Give (release) mutex semaphore

		dt = DisplayTiming::IGNORE_DEFAULT_PROCESS;
		break;
	case DisplayTiming::MQTT_CONNECT_WIFI:

		HAL.Display.takeSemaphoreMutex(portMAX_DELAY); // Take mutex semaphore
		HAL.Display.fillRect(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET,
		                     smrp.DISPLAY_WIDTH, smrp.MSGBOX_HEIGHT + smrp.BOTTOM_OFFSET, TFT_BLACK);
		render->setFontSize(smrp.FONT_SIZE);
		render->setCursor(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT);
		render->printf("WiFi 接続中（MQTT） ...");
		HAL.Display.giveSemaphoreMutex(); // Give (release) mutex semaphore

		dt = DisplayTiming::IGNORE_DEFAULT_PROCESS;
		break;
	case DisplayTiming::DISPLAY_OFFLINE_MODE:

		HAL.Display.takeSemaphoreMutex(portMAX_DELAY); // Take mutex semaphore
		HAL.Display.fillRect(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET,
		                     smrp.DISPLAY_WIDTH, smrp.MSGBOX_HEIGHT + smrp.BOTTOM_OFFSET, TFT_BLACK);
		render->setFontSize(smrp.FONT_SIZE);
		render->setCursor(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT);
		render->printf("OFFLINE モードで\n起動中 ...");
		HAL.Display.giveSemaphoreMutex(); // Give (release) mutex semaphore

		dt = DisplayTiming::IGNORE_DEFAULT_PROCESS;
		break;
	case DisplayTiming::SYSTEM_INIT_DONE: {
		const char *dev_id = va_arg(ap, const char *);
		int sensor_num     = va_arg(ap, int);

		HAL.Display.takeSemaphoreMutex(portMAX_DELAY); // Take mutex semaphore
		HAL.Display.fillRect(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET,
		                     smrp.DISPLAY_WIDTH, smrp.MSGBOX_HEIGHT + smrp.BOTTOM_OFFSET, TFT_BLACK);
		render->setFontSize(smrp.FONT_SIZE);
		render->setCursor(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT);
		render->setFontColor(TFT_GREENYELLOW);
		render->printf("システムを起動する\n準備が完了しました!");
		render->setFontColor(TFT_WHITE);

		HAL.Display.giveSemaphoreMutex();
		HAL.delay(3000);
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

	if (first_dt == DisplayTiming::SYSTEM_INIT_BEGIN) {
		/* Rendering Title */
		HAL.Display.takeSemaphoreMutex(portMAX_DELAY);
		render->setAlignment(Align::TopCenter);
		render->setFontSize(20);
		render->setCursor((smrp.DISPLAY_WIDTH) / 2, 0);
		render->setFontColor(TFT_WHITE);
		render->cprintf("YRIT\nIoT Device\n");

		render->setCursor(smrp.DISPLAY_WIDTH / 2, 47);
		render->setFontSize(11);
		render->setFontColor(TFT_GREEN);
		render->cprintf("- M5Atom Edition -");
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

#endif /* TARGET__M5ATOM_S3R */