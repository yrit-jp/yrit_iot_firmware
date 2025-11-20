#include "iot_tx_ftp.h"

void IoTTxFTP::setFtpParam(String server, String user, String pass, uint8_t _ftp_verbose) {
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
			debugMessage("FTP", "Error", "Failed to resolve mDNS", "\n");
		} else {
			debugMessage("FTP", "Success", ("mDNS Resolve: " + server + " -> " + ip.toString()).c_str(), "\n");
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
		if (HAL.getUseLinkLayer() == DATALINK_LAYER::ETHERNET) {
			/* -+-+-+-+- Mutex block BEGIN -+-+-+-+- */
			HAL.Lan.takeSemaphoreMutex(portMAX_DELAY);
		}

		_ftp.openConnection();
		if (!_ftp.isConnected()) {
			_ftp.closeConnection();
			result = false;
		}

		if (HAL.getUseLinkLayer() == DATALINK_LAYER::ETHERNET) {
			HAL.Lan.giveSemaphoreMutex();
			/* -+-+-+-+- Mutex block END -+-+-+-+- */
		}
	}

	if (result == false) {
		debugMessage("txFTP", "Warning", "Failed OpenConnection", "\n");
		return false;
	}
	result = true;

	{
		if (HAL.getUseLinkLayer() == DATALINK_LAYER::ETHERNET) {
			/* -+-+-+-+- Mutex block BEGIN -+-+-+-+- */
			HAL.Lan.takeSemaphoreMutex(portMAX_DELAY);
		}

		_ftp.changeWorkDir(dir.c_str());
		if (!_ftp.isConnected()) {
			_ftp.closeConnection();
			result = false;
		}

		if (HAL.getUseLinkLayer() == DATALINK_LAYER::ETHERNET) {
			HAL.Lan.giveSemaphoreMutex();
			/* -+-+-+-+- Mutex block END -+-+-+-+- */
		}
	}

	if (result == false) {
		debugMessage("txFTP", "Warning", "Failed CWD", "\n");
		debugMessage("txFTP", "Warning", dir.c_str(), "\n");
		return false;
	}
	result = true;

	{
		if (HAL.getUseLinkLayer() == DATALINK_LAYER::ETHERNET) {
			/* -+-+-+-+- Mutex block BEGIN -+-+-+-+- */
			HAL.Lan.takeSemaphoreMutex(portMAX_DELAY);
		}

		_ftp.initFile("Type A");
		if (!_ftp.isConnected()) {
			_ftp.closeConnection();
			result = false;
		}

		if (HAL.getUseLinkLayer() == DATALINK_LAYER::ETHERNET) {
			HAL.Lan.giveSemaphoreMutex();
			/* -+-+-+-+- Mutex block END -+-+-+-+- */
		}
	}

	if (result == false) {
		debugMessage("txFTP", "Warning", "Failed Init Type", "\n");
		return false;
	}
	result = true;

	{
		if (HAL.getUseLinkLayer() == DATALINK_LAYER::ETHERNET) {
			/* -+-+-+-+- Mutex block BEGIN -+-+-+-+- */
			HAL.Lan.takeSemaphoreMutex(portMAX_DELAY);
		}

		_ftp.clearBuffer();
		if (create_new) {
			// Create new file
			_ftp.newFile(file_name.c_str());
			if (!_ftp.isConnected()) {
				_ftp.closeConnection();
				result = false;
			}
		} else {
			// Append to file
			_ftp.appendFile((char *)file_name.c_str());
			if (!_ftp.isConnected()) {
				_ftp.closeConnection();
				result = false;
			}
		}

		if (HAL.getUseLinkLayer() == DATALINK_LAYER::ETHERNET) {
			HAL.Lan.giveSemaphoreMutex();
			/* -+-+-+-+- Mutex block END -+-+-+-+- */
		}
	}

	if (result == false) {
		if (create_new) {
			debugMessage("txFTP", "Warning", "Failed New Create", "\n");
		} else {
			debugMessage("txFTP", "Warning", "Failed APPE", "\n");
		}
		return false;
	}
	result = true;

	{
		if (HAL.getUseLinkLayer() == DATALINK_LAYER::ETHERNET) {
			/* -+-+-+-+- Mutex block BEGIN -+-+-+-+- */
			HAL.Lan.takeSemaphoreMutex(portMAX_DELAY);
		}

		_ftp.write(value.c_str());
		if (!_ftp.isConnected()) {
			_ftp.closeConnection();
			result = false;
		}

		if (HAL.getUseLinkLayer() == DATALINK_LAYER::ETHERNET) {
			HAL.Lan.giveSemaphoreMutex();
			/* -+-+-+-+- Mutex block END -+-+-+-+- */
		}
	}

	if (result == false) {
		debugMessage("txFTP", "Warning", "Failed Write", "\n");
		return false;
	}
	result = true;

	{
		if (HAL.getUseLinkLayer() == DATALINK_LAYER::ETHERNET) {
			/* -+-+-+-+- Mutex block BEGIN -+-+-+-+- */
			HAL.Lan.takeSemaphoreMutex(portMAX_DELAY);
		}

		_ftp.closeFile();
		_ftp.closeConnection();

		if (HAL.getUseLinkLayer() == DATALINK_LAYER::ETHERNET) {
			HAL.Lan.giveSemaphoreMutex();
			/* -+-+-+-+- Mutex block END -+-+-+-+- */
		}
	}

	return true;
}

bool IoTTxFTP::uploadImage(String dir, String file_name, std::vector<uint8_t> jpgImage) {
	bool result = true;

	{
		if (HAL.getUseLinkLayer() == DATALINK_LAYER::ETHERNET) {
			/* -+-+-+-+- Mutex block BEGIN -+-+-+-+- */
			HAL.Lan.takeSemaphoreMutex(portMAX_DELAY);
		}

		_ftp.openConnection();
		if (!_ftp.isConnected()) {
			_ftp.closeConnection();
			result = false;
		}

		if (HAL.getUseLinkLayer() == DATALINK_LAYER::ETHERNET) {
			HAL.Lan.giveSemaphoreMutex();
			/* -+-+-+-+- Mutex block END -+-+-+-+- */
		}
	}

	if (result == false) {
		debugMessage("uploadImage", "Warning", "Failed OpenConnection", "\n");
		return false;
	}
	result = true;

	{
		if (HAL.getUseLinkLayer() == DATALINK_LAYER::ETHERNET) {
			/* -+-+-+-+- Mutex block BEGIN -+-+-+-+- */
			HAL.Lan.takeSemaphoreMutex(portMAX_DELAY);
		}

		_ftp.changeWorkDir(dir.c_str());
		if (!_ftp.isConnected()) {
			_ftp.closeConnection();
			result = false;
		}

		if (HAL.getUseLinkLayer() == DATALINK_LAYER::ETHERNET) {
			HAL.Lan.giveSemaphoreMutex();
			/* -+-+-+-+- Mutex block END -+-+-+-+- */
		}
	}

	if (result == false) {
		debugMessage("uploadImage", "Warning", "Failed CWD", "\n");
		debugMessage("uploadImage", "Warning", dir.c_str(), "\n");
		return false;
	}
	result = true;

	{
		if (HAL.getUseLinkLayer() == DATALINK_LAYER::ETHERNET) {
			/* -+-+-+-+- Mutex block BEGIN -+-+-+-+- */
			HAL.Lan.takeSemaphoreMutex(portMAX_DELAY);
		}

		_ftp.initFile("Type I");
		if (!_ftp.isConnected()) {
			_ftp.closeConnection();
			result = false;
		}

		if (HAL.getUseLinkLayer() == DATALINK_LAYER::ETHERNET) {
			HAL.Lan.giveSemaphoreMutex();
			/* -+-+-+-+- Mutex block END -+-+-+-+- */
		}
	}

	if (result == false) {
		debugMessage("uploadImage", "Warning", "Failed Init Type", "\n");
		return false;
	}
	result = true;

	{
		if (HAL.getUseLinkLayer() == DATALINK_LAYER::ETHERNET) {
			/* -+-+-+-+- Mutex block BEGIN -+-+-+-+- */
			HAL.Lan.takeSemaphoreMutex(portMAX_DELAY);
		}

		_ftp.clearBuffer();
		// Create new file
		_ftp.newFile(file_name.c_str());
		if (!_ftp.isConnected()) {
			_ftp.closeConnection();
			result = false;
		}

		if (HAL.getUseLinkLayer() == DATALINK_LAYER::ETHERNET) {
			HAL.Lan.giveSemaphoreMutex();
			/* -+-+-+-+- Mutex block END -+-+-+-+- */
		}
	}

	if (result == false) {
		debugMessage("uploadImage", "Warning", "Failed New Create", "\n");
		debugMessage("uploadImage", "Warning", dir.c_str(), "\n");
		debugMessage("uploadImage", "Warning", file_name.c_str(), "\n");
		return false;
	}
	result = true;

	{
		if (HAL.getUseLinkLayer() == DATALINK_LAYER::ETHERNET) {
			/* -+-+-+-+- Mutex block BEGIN -+-+-+-+- */
			HAL.Lan.takeSemaphoreMutex(portMAX_DELAY);
		}

		_ftp.writeData((uint8_t *)(&jpgImage[0]), jpgImage.size());
		if (!_ftp.isConnected()) {
			_ftp.closeConnection();
			result = false;
		}

		if (HAL.getUseLinkLayer() == DATALINK_LAYER::ETHERNET) {
			HAL.Lan.giveSemaphoreMutex();
			/* -+-+-+-+- Mutex block END -+-+-+-+- */
		}
	}

	if (result == false) {
		debugMessage("uploadImage", "Warning", "Failed Write", "\n");
		return false;
	}
	result = true;

	{
		if (HAL.getUseLinkLayer() == DATALINK_LAYER::ETHERNET) {
			/* -+-+-+-+- Mutex block BEGIN -+-+-+-+- */
			HAL.Lan.takeSemaphoreMutex(portMAX_DELAY);
		}

		_ftp.closeFile();
		_ftp.closeConnection();

		if (HAL.getUseLinkLayer() == DATALINK_LAYER::ETHERNET) {
			HAL.Lan.giveSemaphoreMutex();
			/* -+-+-+-+- Mutex block END -+-+-+-+- */
		}
	}

	return true;
}

/**********************************
 * Protected functions
 **********************************/
bool IoTTxFTP::makeDirRecursive(String absolute_path) {
	bool result = true;

	{
		if (HAL.getUseLinkLayer() == DATALINK_LAYER::ETHERNET) {
			/* -+-+-+-+- Mutex block BEGIN -+-+-+-+- */
			HAL.Lan.takeSemaphoreMutex(portMAX_DELAY);
		}

		_ftp.openConnection();
		if (!_ftp.isConnected()) {
			_ftp.closeConnection();
			result = false;
		}

		if (HAL.getUseLinkLayer() == DATALINK_LAYER::ETHERNET) {
			HAL.Lan.giveSemaphoreMutex();
			/* -+-+-+-+- Mutex block END -+-+-+-+- */
		}
	}

	if (result == false) {
		debugMessage("FTP", "Warning", "Failed OpenConnection", "\n");
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
			if (HAL.getUseLinkLayer() == DATALINK_LAYER::ETHERNET) {
				/* -+-+-+-+- Mutex block BEGIN -+-+-+-+- */
				HAL.Lan.takeSemaphoreMutex(portMAX_DELAY);
			}

			_ftp.changeWorkDir(d.c_str());
			rc = _ftp.getLastResponseCode();

			if (HAL.getUseLinkLayer() == DATALINK_LAYER::ETHERNET) {
				HAL.Lan.giveSemaphoreMutex();
				/* -+-+-+-+- Mutex block END -+-+-+-+- */
			}
		}

		if (rc >= 400) {
			String tmp = "Try to create directory: ";
			tmp += exist_dir_path + d;
			debugMessage("FTP", "Debug", tmp.c_str(), "\n");

			{
				if (HAL.getUseLinkLayer() == DATALINK_LAYER::ETHERNET) {
					/* -+-+-+-+- Mutex block BEGIN -+-+-+-+- */
					HAL.Lan.takeSemaphoreMutex(portMAX_DELAY);
				}

				// Re-open _ftp.connection
				_ftp.openConnection();
				_ftp.changeWorkDir(exist_dir_path.c_str());
				_ftp.makeDir(d.c_str());
				if (!_ftp.isConnected()) {
					_ftp.closeConnection();
					result = false;
				}

				if (HAL.getUseLinkLayer() == DATALINK_LAYER::ETHERNET) {
					HAL.Lan.giveSemaphoreMutex();
					/* -+-+-+-+- Mutex block END -+-+-+-+- */
				}
			}

			if (result == false) {
				debugMessage("FTP", "Warning", "Failed MakeDir", "\n");
				return false;
			}
			result = true;

			{
				if (HAL.getUseLinkLayer() == DATALINK_LAYER::ETHERNET) {
					/* -+-+-+-+- Mutex block BEGIN -+-+-+-+- */
					HAL.Lan.takeSemaphoreMutex(portMAX_DELAY);
				}

				_ftp.changeWorkDir(d.c_str());
				if (!_ftp.isConnected()) {
					_ftp.closeConnection();
					result = false;
				}

				if (HAL.getUseLinkLayer() == DATALINK_LAYER::ETHERNET) {
					HAL.Lan.giveSemaphoreMutex();
					/* -+-+-+-+- Mutex block END -+-+-+-+- */
				}
			}

			if (result == false) {
				debugMessage("FTP", "Warning", "Failed ChangeWorkDir in loop", "\n");
				return false;
			}
			result = true;

			tmp = "Success to create directory!: ";
			tmp += exist_dir_path + d;
			debugMessage("FTP", "Debug", tmp.c_str(), "\n");
		}

		exist_dir_path += d + "/";
		absolute_path = absolute_path.substring(sp, absolute_path.length());
	}

	{
		if (HAL.getUseLinkLayer() == DATALINK_LAYER::ETHERNET) {
			/* -+-+-+-+- Mutex block BEGIN -+-+-+-+- */
			HAL.Lan.takeSemaphoreMutex(portMAX_DELAY);
		}

		// _ftp.closeFile();
		_ftp.closeConnection();

		if (HAL.getUseLinkLayer() == DATALINK_LAYER::ETHERNET) {
			HAL.Lan.giveSemaphoreMutex();
			/* -+-+-+-+- Mutex block END -+-+-+-+- */
		}
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
	if (!p1.endsWith("/") && !p2.startsWith("/")) {
		p1.concat("/");
	}
	p1.concat(p2);
	return p1;
}

IoTTxFTP::FileExist IoTTxFTP::exist(String fpath) {
	IoTTxFTP::FileExist result = FE_NOT_EXIST;
	String ret                 = "";

	{
		if (HAL.getUseLinkLayer() == DATALINK_LAYER::ETHERNET) {
			/* -+-+-+-+- Mutex block BEGIN -+-+-+-+- */
			HAL.Lan.takeSemaphoreMutex(portMAX_DELAY);
		}

		_ftp.openConnection();
		if (!_ftp.isConnected()) {
			result = FE_FTP_ERROR;
		}

		if (HAL.getUseLinkLayer() == DATALINK_LAYER::ETHERNET) {
			HAL.Lan.giveSemaphoreMutex();
			/* -+-+-+-+- Mutex block END -+-+-+-+- */
		}
	}

	if (result == FE_FTP_ERROR) {
		return FE_FTP_ERROR;
	}

	{
		if (HAL.getUseLinkLayer() == DATALINK_LAYER::ETHERNET) {
			/* -+-+-+-+- Mutex block BEGIN -+-+-+-+- */
			HAL.Lan.takeSemaphoreMutex(portMAX_DELAY);
		}

		_ftp.renameFile(fpath.c_str(), fpath.c_str());
		uint16_t rc = _ftp.getLastResponseCode();

		if (200 <= rc && rc < 300) {
			result = FE_EXIST;
		} else if (rc == 503) {
			result = FE_NOT_EXIST;
		} else {
			result = FE_FTP_ERROR;
		}
		_ftp.closeConnection();

		if (HAL.getUseLinkLayer() == DATALINK_LAYER::ETHERNET) {
			HAL.Lan.giveSemaphoreMutex();
			/* -+-+-+-+- Mutex block END -+-+-+-+- */
		}
	}
	return result;
}

bool IoTTxFTP::transmitHeartbeat() {
	static struct Penalty _penalty_wait = {false, 0, 0};

	if (_penalty_wait.enable) {
		if ((millis() - _penalty_wait.last_failed_millis) > _penalty_wait.wait_ms) {
			_penalty_wait.enable = false;
		}
		return false;
	}

	if (!_heartbeat_store) {
		return false;
	}

	switch (HAL.getUseLinkLayer()) {
	case DATALINK_LAYER::WIFI:
		if (!HAL.Wlan.connectWlan()) {
			_penalty_wait.enable             = true;     // Enable wait
			_penalty_wait.last_failed_millis = millis(); // Update last failed time
			_penalty_wait.wait_ms            = 5000;     // Wait interval time 5sec
			return false;
		}
		break;
	case DATALINK_LAYER::ETHERNET:
		if (HAL.Lan.isLinkUp() == false) {
			_penalty_wait.enable             = true;     // Enable wait
			_penalty_wait.last_failed_millis = millis(); // Update last failed time
			_penalty_wait.wait_ms            = 5000;     // Wait interval time 5sec
			return false;
		}
		break;
	default:
		break;
	}

	if (!makeDirRecursive(_heartbeat_current_dir)) {
		debugMessage("FTP HB", "Warning", "Failed to create directory -> ");
		debugMessage("FTP HB", "Path", _heartbeat_current_dir.c_str(), "\n");
		// WiFi disconnect
		// disconnectWiFi();
		return false;
	}

	char s_day[64] = {'\0'};
	strftime(s_day, 64, "%Y/%m/%d %H:%M:%S", &_heartbeat_tm);
	char buffer[256] = {'\0'};
	int res          = snprintf(buffer, 256, "%s\n", s_day);
	bool success     = false;
	if (res > 0) {
		success = txFtp(_heartbeat_current_dir, _heartbeat_file_name, buffer, true);
	}
	if (success) {
		_heartbeat_store = false;
	} else {
		debugMessage("FTP HB", "Warning", "Failed to write heartbeat", "\n");
		// disconnectWiFi();
		_penalty_wait.enable             = true;     // Enable wait
		_penalty_wait.last_failed_millis = millis(); // Update last failed time
		_penalty_wait.wait_ms            = 1000;     // Wait interval time 1sec
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
	static struct Penalty _penalty_wait = {false, 0, 0};
	String prev_dir                     = "";
	enum IoTTx_Tx_Result result         = RESULT_TX_ERROR;

	if (_penalty_wait.enable) {
		if ((millis() - _penalty_wait.last_failed_millis) > _penalty_wait.wait_ms) {
			_penalty_wait.enable = false;
		}
		return result;
	}

	switch (HAL.getUseLinkLayer()) {
	case DATALINK_LAYER::WIFI:
		if (!HAL.Wlan.connectWlan()) {
			_penalty_wait.enable             = true;     // Enable wait
			_penalty_wait.last_failed_millis = millis(); // Update last failed time
			_penalty_wait.wait_ms            = 5000;     // Wait interval time 5sec
			return result;
		}
		break;
	case DATALINK_LAYER::ETHERNET:
		if (HAL.Lan.isLinkUp() == false) {
			_penalty_wait.enable             = true;     // Enable wait
			_penalty_wait.last_failed_millis = millis(); // Update last failed time
			_penalty_wait.wait_ms            = 5000;     // Wait interval time 5sec
			return result;
		}
		break;
	default:
		break;
	}

	while (_tx_idx.store != _tx_idx.transmit) {
		while (_mutex) {
			HAL.delay(1);
		}
		_mutex           = true;
		char dirname[64] = {'\0'};
		strftime(dirname, 64, "%Y%m%d", &_buffer[_tx_idx.transmit].time);
		_mutex          = false;
		String save_dir = _current_dir + String(dirname);

		if (!prev_dir.equals(save_dir)) {
			// Create directory and csv header
			if (!makeDirRecursive(save_dir)) {
				debugMessage("FTP", "Warning", "Failed to create directory\n");
				debugMessage("FTP", "Path", save_dir.c_str(), "\n");
				_failed_counter++;
				_penalty_wait.enable             = true;     // Enable wait
				_penalty_wait.last_failed_millis = millis(); // Update last failed time
				if (_failed_counter >= 5) {
					_failed_counter       = 0;
					_penalty_wait.wait_ms = 5000; // Wait interval time 5sec
				} else {
					_penalty_wait.wait_ms = 1000; // Wait interval time 1sec
				}
				return result;
			}

			IoTTxFTP::FileExist fe = exist(joinPath(save_dir, _file_name));
			if (fe == FE_FTP_ERROR) {
				debugMessage("FTP", "Warning", "Failed to check file exist", "\n");
				_failed_counter++;
				_penalty_wait.enable             = true;     // Enable wait
				_penalty_wait.last_failed_millis = millis(); // Update last failed time
				if (_failed_counter >= 5) {
					_failed_counter       = 0;
					_penalty_wait.wait_ms = 5000; // Wait interval time 5sec
				} else {
					_penalty_wait.wait_ms = 1000; // Wait interval time 1sec
				}
				return result;
			}
			if (fe == FE_NOT_EXIST) {
				_file_not_exist_count++;
				if (_file_not_exist_count >= 3) {
					debugMessage("FTP", "Info", "Create new file", "\n");
					String header = "day, value, devID, sensName, unit\n";
					if (_unix_time_field.enable) {
						header = "time, " + header;
					}
					txFtp(save_dir, _file_name, header.c_str(), true);
					_file_not_exist_count = 0;
				} else {
					_penalty_wait.enable             = true;     // Enable wait
					_penalty_wait.last_failed_millis = millis(); // Update last failed time
					_penalty_wait.wait_ms            = 500;      // Wait interval time 500msec
					return result;
				}
			}
			_file_not_exist_count = 0;
			prev_dir              = String(save_dir);
		}

		while (_mutex) {
			HAL.delay(1);
		}
		_mutex         = true;
		char s_day[64] = {'\0'};
		strftime(s_day, 64, "%Y/%m/%d %H:%M:%S", &_buffer[_tx_idx.transmit].time);

		String value     = _buffer[_tx_idx.transmit].value;
		String name      = _buffer[_tx_idx.transmit].name;
		String unit      = _buffer[_tx_idx.transmit].unit;
		String unix_time = "";
		if (_unix_time_field.enable) {
			signed long long ts = mktime(&_buffer[_tx_idx.transmit].time);
			_mutex              = false;
			if (ts != -1) {
				String s  = lld2String(ts);
				String p  = _unix_time_field.nano_precision ? "000000000" : "";
				unix_time = s + p;
			}
		}
		_mutex = false;

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
			while (_mutex) {
				HAL.delay(1);
			}
			_mutex                          = true;
			_buffer[_tx_idx.transmit].value = "";
			_buffer[_tx_idx.transmit].name  = "";
			_buffer[_tx_idx.transmit].unit  = "";

			_tx_idx.transmit    = ((_tx_idx.transmit + 1) == IOTTX_BUFFER_SIZE) ? 0 : (_tx_idx.transmit + 1);
			_pending_data_count = (((_pending_data_count - 1) > 0) ? (_pending_data_count - 1) : 0);
			_mutex              = false;
			result              = RESULT_TX_SOME_DATA;
			// printIndex("FTP:", "\n");
		} else {
			debugMessage("FTP", "Error", "Failed to write data", "\n");
			_failed_counter++;
			if (_failed_counter >= 5) {
				_failed_counter = 0;
			}

			_penalty_wait.enable             = true;     // Enable wait
			_penalty_wait.last_failed_millis = millis(); // Update last failed time
			_penalty_wait.wait_ms            = 5000;     // Wait interval time 5sec
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
	static struct Penalty _penalty_wait = {false, 0, 0};
	String prev_dir                     = "";
	enum IoTTx_Tx_Result result         = RESULT_TX_ERROR;

	if (_penalty_wait.enable) {
		if ((millis() - _penalty_wait.last_failed_millis) > _penalty_wait.wait_ms) {
			_penalty_wait.enable = false;
		}
		return result;
	}

	switch (HAL.getUseLinkLayer()) {
	case DATALINK_LAYER::WIFI:
		if (!HAL.Wlan.connectWlan()) {
			_penalty_wait.enable             = true;     // Enable wait
			_penalty_wait.last_failed_millis = millis(); // Update last failed time
			_penalty_wait.wait_ms            = 5000;     // Wait interval time 5sec
			return result;
		}
		break;
	case DATALINK_LAYER::ETHERNET:
		if (HAL.Lan.isLinkUp() == false) {
			_penalty_wait.enable             = true;     // Enable wait
			_penalty_wait.last_failed_millis = millis(); // Update last failed time
			_penalty_wait.wait_ms            = 5000;     // Wait interval time 5sec
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

		while (_mutex) {
			HAL.delay(1);
		}
		_mutex           = true;
		char dirname[64] = {'\0'};
		strftime(dirname, 64, "%Y%m%d", &_ex_buffer_info[storage].time);
		_mutex          = false;
		String save_dir = _current_dir;

		if (storage == 0) {
			// ex) Camera/00_00/20201210
			save_dir += "Camera/" + _devID + "/" + String(dirname);
		}

		if (!prev_dir.equals(save_dir)) {
			// Create directory and csv header
			if (!makeDirRecursive(save_dir)) {
				debugMessage("FTP Ex", "Warning", "Failed to create directory\n");
				debugMessage("FTP Ex", "Path", save_dir.c_str(), "\n");
				_failed_counter++;
				_penalty_wait.enable             = true;     // Enable wait
				_penalty_wait.last_failed_millis = millis(); // Update last failed time
				if (_failed_counter >= 5) {
					_failed_counter       = 0;
					_penalty_wait.wait_ms = 5000; // Wait interval time 5sec
				} else {
					_penalty_wait.wait_ms = 1000; // Wait interval time 1sec
				}
				return result;
			}
			prev_dir = String(save_dir);
		}

		while (_mutex) {
			HAL.delay(1);
		}
		_mutex = true;

		bool success = false;
		if (storage == 0) {
			char fname[64] = {'\0'};
			strftime(fname, 64, "%Y%m%d_%H%M%S.jpg", &_ex_buffer_info[storage].time);
			_mutex  = false;
			success = uploadImage(save_dir, fname, HAL.Camera.jpgImage);
		}
		_mutex = false;

		if (success) {
			while (_mutex) {
				HAL.delay(1);
			}
			_mutex                           = true;
			_ex_buffer_info[storage].updated = false;
			_mutex                           = false;
			result                           = RESULT_TX_SOME_DATA;
			// printIndex("FTP:", "\n");
		} else {
			debugMessage("FTP Ex", "Error", "Failed to write data", "\n");
			_failed_counter++;
			if (_failed_counter >= 5) {
				_failed_counter = 0;
			}

			_penalty_wait.enable             = true;     // Enable wait
			_penalty_wait.last_failed_millis = millis(); // Update last failed time
			_penalty_wait.wait_ms            = 5000;     // Wait interval time 5sec
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
