/*
 * Reference: https://gist.github.com/ciniml/11b0302c1ea8fce7a360a7ed9511af91
 */

#ifdef WIO_TERMINAL
#ifndef SAMD51TC_H
#define SAMD51TC_H

#include <Arduino.h>

class SAMD51TC {
public:
	/**
     * @brief Create a new instance of SAM51TC for the TCx peripheral where x == tcUnit.
     * @param [in] tcUnit index of TC peripheral.
     */
	SAMD51TC(uint8_t tcUnit);
	void begin(uint64_t microseconds = 1000000);
	void setPeriod(uint32_t microseconds);
	void __attribute__((noinline)) start();
	void stop();
	void restart();
	void attachInterrupt(void (*isrCallback)());
	void detachInterrupt();

private:
	uint8_t tcUnit;
	TcCount16 *regs;
	IRQn_Type irqn;
	uint32_t microseconds;
	void (*isrCallback)();

	void configureClock();

public:
	void processIsr() {
		if (this->regs->INTFLAG.bit.MC0) {
			this->regs->INTFLAG.bit.MC0 = 1;
			if (this->isrCallback != nullptr) {
				this->isrCallback();
			}
		}
	}
};



#endif /* SAMD51TC_H */
#endif /* WIO_TERMINAL */