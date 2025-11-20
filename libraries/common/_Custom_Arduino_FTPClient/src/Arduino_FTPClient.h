#ifndef ARDUINO_FTP_CLIENT_H
#define ARDUINO_FTP_CLIENT_H

#include <Arduino.h>
#include <Client.h>
#include <IPAddress.h>

class Arduino_FTPClient {
private:
	void writeClientBuffered(Client *cli, unsigned char *data, int dataLength);
	char outBuf[128];
	unsigned char outCount;
	Client *client;
	Client *dclient;
	uint8_t verbose;

	template <typename T>
	void FTPdbg(T msg) {
		if (verbose == 2)
			Serial.print(msg);
	}

	template <typename T>
	void FTPdbgn(T msg) {
		if (verbose == 2)
			Serial.println(msg);
	}

	template <typename T>
	void FTPerr(T msg) {
		if (verbose == 1 || verbose == 2)
			Serial.print(msg);
	}

	String userName;
	String passWord;
	String serverAddress;
	uint16_t port;
	bool _isConnected = false;
	unsigned char clientBuf[1500];
	size_t bufferSize          = 1500;
	uint16_t timeout           = 10000;
	uint16_t _lastResponseCode = 0;
	Client *getDataClient();

public:
	Arduino_FTPClient();
	Arduino_FTPClient(char *_serverAddress, uint16_t _port, char *_userName, char *_passWord, uint16_t _timeout = 10000, uint8_t _verbose = 1);
	Arduino_FTPClient(char *_serverAddress, char *_userName, char *_passWord, uint16_t _timeout = 10000, uint8_t _verbose = 1);

	void setClient(Client &_client);
	void setDataClient(Client &_dclient);
	void setClients(Client &_client, Client &_dclient);

	void setPort(uint16_t _port);
	uint16_t getPort();

	void setServerAddress(String _serverAddress);
	String getServerAddress();

	void setAccount(String _userName, String _password);
	void setTimeout(uint16_t _timeout);
	void setVerbose(uint8_t _verbose);

	void openConnection();
	void closeConnection();
	bool isConnected();
	void newFile(const char *fileName);
	void appendFile(char *fileName);
	void writeData(unsigned char *data, int dataLength);
	void closeFile();
	void getFTPAnswer(char *result = NULL, int offsetStart = 0);
	void getLastModifiedTime(const char *fileName, char *result);
	void renameFile(const char *from, const char *to);
	void write(const char *str);
	void initFile(const char *type);
	void changeWorkDir(const char *dir);
	void deleteFile(const char *file);
	void makeDir(const char *dir);
	void contentList(const char *dir, String *list);
	void contentListWithListCommand(const char *dir, String *list);
	void downloadString(const char *filename, String &str);
	void downloadFile(const char *filename, unsigned char *buf, size_t length, bool printUART = false);
	void getFileStatus(const char *fpath);
	void getFileStatus(const char *fpath, String *result);
	uint16_t getLastResponseCode();
	void clearBuffer(bool debug = false);
};

#endif /* ARDUINO_FTP_CLIENT_H */