#include "Arduino_FTPClient.h"

Arduino_FTPClient::Arduino_FTPClient() {
	port    = 21;
	verbose = -1;
}

Arduino_FTPClient::Arduino_FTPClient(char *_serverAddress, uint16_t _port, char *_userName, char *_passWord, uint16_t _timeout, uint8_t _verbose) {
	userName      = _userName;
	passWord      = _passWord;
	serverAddress = _serverAddress;
	port          = _port;
	timeout       = _timeout;
	verbose       = _verbose;
}

Arduino_FTPClient::Arduino_FTPClient(char *_serverAddress, char *_userName, char *_passWord, uint16_t _timeout, uint8_t _verbose) {
	userName      = _userName;
	passWord      = _passWord;
	serverAddress = _serverAddress;
	port          = 21;
	timeout       = _timeout;
	verbose       = _verbose;
}

void Arduino_FTPClient::setClient(Client &_client) {
	this->client = &_client;
}

void Arduino_FTPClient::setDataClient(Client &_dclient) {
	this->dclient = &_dclient;
}

void Arduino_FTPClient::setClients(Client &_client, Client &_dclient) {
	this->client  = &_client;
	this->dclient = &_dclient;
}

void Arduino_FTPClient::setPort(uint16_t _port) {
	port = _port;
}

uint16_t Arduino_FTPClient::getPort() {
	return this->port;
}

void Arduino_FTPClient::setServerAddress(String _serverAddress) {
	serverAddress = _serverAddress;
}

String Arduino_FTPClient::getServerAddress() {
	return this->serverAddress;
}

void Arduino_FTPClient::setAccount(String _userName, String _password) {
	this->userName = _userName;
	this->passWord = _password;
}

void Arduino_FTPClient::setTimeout(uint16_t _timeout) {
	this->timeout = _timeout;
	this->client->setTimeout(_timeout);
}

void Arduino_FTPClient::setVerbose(uint8_t _verbose) {
	this->verbose = _verbose;
}

Client *Arduino_FTPClient::getDataClient() {
	return dclient;
}

bool Arduino_FTPClient::isConnected() {
	if (!_isConnected) {
		FTPerr("FTP error: ");
		FTPerr(outBuf);
		FTPerr("\n");
	}

	return _isConnected;
}

void Arduino_FTPClient::getLastModifiedTime(const char *fileName, char *result) {
	FTPdbgn("Send MDTM");
	if (!isConnected()) {
		return;
	}
	client->print(F("MDTM "));
	client->println(F(fileName));
	getFTPAnswer(result, 4);
}

void Arduino_FTPClient::writeClientBuffered(Client *cli, unsigned char *data, int dataLength) {
	if (!isConnected()) {
		return;
	}

	size_t clientCount = 0;
	for (int i = 0; i < dataLength; i++) {
		clientBuf[clientCount] = data[i];
		// client->write(data[i])
		clientCount++;
		if (clientCount > bufferSize - 1) {
			cli->write(clientBuf, bufferSize);
			clientCount = 0;
		}
	}
	if (clientCount > 0) {
		cli->write(clientBuf, clientCount);
	}
}

void Arduino_FTPClient::getFTPAnswer(char *result, int offsetStart) {
	char thisByte;

	unsigned long _m = millis();
	while (!client->available() && millis() < _m + timeout) {
		delay(1);
	}

	if (!client->available()) {
		outCount = 0;
		memset(outBuf, 0, sizeof(outBuf));
		strcpy(outBuf, "Offline");

		_isConnected = false;
		isConnected();
		return;
	}

	while (client->available()) {
		thisByte = client->read();
		if (outCount < sizeof(outBuf)) {
			outBuf[outCount] = thisByte;
			outCount++;
			outBuf[outCount] = 0;
		}
		if (thisByte == '\n') {
			_lastResponseCode = atoi(outBuf);
			outCount          = 0;
			break;
		}
	}

	if (400 <= _lastResponseCode) {
		_isConnected = false;
		isConnected();
		return;
	} else {
		_isConnected = true;
	}

	if (result != NULL) {
		FTPdbgn("Result start");
		// Deprecated
		for (int i = offsetStart; i < sizeof(outBuf); i++) {
			result[i] = outBuf[i - offsetStart];
		}
		FTPdbg("Result: ");
		// Serial.write(result);
		FTPdbg(outBuf);
		FTPdbgn("Result end");
	}
}

void Arduino_FTPClient::writeData(unsigned char *data, int dataLength) {
	FTPdbgn(F("Writing"));
	if (!isConnected()) {
		return;
	}
	writeClientBuffered(dclient, &data[0], dataLength);
}

void Arduino_FTPClient::closeFile() {
	FTPdbgn(F("Close File"));
	dclient->stop();

	if (!_isConnected) {
		return;
	}

	getFTPAnswer();
}

void Arduino_FTPClient::write(const char *str) {
	FTPdbgn(F("Write File"));
	if (!isConnected()) {
		return;
	}

	getDataClient()->print(str);
}

void Arduino_FTPClient::closeConnection() {
	client->println(F("QUIT"));
	client->stop();
	FTPdbgn(F("Connection closed"));
}

void Arduino_FTPClient::openConnection() {
	FTPdbg(F("Connecting to: "));
	FTPdbgn(serverAddress.c_str());
	if (client->connect(serverAddress.c_str(), port)) {
		FTPdbgn(F("Command connected"));
	}

	// clear buffer
	while (client->available()) {
		client->read();
	}

	FTPdbgn("Send USER");
	client->print(F("USER "));
	client->println(F(userName.c_str()));
	getFTPAnswer();
	while (client->available()) {
		getFTPAnswer();
	}

	FTPdbgn("Send PASSWORD");
	client->print(F("PASS "));
	client->println(F(passWord.c_str()));
	getFTPAnswer();
	while (client->available()) {
		getFTPAnswer();
	}

	FTPdbgn("Send SYST");
	client->println(F("SYST"));
	getFTPAnswer();
	while (client->available()) {
		getFTPAnswer();
	}
}

void Arduino_FTPClient::renameFile(const char *from, const char *to) {
	FTPdbgn("Send RNFR");
	if (!isConnected()) {
		return;
	}
	client->print(F("RNFR "));
	client->println(F(from));
	getFTPAnswer();

	FTPdbgn("Send RNTO");
	client->print(F("RNTO "));
	client->println(F(to));
	getFTPAnswer();
}

void Arduino_FTPClient::newFile(const char *fileName) {
	FTPdbgn("Send STOR");
	if (!isConnected()) {
		return;
	}
	client->print(F("STOR "));
	client->println(F(fileName));
	getFTPAnswer();
}

void Arduino_FTPClient::initFile(const char *type) {
	FTPdbgn("Send TYPE");
	if (!isConnected()) {
		return;
	}
	FTPdbgn(type);
	client->println(F(type));
	getFTPAnswer();

	FTPdbgn("Send PASV");
	client->println(F("PASV"));
	getFTPAnswer();

	if (isConnected() && getLastResponseCode() != 227) {
		getFTPAnswer();
	}

	char *tStr = strtok(outBuf, "(,");
	int array_pasv[6];
	for (int i = 0; i < 6; i++) {
		tStr = strtok(NULL, "(,");
		if (tStr == NULL) {
			FTPdbgn(F("Bad PASV Answer"));
			closeConnection();
			return;
		}
		array_pasv[i] = atoi(tStr);
	}
	unsigned int hiPort, loPort;
	hiPort = array_pasv[4] << 8;
	loPort = array_pasv[5] & 255;

	IPAddress pasvServer(array_pasv[0], array_pasv[1], array_pasv[2], array_pasv[3]);

	FTPdbg(F("Data port: "));
	hiPort = hiPort | loPort;
	FTPdbgn(hiPort);
	if (dclient->connect(pasvServer, hiPort)) {
		FTPdbgn(F("Data connection established"));
	}
}

void Arduino_FTPClient::appendFile(char *fileName) {
	FTPdbgn("Send APPE");
	if (!isConnected()) {
		return;
	}
	client->print(F("APPE "));
	client->println(F(fileName));
	getFTPAnswer();
}

void Arduino_FTPClient::changeWorkDir(const char *dir) {
	FTPdbgn("Send CWD");
	if (!isConnected()) {
		return;
	}
	client->print(F("CWD "));
	client->println(F(dir));
	getFTPAnswer();
}

void Arduino_FTPClient::deleteFile(const char *file) {
	FTPdbgn("Send DELE");
	if (!isConnected()) {
		return;
	}
	client->print(F("DELE "));
	client->println(F(file));
	getFTPAnswer();
}

void Arduino_FTPClient::makeDir(const char *dir) {
	FTPdbgn("Send MKD");
	if (!isConnected()) {
		return;
	}
	client->print(F("MKD "));
	client->println(F(dir));
	getFTPAnswer();
}

void Arduino_FTPClient::contentList(const char *dir, String *list) {
	char _resp[sizeof(outBuf)];
	uint16_t _b = 0;

	FTPdbgn("Send MLSD");
	if (!isConnected()) {
		return;
	}
	client->print(F("MLSD"));
	client->println(F(dir));
	getFTPAnswer(_resp);

	// Convert char array to string to manipulate and find response size
	// each server reports it differently, TODO = FEAT
	// String resp_string = _resp;
	// resp_string.substring(resp_string.lastIndexOf('matches')-9);
	// FTPdbgn(resp_string);

	unsigned long _m = millis();
	while (!dclient->available() && millis() < _m + timeout) {
		delay(1);
	}

	while (dclient->available()) {
		if (_b < 128) {
			list[_b] = dclient->readStringUntil('\n');
			// FTPdbgn(String(_b) + ":" + list[_b]);
			_b++;
		} else {
			// too many files
			break;
			;
		}
	}

	clearBuffer(false);
}

void Arduino_FTPClient::contentListWithListCommand(const char *dir, String *list) {
	char _resp[sizeof(outBuf)];
	uint16_t _b = 0;

	FTPdbgn("Send LIST");
	if (!isConnected()) {
		return;
	}
	client->print(F("LIST"));
	client->println(F(dir));
	getFTPAnswer(_resp);

	// Convert char array to string to manipulate and find response size
	// each server reports it differently, TODO = FEAT
	// String resp_string = _resp;
	// resp_string.substring(resp_string.lastIndexOf('matches')-9);
	// FTPdbgn(resp_string);

	unsigned long _m = millis();
	while (!dclient->available() && millis() < _m + timeout) {
		delay(1);
	}

	while (dclient->available()) {
		if (_b < 128) {
			String tmp = dclient->readStringUntil('\n');
			list[_b]   = tmp.substring(tmp.lastIndexOf(" ") + 1, tmp.length());
			// FTPdbgn(String(_b) + ":" + tmp);
			_b++;
		} else {
			// too many files
			break;
		}
	}

	clearBuffer(false);
}

void Arduino_FTPClient::downloadString(const char *filename, String &str) {
	FTPdbgn("Send RETR");
	if (!isConnected()) {
		return;
	}

	client->print(F("RETR "));
	client->println(F(filename));

	char _resp[sizeof(outBuf)];
	getFTPAnswer(_resp);

	unsigned long _m = millis();
	while (!getDataClient()->available() && millis() < _m + timeout) {
		delay(1);
	}

	while (getDataClient()->available()) {
		str += getDataClient()->readString();
	}
}

void Arduino_FTPClient::downloadFile(const char *filename, unsigned char *buf, size_t length, bool printUART) {
	FTPdbgn("Send RETR");
	if (!isConnected()) {
		return;
	}
	client->print(F("RETR "));
	client->println(F(filename));

	char _resp[sizeof(outBuf)];
	getFTPAnswer(_resp);

	char _buf[2];

	unsigned long _m = millis();
	while (!dclient->available() && millis() < _m + timeout) {
		delay(1);
	}

	while (dclient->available()) {
		if (!printUART) {
			dclient->readBytes(buf, length);
		} else {
			for (size_t _b = 0; _b < length; _b++) {
				dclient->readBytes(_buf, 1),
				    Serial.print(_buf[0], HEX);
			}
		}
	}
}

void Arduino_FTPClient::getFileStatus(const char *fpath) {
	String _result = "";
	getFileStatus(fpath, &_result);
	Serial.print(_result);
}

void Arduino_FTPClient::getFileStatus(const char *fpath, String *result) {
	FTPdbgn("Send STAT");
	if (!isConnected()) {
		return;
	}
	client->print(F("STAT "));
	client->println(F(fpath));

	char _resp[sizeof(outBuf)];

	*result          = "";
	bool _isComplete = false;
	uint8_t count    = 0;
	while (isConnected() && _isComplete == false) {
		getFTPAnswer(_resp);
		*result += _resp;
		if (getLastResponseCode() == 213) {
			count++;
			if (count != 2) {
				_lastResponseCode = 0;
			} else {
				_isComplete = true;
			}
		}
	}

	if (_isComplete == false) {
		Serial.println("STAT Failed BEGIN ~~~");
		Serial.println(*result);
		Serial.println("STAT Failed END ~~~");
		*result = "";
		return;
	}
}

uint16_t Arduino_FTPClient::getLastResponseCode() {
	return _lastResponseCode;
}

void Arduino_FTPClient::clearBuffer(bool debug) {
	if (debug) {
		while (client->available()) {
			Serial.printf("%c", client->read());
		}
	} else {
		while (client->available()) {
			client->flush();
		}
	}
}