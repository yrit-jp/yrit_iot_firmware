#ifndef HAL_H
#define HAL_H

#define _ENABLE_ (1)
#define _DISABLE_ (0)

// clang-format off
#if defined (ESP32)
	/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/
	//  ESP32
	/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/
	#if defined (ARDUINO_M5Stack_Core_ESP32) || defined (ARDUINO_M5STACK_CORE)
		#include "hardware/m5stack_basic/hal_m5stack_basic.h"
		#define TARGET__M5STACK_BASIC
		#define DISPLAY_320X240
		#define ZIGBEE_API_MAX_FRAME_SIZE (3584) //1024 * 3 + 512
		
	#elif defined (ARDUINO_M5STACK_Core2) || defined (ARDUINO_m5stack_core2) || defined (ARDUINO_M5STACK_CORE2)
		#include "hardware/m5stack_core2/hal_m5stack_core2.h"
		#define TARGET__M5STACK_CORE2
		#define DISPLAY_320X240
	
	#elif defined (ARDUINO_M5STACK_CORES3)
		#include "hardware/m5stack_core_s3/hal_m5stack_core_s3.h"
		#define TARGET__M5STACK_CORE_S3
		#define DISPLAY_320X240
		#define HAL_MQTT_MAX_PACKET_SIZE (65534)
		#define HAL_HTTP_MAX_PACKET_SIZE (65534)

	#elif defined (ARDUINO_M5STACK_Tough) || defined (ARDUINO_M5STACK_TOUGH)
		#include "hardware/m5stack_tough/hal_m5stack_tough.h"
		#define TARGET__M5STACK_TOUGH
		#define DISPLAY_320X240
	
	#elif defined (ARDUINO_M5Stick_C_PLUS) || defined (ARDUINO_M5STACK_STICKC_PLUS)
		#include "hardware/m5stick_c_plus/hal_m5stick_c_plus.h"
		#define TARGET__M5STICK_C_PLUS
		#define DISPLAY_240X135
		#define BTN_LAYOUT_STICK_C_PLUS

	#elif defined (ARDUINO_M5Stack_Station) || defined (ARDUINO_M5STACK_STATION)
		#include "hardware/m5station/hal_m5station.h"
		#define TARGET__M5STATION
		#define DISPLAY_240X135
		#define BTN_LAYOUT_STACK_BASIC
		#define ZIGBEE_API_MAX_FRAME_SIZE (3584) //1024 * 3 + 512

	#elif defined (ARDUINO_M5Stack_ATOMS3) || defined (ARDUINO_M5STACK_ATOMS3)
		#include "hardware/m5stack_atom_s3/hal_m5stack_atom_s3.h"
		#define TARGET__M5ATOM_S3
		#define DISPLAY_128X128
	
	#elif defined (ARDUINO_M5STACK_ATOMS3R)
		#include "hardware/m5stack_atom_s3r/hal_m5stack_atom_s3r.h"
		#define TARGET__M5ATOM_S3R
		#define DISPLAY_128X128

	#elif defined (ARDUINO_M5STACK_STICKC_PLUS2)
		#include "hardware/m5stick_c_plus2/hal_m5stick_c_plus2.h"
		#define TARGET__M5STICK_C_PLUS
		#define DISPLAY_240X135
		#define BTN_LAYOUT_STICK_C_PLUS

	#elif defined (ARDUINO_M5STACK_UNIT_C6L)
		#include "hardware/m5stack_unit_c6l/hal_m5stack_unit_c6l.h"
		#define TARGET__M5STACK_UNIT_C6L
		#define DISPLAY_64X48_MONO
		#define TASK_USER_INT_RUNNING_CORE 0

	#elif defined (ARDUINO_M5STACK_STICKS3)
		#include "hardware/m5stack_stick_s3/hal_m5stack_stick_s3.h"
		#define TARGET__M5STACK_STICK_S3
		#define DISPLAY_240X135
		#define BTN_LAYOUT_STICK_C_PLUS

	#elif defined (ARDUINO_ESP32S3_DEV) && defined (ARDUINO_PARTITION_app3M_fat9M_16MB)
		#include "hardware/lilygo_t_display_s3_pro/hal_lilygo_t_display_s3_pro.h"
		#define TARGET__LILYGO_T_DISPLAY_S3_PRO
		#define DISPLAY_222X480

	#elif defined (ARDUINO_ARDUINO_NESSO_N1)
		#include "hardware/arduino_nesso_n1/hal_arduino_nesso_n1.h"
		#define TARGET__ARDUINO_NESSO_N1
		#define DISPLAY_240X135
		#define BTN_LAYOUT_STICK_C_PLUS
		#define TASK_USER_INT_RUNNING_CORE 0
		#define DISPLAY_INCLUDE_LORA

	#else
		#include "hardware/esp32_devkit/hal_esp32_devkit.h"
		#define TARGET__ESP32_DEVKIT
		#define DISPLAY_CHARACTER_8X2
		#define INPUT_WAIT_TIME (10)

	#endif

	#define RENDER_FACE_CACHE_SIZE     1       //!< OpenFontRender 用のキャッシュサイズ定義
	#define RENDER_SIZES_CACHE_SIZE    5       //!< OpenFontRender 用のキャッシュサイズ定義
	#define RENDER_BYTES_CACHE_SIZE    1024    //!< OpenFontRender 用のキャッシュサイズ定義
	#define TASK_TRANSMIT_PRIORITY     0       //!< 0 - 24（低 - 高）
	#define TASK_TRANSMIT_RUNNING_CORE 0
	#define HAL_REGISTER_TRANSMIT_TASK() xTaskCreatePinnedToCore(transmitLoop, "TRANSMIT", 8192, NULL, TASK_TRANSMIT_PRIORITY, NULL, TASK_TRANSMIT_RUNNING_CORE)	//!< 送信タスク（transmitLoop）の定義
	#define TASK_USER_INT_PRIORITY     1       //!< 0 - 24（低 - 高）
	#ifndef TASK_USER_INT_RUNNING_CORE
		#define TASK_USER_INT_RUNNING_CORE 1
	#endif
	#define HAL_REGISTER_USER_INTERACTIVE_TASK() xTaskCreatePinnedToCore(userInteractiveLoop, "USER_INT", 8192, NULL, TASK_USER_INT_PRIORITY, NULL, TASK_USER_INT_RUNNING_CORE)	//!< ユーザ入力応答タスク（userInteractiveLoop）の定義
	#define HAL_BLE_TX_SIZE (BLE_BUF_LENGTH / 2)
	#ifndef INPUT_WAIT_TIME
		#define INPUT_WAIT_TIME (5000)
	#endif

#elif defined (WIO_TERMINAL)
	/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/
	//  Wio Terminal
	/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/
	#include "hardware/wio_terminal/hal_wio_terminal.h"
	#define TARGET__WIO_TERMINAL
	#define DISPLAY_320X240

	#define RENDER_FACE_CACHE_SIZE  1       //!< OpenFontRender 用のキャッシュサイズ定義
	#define RENDER_SIZES_CACHE_SIZE 1       //!< OpenFontRender 用のキャッシュサイズ定義
	#define RENDER_BYTES_CACHE_SIZE 1       //!< OpenFontRender 用のキャッシュサイズ定義
	#define TASK_TRANSMIT_PRIORITY  0       //!< 0 - 24（低 - 高）
	#define HAL_REGISTER_TRANSMIT_TASK() xTaskCreate(transmitLoop, "TRANSMIT", 1024 * 2 + 512, NULL, TASK_TRANSMIT_PRIORITY, &TransmitTaskHandle)	//!< 送信タスク（transmitLoop）の定義
	#define TASK_USER_INT_PRIORITY  24      //!< 0 - 24（低 - 高）
	#define HAL_REGISTER_USER_INTERACTIVE_TASK() xTaskCreate(userInteractiveLoop, "USER_INT", 1024 + 64, NULL, TASK_USER_INT_PRIORITY, &UserTaskHandle)	//!< ユーザ入力応答タスク（userInteractiveLoop）の定義

	#define HAL_BLE_TX_SIZE (BLE_BUF_LENGTH / 2)

	#define INPUT_WAIT_TIME (5000)
	
#elif defined (ARDUINO_WIO_TRACKER_1110)
	#include "hardware/wio_tracker_1110/hal_wio_tracker_1110.h"
	#define TARGET__WIO_TRACKER_1110

	#define RENDER_FACE_CACHE_SIZE  1       //!< OpenFontRender 用のキャッシュサイズ定義
	#define RENDER_SIZES_CACHE_SIZE 1       //!< OpenFontRender 用のキャッシュサイズ定義
	#define RENDER_BYTES_CACHE_SIZE 1       //!< OpenFontRender 用のキャッシュサイズ定義
	#define TASK_TRANSMIT_PRIORITY  0       //!< 0 - 24（低 - 高）
	#define HAL_REGISTER_TRANSMIT_TASK() xTaskCreate(transmitLoop, "TRANSMIT", 1024 * 2 + 512, NULL, TASK_TRANSMIT_PRIORITY, &TransmitTaskHandle)	//!< 送信タスク（transmitLoop）の定義
	#define HAL_REGISTER_USER_INTERACTIVE_TASK()

	#define HAL_BLE_TX_SIZE (BLE_BUF_LENGTH / 2)

	#define INPUT_WAIT_TIME (5000)

	#define HAL_ENABLE_EXCEPTIONAL_BUFFER _DISABLE_

	#define ENABLE_ASCII_ART _ENABLE_
#endif

/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/
//  Common
/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/
#ifndef HAL_MQTT_MAX_PACKET_SIZE
	#define HAL_MQTT_MAX_PACKET_SIZE (4096)
#endif

#ifndef HAL_HTTP_MAX_PACKET_SIZE
	#define HAL_HTTP_MAX_PACKET_SIZE (8192)
#endif

#ifndef HAL_ENABLE_EXCEPTIONAL_BUFFER
	#define HAL_ENABLE_EXCEPTIONAL_BUFFER _ENABLE_
#endif

#ifndef ENABLE_ASCII_ART
	#define ENABLE_ASCII_ART _ENABLE_
#endif
// clang-format on
#endif /* HAL_H */