#include "iot_tx_ftp.h"

void IoTTxFTP::setFtpParam(String server, String user, String pass, uint8_t _ftp_verbose) {
	_mutex_key    = "iot_tx_ftp";
	_DEBUG_PREFIX = "FTP";

	if (server.endsWith(".local")) {
		// Resolve mDNS address
		MyIPAddress ip;
		switch (HAL.getUseLinkLayer()) {
		case DATALINK_LAYER::WIFI:
			ip = HAL.Wlan.resolveMDNS(server);
			break;
		case DATALINK_LAYER::ETHERNET:
			ip = HAL.Lan.resolveMDNS(server);
			break;
		default:
			break;
		}

		if (ip.toString() == "0.0.0.0") {
			debugMessage(LogCategory::FTP, LogLevel::Error, "Failed to resolve mDNS");
		} else {
			debugMessage(LogCategory::FTP, LogLevel::Info, ("mDNS Resolve: " + server + " -> " + ip.toString()).c_str());
		}
		_ftp.setServerAddress(ip.toString());
	} else {
		_ftp.setServerAddress(server);
	}

	switch (HAL.getUseLinkLayer()) {
	case DATALINK_LAYER::WIFI:
		_ftp.setClient(*(HAL.Wlan.getClient("ftp")));
		_ftp.setDataClient(*(HAL.Wlan.getClient("ftp_data")));
		break;
	case DATALINK_LAYER::ETHERNET:
		_ftp.setClient(*(HAL.Lan.getClient("ftp")));
		_ftp.setDataClient(*(HAL.Lan.getClient("ftp_data")));
		break;
	default:
		break;
	}

	_ftp.setAccount(user, pass);
	_ftp.setVerbose(_ftp_verbose);
	_ftp.setTimeout(5000);

	_heartbeat_store = false;
}

void IoTTxFTP::setFilePath(String current_dir, String file_name) {
	_current_dir = current_dir;
	_file_name   = file_name;
}

void IoTTxFTP::setHeartbeatFilePath(String current_dir, String file_name) {
	_heartbeat_current_dir = current_dir;
	_heartbeat_file_name   = file_name;
}

bool IoTTxFTP::txFtp(String dir, String file_name, String value, bool create_new) {
	bool result = true;

	{
		/* -+-+-+-+- Mutex block BEGIN -+-+-+-+- */
		EthernetMutexGuard _;
		_ftp.openConnection();
		if (not _ftp.isConnected()) {
			_ftp.closeConnection();
			result = false;
		}
		/* -+-+-+-+- Mutex block END -+-+-+-+- */
	}

	if (result == false) {
		debugMessage(LogCategory::FTP, LogLevel::Error, "Failed OpenConnection");
		return false;
	}
	result = true;

	{
		/* -+-+-+-+- Mutex block BEGIN -+-+-+-+- */
		EthernetMutexGuard _;
		_ftp.changeWorkDir(dir.c_str());
		if (not _ftp.isConnected()) {
			_ftp.closeConnection();
			result = false;
		}
		/* -+-+-+-+- Mutex block END -+-+-+-+- */
	}

	if (result == false) {
		debugMessage(LogCategory::FTP, LogLevel::Error, "Failed CWD");
		debugMessage(LogCategory::FTP, LogLevel::Error, dir.c_str());
		return false;
	}
	result = true;

	{
		/* -+-+-+-+- Mutex block BEGIN -+-+-+-+- */
		EthernetMutexGuard _;
		_ftp.initFile("Type A");
		if (not _ftp.isConnected()) {
			_ftp.closeConnection();
			result = false;
		}
		/* -+-+-+-+- Mutex block END -+-+-+-+- */
	}

	if (result == false) {
		debugMessage(LogCategory::FTP, LogLevel::Error, "Failed Init Type");
		return false;
	}
	result = true;

	{
		/* -+-+-+-+- Mutex block BEGIN -+-+-+-+- */
		EthernetMutexGuard _;
		_ftp.clearBuffer();
		if (create_new) {
			// Create new file
			_ftp.newFile(file_name.c_str());
			if (not _ftp.isConnected()) {
				_ftp.closeConnection();
				result = false;
			}
		} else {
			// Append to file
			_ftp.appendFile((char *)file_name.c_str());
			if (not _ftp.isConnected()) {
				_ftp.closeConnection();
				result = false;
			}
		}
		/* -+-+-+-+- Mutex block END -+-+-+-+- */
	}

	if (result == false) {
		if (create_new) {
			debugMessage(LogCategory::FTP, LogLevel::Error, "Failed New Create");
		} else {
			debugMessage(LogCategory::FTP, LogLevel::Error, "Failed APPE");
		}
		return false;
	}
	result = true;

	{
		/* -+-+-+-+- Mutex block BEGIN -+-+-+-+- */
		EthernetMutexGuard _;
		_ftp.write(value.c_str());
		if (not _ftp.isConnected()) {
			_ftp.closeConnection();
			result = false;
		}
		/* -+-+-+-+- Mutex block END -+-+-+-+- */
	}

	if (result == false) {
		debugMessage(LogCategory::FTP, LogLevel::Error, "Failed Write");
		return false;
	}
	result = true;

	{
		/* -+-+-+-+- Mutex block BEGIN -+-+-+-+- */
		EthernetMutexGuard _;
		_ftp.closeFile();
		_ftp.closeConnection();
		/* -+-+-+-+- Mutex block END -+-+-+-+- */
	}

	return true;
}

bool IoTTxFTP::uploadImage(String dir, String file_name, const std::vector<uint8_t> &jpgImage) {
	bool result = true;

	{
		/* -+-+-+-+- Mutex block BEGIN -+-+-+-+- */
		EthernetMutexGuard _;
		_ftp.openConnection();
		if (not _ftp.isConnected()) {
			_ftp.closeConnection();
			result = false;
		}
		/* -+-+-+-+- Mutex block END -+-+-+-+- */
	}

	if (result == false) {
		debugMessage(LogCategory::FTP, LogLevel::Error, "Failed OpenConnection");
		return false;
	}
	result = true;

	{
		/* -+-+-+-+- Mutex block BEGIN -+-+-+-+- */
		EthernetMutexGuard _;
		_ftp.changeWorkDir(dir.c_str());
		if (not _ftp.isConnected()) {
			_ftp.closeConnection();
			result = false;
		}
		/* -+-+-+-+- Mutex block END -+-+-+-+- */
	}

	if (result == false) {
		debugMessage(LogCategory::FTP, LogLevel::Error, "Failed CWD");
		debugMessage(LogCategory::FTP, LogLevel::Error, dir.c_str());
		return false;
	}
	result = true;

	{
		/* -+-+-+-+- Mutex block BEGIN -+-+-+-+- */
		EthernetMutexGuard _;
		_ftp.initFile("Type I");
		if (not _ftp.isConnected()) {
			_ftp.closeConnection();
			result = false;
		}
		/* -+-+-+-+- Mutex block END -+-+-+-+- */
	}

	if (result == false) {
		debugMessage(LogCategory::FTP, LogLevel::Error, "Failed Init Type");
		return false;
	}
	result = true;

	{
		/* -+-+-+-+- Mutex block BEGIN -+-+-+-+- */
		EthernetMutexGuard _;
		_ftp.clearBuffer();
		// Create new file
		_ftp.newFile(file_name.c_str());
		if (not _ftp.isConnected()) {
			_ftp.closeConnection();
			result = false;
		}
		/* -+-+-+-+- Mutex block END -+-+-+-+- */
	}

	if (result == false) {
		debugMessage(LogCategory::FTP, LogLevel::Error, "Failed New Create");
		debugMessage(LogCategory::FTP, LogLevel::Error, dir.c_str());
		debugMessage(LogCategory::FTP, LogLevel::Error, file_name.c_str());
		return false;
	}
	result = true;

	{
		/* -+-+-+-+- Mutex block BEGIN -+-+-+-+- */
		EthernetMutexGuard _;
		_ftp.writeData((uint8_t *)(&jpgImage[0]), jpgImage.size());
		if (not _ftp.isConnected()) {
			_ftp.closeConnection();
			result = false;
		}
		/* -+-+-+-+- Mutex block END -+-+-+-+- */
	}

	if (result == false) {
		debugMessage(LogCategory::FTP, LogLevel::Error, "Failed Write");
		return false;
	}
	result = true;

	{
		/* -+-+-+-+- Mutex block BEGIN -+-+-+-+- */
		EthernetMutexGuard _;
		_ftp.closeFile();
		_ftp.closeConnection();
		/* -+-+-+-+- Mutex block END -+-+-+-+- */
	}

	return true;
}

/**********************************
 * Protected functions
 **********************************/
bool IoTTxFTP::preAddExceptionalValue(uint8_t storage) {
	constexpr uint8_t EX_STORAGE_NUMBER_0 = 0;
	constexpr uint8_t EX_STORAGE_NUMBER_1 = 1;

	switch (storage) {
	case EX_STORAGE_NUMBER_0:
		// カメラ画像
		return true;
	case EX_STORAGE_NUMBER_1:
		// バッチ送信
		debugMessage(LogCategory::FTP, LogLevel::Warning, "Batch transfer is NOT supported!");
		return false;
	default:
		break;
	}
	return false;
}

bool IoTTxFTP::makeDirRecursive(String absolute_path) {
	bool result = true;

	{
		/* -+-+-+-+- Mutex block BEGIN -+-+-+-+- */
		EthernetMutexGuard _;
		_ftp.openConnection();
		if (not _ftp.isConnected()) {
			_ftp.closeConnection();
			result = false;
		}
		/* -+-+-+-+- Mutex block END -+-+-+-+- */
	}

	if (result == false) {
		debugMessage(LogCategory::FTP, LogLevel::Error, "Failed OpenConnection");
		return false;
	}
	result = true;

	String exist_dir_path = "/";
	while (1) {
		int sp = absolute_path.indexOf('/', 1);

		String d;
		if (sp < 0) {
			d = absolute_path.substring(1, absolute_path.length());
			if (d.length() == 0) {
				break;
			}
		} else {
			d = absolute_path.substring(1, sp);
		}

		// HAL.printf("ed: %s\n", exist_dir_path.c_str());
		// HAL.printf("d: %s\n", d.c_str());
		uint16_t rc;
		{
			/* -+-+-+-+- Mutex block BEGIN -+-+-+-+- */
			EthernetMutexGuard _;
			_ftp.changeWorkDir(d.c_str());
			rc = _ftp.getLastResponseCode();
			/* -+-+-+-+- Mutex block END -+-+-+-+- */
		}

		if (rc >= 400) {
			String tmp = "Try to create directory: ";
			tmp += exist_dir_path + d;
			debugMessage(LogCategory::FTP, LogLevel::Debug, tmp.c_str());

			{
				/* -+-+-+-+- Mutex block BEGIN -+-+-+-+- */
				EthernetMutexGuard _;

				// Re-open _ftp.connection
				_ftp.openConnection();
				_ftp.changeWorkDir(exist_dir_path.c_str());
				_ftp.makeDir(d.c_str());
				if (not _ftp.isConnected()) {
					_ftp.closeConnection();
					result = false;
				}
				/* -+-+-+-+- Mutex block END -+-+-+-+- */
			}

			if (result == false) {
				debugMessage(LogCategory::FTP, LogLevel::Error, "Failed MakeDir");
				return false;
			}
			result = true;

			{
				/* -+-+-+-+- Mutex block BEGIN -+-+-+-+- */
				EthernetMutexGuard _;
				_ftp.changeWorkDir(d.c_str());
				if (not _ftp.isConnected()) {
					_ftp.closeConnection();
					result = false;
				}
				/* -+-+-+-+- Mutex block END -+-+-+-+- */
			}

			if (result == false) {
				debugMessage(LogCategory::FTP, LogLevel::Error, "Failed ChangeWorkDir in loop");
				return false;
			}
			result = true;

			tmp = "Success to create directory!: ";
			tmp += exist_dir_path + d;
			debugMessage(LogCategory::FTP, LogLevel::Debug, tmp.c_str(), "\n");
		}

		exist_dir_path += d + "/";
		absolute_path = absolute_path.substring(sp, absolute_path.length());
	}

	{
		/* -+-+-+-+- Mutex block BEGIN -+-+-+-+- */
		EthernetMutexGuard _;
		// _ftp.closeFile();
		_ftp.closeConnection();
		/* -+-+-+-+- Mutex block END -+-+-+-+- */
	}
	return true;
}

String IoTTxFTP::getDirName(String path) {
	int idx        = path.lastIndexOf('/');
	String dirname = path.substring(0, idx + 1);
	return dirname;
}

String IoTTxFTP::getFileName(String path) {
	int idx      = path.lastIndexOf('/');
	String fname = path.substring(idx + 1);
	return fname;
}

String IoTTxFTP::joinPath(String p1, String p2) {
	if (not p1.endsWith("/") and not p2.startsWith("/")) {
		p1.concat("/");
	}
	p1.concat(p2);
	return p1;
}

IoTTxFTP::FileExist IoTTxFTP::exist(String fpath) {
	IoTTxFTP::FileExist result = FE_NOT_EXIST;
	String ret                 = "";

	{
		/* -+-+-+-+- Mutex block BEGIN -+-+-+-+- */
		EthernetMutexGuard _;
		_ftp.openConnection();
		if (not _ftp.isConnected()) {
			result = FE_FTP_ERROR;
		}
		/* -+-+-+-+- Mutex block END -+-+-+-+- */
	}

	if (result == FE_FTP_ERROR) {
		return FE_FTP_ERROR;
	}

	{
		/* -+-+-+-+- Mutex block BEGIN -+-+-+-+- */
		EthernetMutexGuard _;
		_ftp.renameFile(fpath.c_str(), fpath.c_str());
		uint16_t rc = _ftp.getLastResponseCode();

		if (200 <= rc and rc < 300) {
			result = FE_EXIST;
		} else if (rc == 503) {
			result = FE_NOT_EXIST;
		} else {
			result = FE_FTP_ERROR;
		}
		_ftp.closeConnection();
		/* -+-+-+-+- Mutex block END -+-+-+-+- */
	}
	return result;
}

bool IoTTxFTP::transmitHeartbeat() {
	if (_penalty_wait.isActive()) {
		return false;
	}

	if (not _heartbeat_store) {
		return false;
	}

	switch (HAL.getUseLinkLayer()) {
	case DATALINK_LAYER::WIFI:
		if (not HAL.Wlan.connectWlan()) {
			_penalty_wait.activate(5000);
			return false;
		}
		break;
	case DATALINK_LAYER::ETHERNET:
		if (HAL.Lan.isLinkUp() == false) {
			_penalty_wait.activate(5000);
			return false;
		}
		break;
	default:
		break;
	}

	if (not makeDirRecursive(_heartbeat_current_dir)) {
		debugMessage(LogCategory::FTP_HB, LogLevel::Error, ("Failed to create directory -> " + _heartbeat_current_dir).c_str());
		// WiFi disconnect
		// disconnectWiFi();
		return false;
	}

	char s_day[64] = {'\0'};
	strftime(s_day, 64, "%Y/%m/%d %H:%M:%S", &_heartbeat_tm);
	unsigned long uptime_sec = millis() / 1000;
	uint32_t free_heap       = HAL.getFreeHeapSize();
	uint32_t min_heap        = HAL.getMinimumFreeHeapSize();
	char buffer[256]         = {'\0'};
	int res                  = snprintf(buffer, 256, "day, uptimeSec, freeHeap, minHeap\n%s, %lu, %lu, %lu\n",
	                                    s_day, uptime_sec, (unsigned long)free_heap, (unsigned long)min_heap);
	bool success             = false;
	if (res > 0) {
		success = txFtp(_heartbeat_current_dir, _heartbeat_file_name, buffer, true);
	}
	if (success) {
		_heartbeat_store = false;
	} else {
		debugMessage(LogCategory::FTP_HB, LogLevel::Error, "Failed to write heartbeat", "\n");
		// disconnectWiFi();
		_penalty_wait.activate(1000);
		return false;
	}

	if (_auto_disconnect) {
		if (HAL.getUseLinkLayer() == DATALINK_LAYER::WIFI) {
			HAL.Wlan.disconnectWlan();
		}
	}
	return true;
}

IoTTx_Tx_Result IoTTxFTP::transmit() {
	String prev_dir             = "";
	enum IoTTx_Tx_Result result = RESULT_TX_ERROR;

	if (_penalty_wait.isActive()) {
		return result;
	}

	switch (HAL.getUseLinkLayer()) {
	case DATALINK_LAYER::WIFI:
		if (not HAL.Wlan.connectWlan()) {
			_penalty_wait.activate(5000);
			return result;
		}
		break;
	case DATALINK_LAYER::ETHERNET:
		if (HAL.Lan.isLinkUp() == false) {
			_penalty_wait.activate(5000);
			return result;
		}
		break;
	default:
		break;
	}

	while (_tx_idx.store != _tx_idx.transmit) {
		HAL.takeSemaphoreMutex(_mutex_key, portMAX_DELAY);

		char dirname[64] = {'\0'};
		strftime(dirname, 64, "%Y%m%d", &_buffer[_tx_idx.transmit].time);
		HAL.giveSemaphoreMutex(_mutex_key);

		String save_dir = _current_dir + String(dirname);

		if (not prev_dir.equals(save_dir)) {
			// Create directory and csv header
			if (not makeDirRecursive(save_dir)) {
				debugMessage(LogCategory::FTP, LogLevel::Error, ("Failed to create directory: " + save_dir).c_str());
				_failed_counter++;
				if (_failed_counter >= 5) {
					_failed_counter = 0;
					_penalty_wait.activate(5000);
				} else {
					_penalty_wait.activate(1000);
				}
				return result;
			}

			IoTTxFTP::FileExist fe = exist(joinPath(save_dir, _file_name));
			if (fe == FE_FTP_ERROR) {
				debugMessage(LogCategory::FTP, LogLevel::Error, "Failed to check file exist", "\n");
				_failed_counter++;
				if (_failed_counter >= 5) {
					_failed_counter = 0;
					_penalty_wait.activate(5000);
				} else {
					_penalty_wait.activate(1000);
				}
				return result;
			}
			if (fe == FE_NOT_EXIST) {
				_file_not_exist_count++;
				if (_file_not_exist_count >= 3) {
					debugMessage(LogCategory::FTP, LogLevel::Info, "Create new file");
					String header = "day, value, devID, sensName, unit\n";
					if (_unix_time_field.enable) {
						header = "time, " + header;
					}
					txFtp(save_dir, _file_name, header.c_str(), true);
					_file_not_exist_count = 0;
				} else {
					_penalty_wait.activate(500);
					return result;
				}
			}
			_file_not_exist_count = 0;
			prev_dir              = String(save_dir);
		}

		HAL.takeSemaphoreMutex(_mutex_key, portMAX_DELAY);
		char s_day[64] = {'\0'};
		strftime(s_day, 64, "%Y/%m/%d %H:%M:%S", &_buffer[_tx_idx.transmit].time);

		String value     = _buffer[_tx_idx.transmit].value;
		String name      = _buffer[_tx_idx.transmit].name;
		String unit      = _buffer[_tx_idx.transmit].unit;
		String unix_time = "";
		if (_unix_time_field.enable) {
			signed long long ts = mktime(&_buffer[_tx_idx.transmit].time);

			if (ts != -1) {
				String s  = lld2String(ts);
				String p  = _unix_time_field.nano_precision ? "000000000" : "";
				unix_time = s + p;
			}
		}
		HAL.giveSemaphoreMutex(_mutex_key);

		char buffer[256] = {'\0'};
		int res          = snprintf(buffer, 256, "%s, %s, %s, %s, %s, %s\n",
		                            unix_time.c_str(),
		                            s_day,
		                            value.c_str(),
		                            _devID.c_str(),
		                            name.c_str(),
		                            unit.c_str());
		bool success     = false;
		if (res > 0) {
			HAL.delay(1);
			success = txFtp(save_dir, _file_name, buffer);
			HAL.delay(1);
		}
		if (success) {
			HAL.takeSemaphoreMutex(_mutex_key, portMAX_DELAY);
			_buffer[_tx_idx.transmit].value = "";
			_buffer[_tx_idx.transmit].name  = "";
			_buffer[_tx_idx.transmit].unit  = "";

			_tx_idx.transmit    = ((_tx_idx.transmit + 1) == IOTTX_BUFFER_SIZE) ? 0 : (_tx_idx.transmit + 1);
			_pending_data_count = (((_pending_data_count - 1) > 0) ? (_pending_data_count - 1) : 0);
			HAL.giveSemaphoreMutex(_mutex_key);
			result = RESULT_TX_SOME_DATA;
			// printIndex();
		} else {
			debugMessage(LogCategory::FTP, LogLevel::Error, "Failed to write data");
			_failed_counter++;
			if (_failed_counter >= 5) {
				_failed_counter = 0;
			}

			_penalty_wait.activate(5000);
			return result;
		}
	}

	result = RESULT_TX_ALL_DATA;
	if (_auto_disconnect) {
		if (HAL.getUseLinkLayer() == DATALINK_LAYER::WIFI) {
			HAL.Wlan.disconnectWlan();
		}
	}

	_failed_counter = 0;
	return result;
}

IoTTx_Tx_Result IoTTxFTP::transmitEx() {
#ifdef HAL_ENABLE_EXCEPTIONAL_BUFFER
	String prev_dir             = "";
	enum IoTTx_Tx_Result result = RESULT_TX_ERROR;

	if (_penalty_wait.isActive()) {
		return result;
	}

	switch (HAL.getUseLinkLayer()) {
	case DATALINK_LAYER::WIFI:
		if (not HAL.Wlan.connectWlan()) {
			_penalty_wait.activate(5000);
			return result;
		}
		break;
	case DATALINK_LAYER::ETHERNET:
		if (HAL.Lan.isLinkUp() == false) {
			_penalty_wait.activate(5000);
			return result;
		}
		break;
	default:
		break;
	}

	// for exceptional storage
	for (int storage = 0; storage < _EXCEPTIONAL_STORAGE_NUM; storage++) {
		if (_ex_buffer_info[storage].updated == false) {
			continue;
		}

		HAL.takeSemaphoreMutex(_mutex_key, portMAX_DELAY);
		char dirname[64] = {'\0'};
		strftime(dirname, 64, "%Y%m%d", &_ex_buffer_info[storage].time);
		HAL.giveSemaphoreMutex(_mutex_key);
		String save_dir = _current_dir;

		if (storage == 0) {
			// ex) Camera/00_00/20201210
			save_dir += "Camera/" + _devID + "/" + String(dirname);
		}

		if (not prev_dir.equals(save_dir)) {
			// Create directory and csv header
			if (not makeDirRecursive(save_dir)) {
				debugMessage(LogCategory::FTP_EX, LogLevel::Error, ("Failed to create directory: " + save_dir).c_str());
				_failed_counter++;
				if (_failed_counter >= 5) {
					_failed_counter = 0;
					_penalty_wait.activate(5000);
				} else {
					_penalty_wait.activate(1000);
				}
				return result;
			}
			prev_dir = String(save_dir);
		}

		HAL.takeSemaphoreMutex(_mutex_key, portMAX_DELAY);

		bool success = false;
		if (storage == 0) {
			char fname[64] = {'\0'};
			strftime(fname, 64, "%Y%m%d_%H%M%S.jpg", &_ex_buffer_info[storage].time);

			HAL.takeSemaphoreMutex("camera", portMAX_DELAY);
			success = uploadImage(save_dir, fname, HAL.Camera.jpgImage);
			HAL.giveSemaphoreMutex("camera");
		} else {
			HAL.giveSemaphoreMutex(_mutex_key);
			continue;
		}
		HAL.giveSemaphoreMutex(_mutex_key);

		if (success) {
			HAL.takeSemaphoreMutex(_mutex_key, portMAX_DELAY);
			_ex_buffer_info[storage].updated = false;
			HAL.giveSemaphoreMutex(_mutex_key);
			result = RESULT_TX_SOME_DATA;
			// printIndex();
		} else {
			debugMessage(LogCategory::FTP_EX, LogLevel::Error, "Failed to write data");
			_failed_counter++;
			if (_failed_counter >= 5) {
				_failed_counter = 0;
			}

			_penalty_wait.activate(5000);
			return result;
		}
	}

	result = RESULT_TX_ALL_DATA;
	if (_auto_disconnect) {
		if (HAL.getUseLinkLayer() == DATALINK_LAYER::WIFI) {
			HAL.Wlan.disconnectWlan();
		}
	}

	_failed_counter = 0;
	return result;
#else
	return RESULT_TX_NOTHING;
#endif /* HAL_ENABLE_EXCEPTIONAL_BUFFER */
}
