#ifndef PARAM_MANAGER_H
#define PARAM_MANAGER_H

#include "../hal/hal.h"

enum SettingLoadResult {
	SUCCESS,
	INITIALIZED,
	FORMATTED,
	FAILED
};

JsonDocument defineSettingTemplate();

class ParamManager {
public:
	ParamManager();

	// ==============================================================
	//! @brief
	//! @param[in] (reset_settings)
	//! @return
	// =============================================================
	virtual enum SettingLoadResult begin(bool reset_settings = false);
	virtual void setSettingFilePath(String fpath);
	virtual String getSettingFilePath();
	virtual JsonDocument getSettingJson();
	virtual void reload(bool before_save = true);

	template <typename T>
	T getParameter(const char *category, const char *key) {
		return _doc[category][key].as<T>();
	};

	bool checkParameter(const char *category, const char *key) {
		JsonVariant err = _doc[category][key];
		if (err.isNull()) {
			return false;
		}
		return true;
	};

	virtual void setSettingTemplate(JsonDocument setting);
	virtual void clearSettingTemplate();

protected:
	String _setting_fpath;
	JsonDocument _doc;

	bool checkParamItems(JsonDocument target, JsonDocument source);
	void debugLog(const char *fmt, ...);
};

#endif /* PARAM_MANAGER_H */