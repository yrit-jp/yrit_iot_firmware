#ifndef EXTERNALFILESYSTEM_H_
#define EXTERNALFILESYSTEM_H_

#include <Adafruit_LittleFS.h>

class ExternalFileSystem : public Adafruit_LittleFS {
public:
	ExternalFileSystem(void);
	// overwrite to also perform low level format (sector erase of whole flash region)
	bool begin(void);

	void flashDeepSleep();
	void flashWakeUp();
};

extern ExternalFileSystem ExternalFS;

#endif /* EXTERNALFILESYSTEM_H_ */
