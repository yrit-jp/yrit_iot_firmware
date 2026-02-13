#ifndef COMMON_SETTING_PARAMETERS_H
#define COMMON_SETTING_PARAMETERS_H

#include "../hal/hal.h"
#include "param_manager.h"
#ifndef SETTING_JSON_CAP
#define SETTING_JSON_CAP (4095) //(2560)	//(1024 * 2 + 512)
#endif

JsonDocument commonSettingTemplate();

// ==============================================================
//! @brief 2つのJsonObjectを統合する関数
//! @param[in/out] (dst) 統合されるJsonObject
//! @param[in] (src) 統合するJsonObject
//! @note JsonObjectのキーが被った場合は、srcの値で上書きされる。
//! @note 呼び出し元でJsonObjectをJsonVariantにキャストする必要がある。doc.as<JsonVariant>();
//! @see https://arduinojson.org/v7/how-to/merge-json-objects/
// =============================================================
void mergeJson(JsonVariant dst, JsonVariantConst src);

#endif /* COMMON_SETTING_PARAMETERS_H */