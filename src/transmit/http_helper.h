#ifndef HTTP_HELPER_H
#define HTTP_HELPER_H

#include "../hal/hal.h"
#include <ArduinoHttpClient.h>

namespace HttpHelper {
	struct UniformResourceLocator {
		String url;
		String protocol;
		String host;
		uint16_t port;
		String path;
		String base64auth;

		void clear() {
			url        = "";
			protocol   = "";
			host       = "";
			port       = 0;
			path       = "";
			base64auth = "";
		}
	};

	// ==============================================================
	//! @brief URL を解析して UniformResourceLocator 構造体に格納します
	//! @param[in] (url) 解析する URL
	//! @param[in] (result) 解析結果（UniformResourceLocator 構造体）
	//! @return 解析に成功した場合は true, それ以外は false を返す
	// =============================================================
	bool tryParseUrl(String url, struct UniformResourceLocator &result);

	// ==============================================================
	//! @brief UniformResourceLocator 構造体の内容を表示します
	//! @param[in] (url) 表示する UniformResourceLocator 構造体
	// =============================================================
	void showParsedUrl(struct UniformResourceLocator url);

};

#endif /* HTTP_HELPER_H */