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

			// ========= Setting Digital pins
			// * NOTE : we set here only the pins that do not belong to specific hardware;
			pinMode(PIN_LED_DEBUG, OUTPUT);   digitalWrite(PIN_LED_DEBUG, LOW);     // for debug, etc
			pinMode(PIN_LED_MESSAGE, OUTPUT); digitalWrite(PIN_LED_MESSAGE, LOW); // to signal good message reception

			// ========= Configure PWM frequency and resolution
			// Resolution [available on Teensy LC, 3.0 - 3.6]
			analogWriteResolution(12); // 0 to 4095 [we could have a ANALOG_RESOLUTION define or const, and do the log]

			// Frequency PWM pins:
			// * NOTE: the PWM signals are created by hardware timers. PWM pins common to each timer always
			// have the same frequency, so if you change one pin's PWM frequency, all other pins for the same timer change:
			setPWMFreq(FREQ_PWM);
			// Duty cycle: I guess by default it is 0. Otherwise make a const array and reset all the powers. This will
			// be done anyway for the lasers in the Lasers namespace init()

			// ========= DAC: of course, the DACs are not PWM pins. They not need setting.
			// On Teensy Teensy 3.5 and 3.6 the native DACs are on pins A21 and A22), A14 on the
			// Teensy 3.1/3.2, and A12 on the Teensy LC.

			PRINTLN("> GPIOs READY");
		}

		// When setting analogWriteFrequency on pin 5, it affects pins 5, 6, 9, 10, 20, 21, 22, 23
		//[works on 3.1 to 3.6]
		void setPWMFreq(uint16_t _freq) {
			analogWriteFrequency(5, _freq);
		}
	}

	namespace Lasers {

		void init() {

			// Power: will use the PWM pins. No need to set as output, plus its frequency
			// is set in the Gpio init().

			// Switch: set as digital outputs:
			for (uint8_t i=0; i<NUM_LASERS; i++) pinMode(pinSwitchLaser[i], OUTPUT);

			setPowerAll(500);
			switchOnAll();

			PRINTLN("> LASERS READY");
		}

		extern void test() { // Switch lasers one by one and try a power ramp on each of these:
			// TODO: have a state variable for current color (in graphics.h), with a push/pop method

			//Stop scan and recenter, to be able to measure the power of the beam?
			// or continue scanning the current figure?
			// bool previousState = DisplayScan::getRunningState();
			// if (previousState) DisplayScan::stopDisplay();
			// Scanner::recenterPosRaw();

			switchOffAll();
			setPowerAll(0);
			elapsedMillis msTime;

	PRINTLN("TESTING LASERS: ");

			for (uint8_t i=0; i<NUM_LASERS; i++) {
				PRINT("TEST LASER: "); PRINTLN(i);
				setSwitchLaser(i, true);

				for (uint16_t p=0; p<MAX_LASER_POWER; p+=100) {
						setPowerLaser(i,p);
						//	PRINT("power: "); PRINTLN(p);
						msTime =0;
						while (msTime < 50);
				}
				for (int16_t p=MAX_LASER_POWER; p>=0; p-=100) { // attn with the >= on uint!!
					setPowerLaser(i,p);
				//	PRINT("power: "); PRINTLN(p);
					msTime =0;
					while (msTime < 50);
				}

				setSwitchLaser(i, false);

				msTime =0;
				while (msTime < 500);
			}

			// restart the ISR?
		 //	if (previousState) DisplayScan::startDisplay();

		}

		extern void switchOffAll() {
			for (uint8_t i=0; i<NUM_LASERS; i++) digitalWrite(pinSwitchLaser[i], LOW);
		}
		extern void switchOnAll() { // NOTE: power is set independently
			for (uint8_t i=0; i<NUM_LASERS; i++) digitalWrite(pinSwitchLaser[i], HIGH);
		}

		extern void setPowerAll(uint16_t _power) {
			for (uint8_t i=0; i<NUM_LASERS; i++) analogWrite(pinPowerLaser[i], _power);
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
			//lcd.clear();
			//lcd.print("\n");
			row++;
			if (row==2) {
				row = 0;
				lcd.clear();
			}
			lcd.setCursor(0,row);
			lcd.print(text);
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
