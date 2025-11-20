#include "http_helper.h"

bool HttpHelper::tryParseUrl(String url, struct UniformResourceLocator &result) {
	// check for : (http: or https:
	int index = url.indexOf(':');
	if (index < 0) {
		return false;
	}

	result.clear();
	result.url      = url;
	result.protocol = url.substring(0, index);
	url.remove(0, (index + 3)); // remove http:// or https://

	index = url.indexOf('/');
	if (index == -1) {
		index = url.length();
		url += '/';
	}
	String host = url.substring(0, index);
	url.remove(0, index); // remove host part

	// get Authorization
	// index = host.indexOf('@');
	// if (index >= 0) {
	// 	// auth info
	// 	String auth = host.substring(0, index);
	// 	host.remove(0, index + 1); // remove auth part including @
	// 	result.base64auth = base64::encode(auth);
	// }

	// get port
	index           = host.indexOf(':');
	String the_host = "";
	if (index >= 0) {
		the_host = host.substring(0, index); // hostname
		host.remove(0, (index + 1));         // remove hostname + :
		result.port = host.toInt();          // get port
	} else {
		the_host = host;
	}
	result.host = the_host;
	result.path = url;

	if (result.port != 0) {
		return true;
	}

	result.port = (result.protocol == "https") ? 443 : 80;

	return true;
}

void HttpHelper::showParsedUrl(struct UniformResourceLocator url) {
	HAL.println("");
	HAL.printf("/> Protocol: %s\n", url.protocol.c_str());
	HAL.printf("/> Port    : %d\n", url.port);
	HAL.printf("/> Host    : %s\n", url.host.c_str());
	HAL.printf("/> Path    : %s\n", url.path.c_str());
}