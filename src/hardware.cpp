#include "hardware.h"

namespace Hardware {


	void print(String _string) {
		#if defined DEBUG_MODE_SERIAL
		Serial.print(_string);
		#endif

		#if defined DEBUG_MODE_LCD
		Lcd::print(_string);
		#endif

		#if defined DEBUG_MODE_TFT
		Tft::print(_string);
		#endif
	}

	extern void println(String _string) {
		#if defined DEBUG_MODE_SERIAL
		Serial.println(_string);
		#endif

		#if defined DEBUG_MODE_LCD
		Lcd::println(_string);
		#endif

		#if defined DEBUG_MODE_TFT
		Tft::println(_string);
		#endif
	}

	// Basic Harware namespace methods
	void blinkLed(uint8_t _pinLed, uint8_t _times) {
		// Non blocking blink! Avoid to use delay() too
		elapsedMicros usec = 0;
		for (uint8_t i=0; i<_times; i++) {

			// Use a loop, or elapsedMicros [see: https://www.pjrc.com/teensy/teensy31.html]
			// for (unsigned long i=0; i<1000000;i++) { digitalWrite(PIN_LED_DEBUG, LOW);}
			// for (unsigned long i=0; i<1000000;i++) { digitalWrite(PIN_LED_DEBUG, HIGH);}

			digitalWrite(_pinLed, HIGH);
			while (usec<500000) {} // half a second
			usec = 0;
			digitalWrite(_pinLed, LOW);
			while (usec<500000) {} // half a second
			usec = 0;
		}
	}

	void blinkLedDebug(uint8_t _times) {
		blinkLed(PIN_LED_DEBUG, _times);
	}
	void blinkLedMessage(uint8_t _times) {
		blinkLed(PIN_LED_MESSAGE, _times);
	}

	void init() {
		//initSerial(); // make a namespace for serial? TODO

		#ifdef DEBUG_MODE_LCD
		Lcd::init();
		#endif

		#ifdef DEBUG_MODE_TFT
		Tft::init();
		#endif

		Gpio::init();
		Lasers::init();
		Scanner::init();

	}

	//Software reset: better than using the RST pin (noisy?)
	void resetBoard() {
		SCB_AIRCR = 0x05FA0004; // software reset on Teensy 3.X
	}


	namespace Gpio {

		void init() { // set default modes, output values, etc of pins, other that the laser and scanner:

			// ========= Setting debug Digital pins
			// * NOTE : we set here only the pins that do not belong to specific hardware;
			pinMode(PIN_LED_DEBUG, OUTPUT);   digitalWrite(PIN_LED_DEBUG, LOW);     // for debug, etc
			pinMode(PIN_LED_MESSAGE, OUTPUT); digitalWrite(PIN_LED_MESSAGE, LOW); // to signal good message reception

			// Multi-purpose external triggers (NOTE: they are not associated necessarily with a microcontroller interrupt,
			// but it may be important for some experiments?). BTW, practically ALL Teensy3.6 pins can be configured as
			// external interrupts, and set using the classic Arduino methods "attachInterrupt()"...
			pinMode(PIN_TRIGGER_OUTPUT, OUTPUT);
			pinMode(PIN_TRIGGER_INPUT, INPUT_PULLUP);

			// LEGACY INTENSITY/BLANKING. Here it will be used so that it is HIGH whenever the lasers are ON, and OFF otherwise.
			// For the time being, it will be ON when display engine is running, and OFF otherwise?
			pinMode(PIN_INTENSITY_BLANKING, OUTPUT);
			if (Lasers::someLaserOn()) digitalWrite(PIN_INTENSITY_BLANKING, HIGH);
			else digitalWrite(PIN_INTENSITY_BLANKING, LOW);

			// * SHUTTER PIN: should put 5V when drawing and lasers ON, and 0 otherwise.
			pinMode(PIN_SHUTTER, OUTPUT);
			digitalWrite(PIN_SHUTTER, LOW); // NOTE: Shutter control is MANUAL (command)


			// Setting D25 exposed digital and analog pins (analog pins on timer TMP1):
			analogWriteFrequency(PIN_ANALOG_A, 65000); //PIN_ANALOG_B set automatically as it is on the same timer, while
			// resolution is common for all PWM timer [available on Teensy LC, 3.0 - 3.6]

			// Setting mode for the digital pins (PIN_DIGITAL_A and B):
			// unnecesary, the mode is re-selected when using the wrapping methods.

			analogWriteResolution(RES_PWM); // 12 is 0 to 4095 [we could have a ANALOG_RESOLUTION define or const, and do the log]
			// NOTE : this affects the resolution on ALL the PWM channels (could not be so, since there are many independent
			// timers, but that's the way the library works now). Fortunately, the carrier pwm uses a fized 50% duty cycle, so
			// with a 12 bit resolution we just make the duty cycle equal to 2047 (and even if this resolution is too large for
			// the "carrier" high freq pwm, the library will map it into the correct value - and will be able to do so properly,
			// since we just need to be in the middle of the range)



			PRINTLN("> GPIOs READY");
		}

	}

	namespace Lasers {

		Laser LaserArray[NUM_LASERS]; // definition external

		extern void init() {

			// Set the PWM frequency for all the power pwm pins (need to set only on one):
			analogWriteFrequency(pinPowerLaser[0], FREQ_PWM_POWER);

			// Carrier (when used). NOTE: it could be something different from a square wave, 50% duty ratio!
			// NOTE: it uses a different timer from the PWM for power.
			analogWriteFrequency(pinSwitchLaser[0],FREQ_PWM_CARRIER);
			//analogWrite(pinSwitchLaser[0], 2047); // this will start the pwm signal

			for (uint8_t i=0; i<NUM_LASERS; i++) {
				LaserArray[i].init(pinPowerLaser[i], pinSwitchLaser[i]); // power, switch and carrier are off
			}

			PRINTLN("> LASERS READY");
		}

		extern void test() { // Switch lasers one by one and try a power ramp on each of these:
			// TODO: have a state variable for current color (in graphics.h), with a push/pop method

			//Stop scan and recenter, to be able to measure the power of the beam?
			// or continue scanning the current figure?
			// bool previousState = DisplayScan::getRunningState();
			// if (previousState) DisplayScan::stopDisplay();
			// Scanner::recenterPosRaw();

			pushState(); // save current laser state
			setCarrierModeAll(false);

			elapsedMillis msTime;

			PRINTLN("TESTING LASERS: ");

			for (uint8_t i=0; i<NUM_LASERS; i++) {
				PRINT("TEST LASER: "); PRINTLN(i);
				setStateSwitch(i, true);

				for (uint16_t p=0; p<MAX_LASER_POWER; p+=100) {
					setStatePower(i,p);
					//	PRINT("power: "); PRINTLN(p);
					msTime =0;
					while (msTime < 50);
				}
				for (int16_t p=MAX_LASER_POWER; p>=0; p-=100) { // attn with the >= on uint!!
					setStatePower(i,p);
					//	PRINT("power: "); PRINTLN(p);
					msTime =0;
					while (msTime < 50);
				}

				setStateSwitch(i, false);

				msTime =0;
				while (msTime < 500);
			}

			popState(); // back to current laser state.

			// restart the ISR?
			//	if (previousState) DisplayScan::startDisplay();

		}

	}

	namespace OptoTuners {

		OptoTune OptoTuneArray[NUM_OPTOTUNERS];

		extern void init() {

			// Set the PWM frequency for all the optotune pwm pins (need to set only on one):
			analogWriteFrequency(pinPowerOptoTuner[OPTOTUNE_A], FREQ_PWM_OPTOTUNE);

			// Resolution [available on Teensy LC, 3.0 - 3.6]:
			analogWriteResolution(RES_PWM); // 12 is 0 to 4095 [we could have a ANALOG_RESOLUTION define or const, and do the log]
			// NOTE : this affects the resolution on ALL the PWM channels (could not be so, since there are many independent
			// timers, but that's the way the library works now).

			for (uint8_t i=0; i<NUM_OPTOTUNERS; i++) {
				OptoTuneArray[i].init(pinPowerOptoTuner[i]); // power, switch and carrier are off
			}

			PRINTLN("> OPTOTUNERS READY");
		}

		extern void test() {

			elapsedMillis msTime;

			PRINTLN("TESTING OPTOTUNERS: ");

			for (uint8_t i=0; i<NUM_OPTOTUNERS; i++) {
				PRINT("TEST OPTOTUNE: "); PRINTLN(char(i+65)); // letters A, B ...

				for (uint16_t p=0; p<MAX_OPTOTUNE_POWER; p+=100) {
					setPower(i,p);
					//	PRINT("power: "); PRINTLN(p);
					msTime =0;
					while (msTime < 50);
				}
				for (int16_t p=MAX_OPTOTUNE_POWER; p>=0; p-=100) { // attn with the >= on uint!!
					setPower(i,p);
					//	PRINT("power: "); PRINTLN(p);
					msTime =0;
					while (msTime < 50);
				}

				// Go back to previous power state for all optotuners: not changed because we did not call setStatePower but setPower
				setToCurrentState();
			}
		}

	}

	namespace Scanner {

		void init() {
			// RENCENTER mirror and fill BOTH buffers with the central position too:
			recenterPosRaw();
			PRINTLN("> SCANNERS READY");
		}

		//inline void setMirrorsTo(uint16_t _posX, uint16_t _posY);
		//inline void recenterMirrors();

		// Repeatedly make a square 50x50 points by side;
		void testMirrorRange(uint16_t _durationSec) {
			elapsedMicros usec =0; // better interface than using micros(),
			// but using it internally. ISR priority: 32
			elapsedMillis msec = 0;

			// First, stop the ISR whatever its state:
			bool previousState = DisplayScan::getRunningState();
			if (previousState) DisplayScan::stopDisplay();

			uint16_t stepX = (MAX_MIRRORS_ADX-MIN_MIRRORS_ADX)/100;
			uint16_t stepY = (MAX_MIRRORS_ADY-MIN_MIRRORS_ADY)/100;
			uint16_t x=MIN_MIRRORS_ADX, y=MIN_MIRRORS_ADY;

			// Prepare initial position (wait a little more)
			analogWrite( PIN_ADCX, x );
			#ifdef TEENSY_35_36
			analogWrite( PIN_ADCY, y );
			#endif
			while (usec < 300); usec = 0;

			while (msec < (_durationSec*1000)) {

				// Make a square 50x50 points side:
				do  {
					#ifdef TEENSY_35_36
					analogWrite( PIN_ADCY, y );
					#endif
					y+=stepY;
					while (usec < 100); usec = 0;// wait 100us
				} while (y< MAX_MIRRORS_ADY);
				y-=stepY;
				do  {
					analogWrite( PIN_ADCX, x );
					x+=stepX;
					while (usec < 100); usec = 0;// wait 100us
				} while (x< MAX_MIRRORS_ADX);
				x-=stepX;
				do  {
					y-=stepY;
					#ifdef TEENSY_35_36
					analogWrite( PIN_ADCY, y );
					#endif
					while (usec < 100); usec = 0;// wait 100us
				} while (y>MIN_MIRRORS_ADY);
				//y=MIN_MIRRORS_ADY;
				do  {
					x-=stepX;
					analogWrite( PIN_ADCX, x );
					while (usec < 100); usec = 0;// wait 100us
				} while (x> MIN_MIRRORS_ADX);
				//x=MIN_MIRRORS_ADX;

			}
			recenterPosRaw();
			// restart the ISR?
			if (previousState) DisplayScan::startDisplay();
		}

		void testCircleRange(uint16_t _durationSec) {
			// first, stop the ISR whatever its state:
			bool previousState = DisplayScan::getRunningState();
			if (previousState) DisplayScan::stopDisplay();
			elapsedMicros usec =0; elapsedMillis msec = 0;
			while (msec<_durationSec*1000) {
				//while (usec < 1000) ; // wait
				//usec = 0;
				for (uint16_t k=0;k<360;k++) {
					float phi = 1.0*DEG_TO_RAD*k;
					uint16_t x=(uint16_t)(  1.0*CENTER_MIRROR_ADX*(1.0+cos(phi))  );
					uint16_t y=(uint16_t)(  1.0*CENTER_MIRROR_ADY*(1.0+sin(phi))  );
					analogWrite( PIN_ADCX, x );
					#ifdef TEENSY_35_36
					analogWrite( PIN_ADCY, y );
					#endif
					while (usec < 100) ; // wait
					usec = 0;
				}
			}
			recenterPosRaw();
			// restart the ISR?
			if (previousState) DisplayScan::startDisplay();
		}

		void testCrossRange(uint16_t _durationSec) {
			// first, stop the ISR whatever its state:
			bool previousState = DisplayScan::getRunningState();
			if (previousState) DisplayScan::stopDisplay();

			uint16_t stepX = (MAX_MIRRORS_ADX-MIN_MIRRORS_ADX)/100;
			uint16_t stepY = (MAX_MIRRORS_ADY-MIN_MIRRORS_ADY)/100;

			uint16_t x, y;
			elapsedMicros usec =0; elapsedMillis msec = 0;

			while (msec < (_durationSec*1000)) {

				// vertical line:
				y = MIN_MIRRORS_ADY;
				analogWrite( PIN_ADCX, CENTER_MIRROR_ADX );
				do  {
					#ifdef TEENSY_35_36
					analogWrite( PIN_ADCY, y );
					#endif
					y+=stepY;
					while (usec < 100); usec = 0;// wait 100us
				} while (y< MAX_MIRRORS_ADY);

				// horizontal line:
				x = MIN_MIRRORS_ADX;
				#ifdef TEENSY_35_36
				analogWrite( PIN_ADCY, CENTER_MIRROR_ADY );
				#endif
				do  {
					analogWrite( PIN_ADCX, x );
					x+=stepX;
					while (usec < 100); usec = 0;// wait 100us
				} while (x < MAX_MIRRORS_ADX);

			}

			recenterPosRaw();

			if (previousState) DisplayScan::startDisplay();
		}

	}

	namespace Lcd {
		#ifdef DEBUG_MODE_LCD

		rgb_lcd lcd;

		void init() {
			// set up the LCD's number of columns and rows:
			lcd.begin(16, 2);
			//lcd.clear();
			lcd.setRGB(0,200,0);
			// lcd.setPWM(REG_GREEN, i);
		}

		// wrappers for LCD display - could do more than just wrap the lcd methods, i.e, vertical scroll with interruption, etc.
		void print(String text) {
			//lcd.clear();
			lcd.print(text);
		}

		void println(String text) {
			static uint8_t row =0;
			lcd.print(text);
			row++;
			if (row==2) {
				row = 0;
				//lcd.clear();
			}
			lcd.setCursor(0,row);
		}
		
		#endif
	}


	namespace Tft {
		#ifdef DEBUG_MODE_TFT

		//uint8_t row=0, col=0;
		// extern Adafruit_ST7735 *tft = new Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

		uint8_t row=0, col=0;

		//Adafruit_ST7735 tft;
		//	Adafruit_ST7735 *tft = new Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);
		Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

		void init() {
			// Use this initializer if you're using a 1.8" TFT
			tft.initR(INITR_BLACKTAB);   // initialize a ST7735S chip, black tab

			// Use this initializer (uncomment) if you're using a 1.44" TFT
			//tft.initR(INITR_144GREENTAB);   // initialize a ST7735S chip, black tab

			tft.fillScreen(ST7735_BLACK);
			tft.setCursor(0, 5);
			tft.setTextColor(ST7735_WHITE);
			tft.setTextWrap(true);
			tft.setTextSize(0.8);
		}

		// wrappers for LCD display - could do more than just wrap the lcd methods, i.e, vertical scroll with interruption, etc.
		void print(String text) {
			//tft.setCursor(row, 0);// on the current row
			tft.print(text);
		}

		void println(String text) {
			row = (row+1)%16;
			if (!row) {
				tft.fillScreen(ST7735_BLACK);
				tft.setCursor(0, 5);
			}
			tft.print(text);
			tft.print("\n");
		}

		void setPixel(uint16_t x, uint16_t y) {
			tft.drawPixel(x, y, ST7735_GREEN);
		}
		#endif
	}

}
