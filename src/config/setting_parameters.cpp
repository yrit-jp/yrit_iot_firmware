#include "setting_parameters.h"
#ifdef COMMON_SETTING_PARAMETERS_H

/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/
//  Prototype definition
/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/

JsonDocument wifiSettingTemplate();
JsonDocument lanSettingTemplate();
JsonDocument ambientSettingTemplate();
JsonDocument ftpSettingTemplate();
JsonDocument httpSettingTemplate();
JsonDocument mqttSettingTemplate();

/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/
//  Public
/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/
JsonDocument commonSettingTemplate() {
	JsonDocument doc;

	if (HAL.FileIO.usePresetWiFi() == true) {
		JsonDocument wifi = wifiSettingTemplate();
		mergeJson(doc.as<JsonVariant>(), wifi.as<JsonVariant>());
		// wifi.~BasicJsonDocument();
	}

	if (HAL.FileIO.usePresetLan() == true) {
		JsonDocument lan = lanSettingTemplate();
		mergeJson(doc.as<JsonVariant>(), lan.as<JsonVariant>());
	}

	{
		JsonObject PARAM = doc["PARAM"].to<JsonObject>();
		PARAM["devID"]   = "99_99";
		if (doc["WIFI"] || doc["LAN"]) {
			PARAM["WAIT_00_SEC"]     = "false";
			PARAM["BATCH_DATA_SEND"] = "false";
		}
		PARAM["TX_INTERVAL_SEC"] = "5";

		JsonObject SAMPLING = doc["SAMPLING"].to<JsonObject>();
		SAMPLING["I2C_MS"]  = "3000";
	}

	if (HAL.FileIO.usePresetAmbient() == true) {
		JsonDocument ambient = ambientSettingTemplate();
		mergeJson(doc.as<JsonVariant>(), ambient.as<JsonVariant>());
		// ambient.~BasicJsonDocument();
	}

	if (HAL.FileIO.usePresetFtp() == true) {
		JsonDocument ftp = ftpSettingTemplate();
		mergeJson(doc.as<JsonVariant>(), ftp.as<JsonVariant>());
		// ftp.~BasicJsonDocument();
	}

	if (HAL.FileIO.usePresetHttp() == true) {
		JsonDocument http = httpSettingTemplate();
		mergeJson(doc.as<JsonVariant>(), http.as<JsonVariant>());
		// http.~BasicJsonDocument();
	}

	if (HAL.FileIO.usePresetMqtt() == true) {
		JsonDocument mqtt = mqttSettingTemplate();
		mergeJson(doc.as<JsonVariant>(), mqtt.as<JsonVariant>());
		// mqtt.~BasicJsonDocument();
	}

	{
		JsonObject I2C_PORT1           = doc["I2C_PORT1"].to<JsonObject>();
		I2C_PORT1["IGNORE_ADDR_REGEX"] = "";
		I2C_PORT1["IGNORE_NAME_REGEX"] = "";
	}

	return doc;
}

JsonDocument defineSettingTemplate() {

	JsonDocument unique = HAL.FileIO.deviceUniqueSettings();
	JsonDocument common = commonSettingTemplate();
	mergeJson(common.as<JsonVariant>(), unique.as<JsonVariant>());

	// unique.~BasicJsonDocument(); // release

	// For debug
	// serializeJsonPretty(tmpl, Serial);

	return common;
}

void mergeJson(JsonVariant dst, JsonVariantConst src) {
	if (src.is<JsonObjectConst>()) {
		for (auto kvp : src.as<JsonObjectConst>()) {
			if (dst[kvp.key()]) {
				mergeJson(dst[kvp.key()], kvp.value());
			} else {
				dst[kvp.key()] = kvp.value();
			}
		}
	} else {
		if (not src.isNull()) {
			dst.set(src);
		}
	}
}

/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/
//  Private
/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/

JsonDocument wifiSettingTemplate() {
	JsonDocument doc;

	JsonObject WIFI            = doc["WIFI"].to<JsonObject>();
	WIFI["SSID"]               = "";
	WIFI["PASS"]               = "";
	WIFI["NTP_SERVER"]         = "ntp.nict.jp";
	WIFI["STATIC_IP"]          = "";
	WIFI["STATIC_GATEWAY"]     = "";
	WIFI["STATIC_SUBNET_MASK"] = "";
	WIFI["DNS"]                = "";

	return doc;
}

JsonDocument lanSettingTemplate() {
	JsonDocument doc;

	JsonObject LAN            = doc["LAN"].to<JsonObject>();
	LAN["NTP_SERVER"]         = "ntp.nict.jp";
	LAN["STATIC_IP"]          = "";
	LAN["STATIC_GATEWAY"]     = "";
	LAN["STATIC_SUBNET_MASK"] = "";
	LAN["DNS"]                = "";

	return doc;
}

JsonDocument ambientSettingTemplate() {
	JsonDocument doc;

	JsonObject AMBIENT = doc["AMBIENT"].to<JsonObject>();
	// 現在はすべてのセンサが
	AMBIENT["ENABLE"]         = "false";
	AMBIENT["WRITE_KEY"]      = "";
	AMBIENT["CHANNEL_ID"]     = "";
	AMBIENT["D1_SENSOR_NAME"] = "";
	AMBIENT["D2_SENSOR_NAME"] = "";
	AMBIENT["D3_SENSOR_NAME"] = "";
	AMBIENT["D4_SENSOR_NAME"] = "";

	return doc;
}

JsonDocument ftpSettingTemplate() {
	JsonDocument doc;

	JsonObject FTP   = doc["FTP"].to<JsonObject>();
	FTP["ENABLE"]    = "false";
	FTP["SERVER"]    = "";
	FTP["USER"]      = "";
	FTP["PASS"]      = "";
	FTP["DIRECTORY"] = "/";

	return doc;
}

JsonDocument httpSettingTemplate() {
	JsonDocument doc;

	JsonObject HTTP = doc["HTTP"].to<JsonObject>();
	HTTP["ENABLE"]  = "false";
	HTTP["URL"]     = "";

	return doc;
}

JsonDocument mqttSettingTemplate() {
	JsonDocument doc;

	JsonObject MQTT    = doc["MQTT"].to<JsonObject>();
	MQTT["ENABLE"]     = "false";
	MQTT["BROKER"]     = "";
	MQTT["PORT"]       = "1883";
	MQTT["USER"]       = "";
	MQTT["PASS"]       = "";
	MQTT["BASE_TOPIC"] = "";
	MQTT["RETAIN"]     = "false";

	return doc;
}

#endif /* COMMON_SETTING_PARAMETERS_H */