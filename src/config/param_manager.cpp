#include "param_manager.h"

ParamManager::ParamManager() {
	_setting_fpath = "/settings.json";
}

enum SettingLoadResult ParamManager::begin(bool reset_settings) {
	enum SettingLoadResult result = SUCCESS;
	// debugLog("Initialize setting template...");
	setSettingTemplate(defineSettingTemplate());

	if (reset_settings || !HAL.FileIO.loadSettings(_setting_fpath, _doc)) {
		// debugLog("Initialized setting file");
		reload(true);
		result = INITIALIZED;
	}

	if (!checkParamItems(_doc, defineSettingTemplate())) {
		// debugLog("Format storage...");
		if (!HAL.FileIO.formatStorage()) {
			// debugLog("Format failed...");
			return FAILED;
		} else {
			// debugLog("Format success...");
			setSettingTemplate(defineSettingTemplate());
			reload(true);

			if (!checkParamItems(_doc, defineSettingTemplate())) {
				result = FAILED;
			}
			result = FORMATTED;
		}
	}
	return result;
}

void ParamManager::setSettingFilePath(String fpath) {
	_setting_fpath = fpath;
}

String ParamManager::getSettingFilePath() {
	return _setting_fpath;
}

JsonDocument ParamManager::getSettingJson() {
	return _doc;
}

void ParamManager::reload(bool before_save) {
	if (before_save) {
		HAL.FileIO.saveSettings(_setting_fpath, _doc);
	}
	HAL.FileIO.loadSettings(_setting_fpath, _doc);
}

void ParamManager::setSettingTemplate(JsonDocument setting) {
	_doc = setting;
	//_doc.garbageCollect();
}

void ParamManager::clearSettingTemplate() {
	_doc.clear();
	//_doc.garbageCollect();
}

// Does target have all keys which source have?
bool ParamManager::checkParamItems(JsonDocument target, JsonDocument source) {
	for (JsonPair kv : source.as<JsonObject>()) {
		if (kv.value().is<JsonObject>()) {
			if (!checkParamItems(target[kv.key()], source[kv.key()])) {
				return false;
			}
			continue;
		}
		if (target[kv.key()].isNull()) {
			// target not have
			return false;
		}
	}
	return true;
}

void ParamManager::debugLog(const char *fmt, ...) {
	char s[256] = {'\0'};
	int n       = 256;
	va_list arg;

	va_start(arg, fmt);
	vsnprintf(s, n, fmt, arg);
	va_end(arg);

	HAL.print("[Debug] ");
	HAL.println(s);
}

__attribute__((weak)) JsonDocument defineSettingTemplate() {
	JsonDocument ddoc;
	return ddoc;
}