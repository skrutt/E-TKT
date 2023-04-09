// MIT License

// Copyright (c) 2022 Andrei Speridião

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

//
// for more information, please visit https://github.com/andreisperid/E-TKT
//

// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
// ~~~~~ IMPORTANT: do not forget to upload the files in "data" folder using SPIFFS ~~~~~
// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

// libraries
#include <Arduino.h>
#include <AccelStepper.h>
#include <ESP32Servo.h>
#include <WiFi.h>
#include "esp_wifi.h"
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESPAsyncWiFiManager.h>
#include "SPIFFS.h"
#include <ESP32Tone.h>
#include <Preferences.h>
#include "ArduinoJson.h"
#include "AsyncJson.h"

#include "Labelmaker.h"



// oled display
#include "display.h"

// BASIC CONFIGURATION ------------------------------------------------------------


// Speed and acceleration of the stepper motor that rotates the character carousel, measured in steps/s and steps/s^2.
// Use lower values if you find that the printer sometimes prints the wrong letter.  Any value above zero is ok but
// lower values will slow down printing, if you're having trouble start by halving them and move up from there.  The
// speed you can reliably achieve depends on the quality of the motor, how much current you've set it up to use, and
// how fast the ESP-32 can talk with it. 1600 steps is a full revolution of the carousel.
#define CHARACTER_STEPPER_MAX_SPEED 320000
#define CHARACTER_STEPPER_MAX_ACCELERATION 16000
#define CHARACTER_STEPPER_FINE_HOMING_SPEED 320

// Speed and acceleration of the stepper motor that feeds the label tape, measured in steps/s and steps/s^2.
// Use lower values if you find that the printer doesn't consistently feed the correct length of tape between letters.
// For calibrating these values the same advice about the character stepper motor above applies. 4076 steps is one full
// revolution of the motor.
#define FEED_STEPPER_MAX_SPEED 1000000
#define FEED_STEPPER_MAX_ACCELERATION 1000

// HARDWARE -----------------------------------------------------------------------


// home sensor
#define sensorPin 34 // hall sensor
#define threshold 128 // between 0 and 1023
int currentCharPosition = -1;
int deltaPosition;

// wifi reset
#define wifiResetPin 13 // tact switch

// steppers
#define MICROSTEP_Feed 8
#define stepsPerRevolutionFeed 4076
AccelStepper stepperFeed(MICROSTEP_Feed, 15, 2, 16, 4);

#define enableCharStepper 25
#define MICROSTEP_Char 16 // for more precision, maximum available microsteps for the character stepper
float stepsPerChar;
AccelStepper stepperChar(1, 32, 33);
#ifndef REVERSE_CHAR_STEPPER_DIRECTION
const int stepsPerRevolutionChar = -200 * MICROSTEP_Char;
#else
const int stepsPerRevolutionChar = 200 * MICROSTEP_Char;
#endif

// servo
Servo myServo;
#define servoPin 14
#define restAngle 50
#define targetAngle 22
int peakAngle;


// leds
#define ledFinish 5
#define ledChar 17

// buzzer
#define buzzerPin 26

// E-TKT musical signature
const int etktNotes[8] = {
	44, 44, 16, 2, 31, 22, 31, 44};

// selected musical scale for playing tunes
const int charNoteSet[charQuantity + 1] = {
	G4, G6, A4, D4, E4, F4, G5, A5, B5, C5, D5, 0, E5, F5, C6, D6, E6, F6, A6, B6, C4, C7, D7, E7, F7, G7, B4, A7, B7, C8, D8, C3, D3, E3, F3, G3, A3, B3, E2, F2, G2, A2, B2, 0};

// DEBUG --------------------------------------------------------------------------
// comment lines individually to isolate functions

// #define do_cut
// #define do_press
#define do_char
#define do_feed
#define do_sound
// #define do_wifi_debug
// #define do_display_debug
 #define do_serial

// DATA ---------------------------------------------------------------------------

//Progress of printing
float webProgress = 0;
String displaySSID = "";
String displayIP = "";

//Getters
float getwebProgress()
{
	return webProgress;
}

String getSSID()
{
	return displaySSID;
}
String getIP()
{
	return displayIP;
}


// A map from each supported character to its index in the carousel.
// Characters are stored as a string instead of a native char type to support
// multi-byte code-points like "☆". Some characters share the same position,
// such as "I" and "1".
// important: keep in mind the home is always on 21st character (J)
const std::map<String, int> charSet = {
	{"$", 0}, {"-", 1}, {".", 2}, {"0", 26}, {"1", 20}, {"2", 3}, {"3", 4}, {"4", 5}, {"5", 6}, {"6", 7}, {"7", 8}, {"8", 9}, {"9", 10}, {"*", 11}, {"A", 12}, {"B", 13}, {"C", 14}, {"D", 15}, {"E", 16}, {"F", 17}, {"G", 18}, {"H", 19}, {"I", 20}, {"J", 21}, {"K", 22}, {"L", 23}, {"M", 24}, {"N", 25}, {"O", 26}, {"P", 27}, {"Q", 28}, {"R", 29}, {"S", 30}, {"T", 31}, {"U", 32}, {"V", 33}, {"W", 34}, {"X", 35}, {"Y", 36}, {"Z", 37}, {"♡", 38}, {"☆", 39}, {"♪", 40}, {"€", 41}, {"@", 42}};



String labelString;
String prevChar = "J";
const int charHome = 21;
bool waitingLabel = false;

volatile bool busy = false; // volatile as the variable is read by both cores (0 and 1)

String parameter = "";
String value = "";
TaskHandle_t processorTaskHandle = NULL; // for dual core operation

// memory storage for align and force settings
Preferences preferences;
#define defaultAlign 5 // 1 to 9, 5 is the mid value
#define defaultForce 1 // 1 to 9
int alignFactor = defaultAlign;
int forceFactor = defaultForce;
int newAlign = defaultAlign;
int newForce = defaultForce;
String combinedSettings = "x";



// --------------------------------------------------------------------------------
// ASSEMBLY -----------------------------------------------------------------------
// adjust both values below for a rough starting point

// depending on the hall sensor positioning, the variable below makes sure the initial calibration is within tolerance
// use a value between -1.0 and 1.0 to make it roughly align during assembly
//positive values are clockwise
const float assemblyCalibrationAlign = 8.5;

// depending on servo characteristics and P_press assembling process, the pressing angle might not be so precise and the value below compensates it
// use a value between 0 and 20 to make sure the press is barely touching the daisy wheel on test align
//higher is a harder press
const int assemblyCalibrationForce = 1;

// after that, proceed to fine tune on the E-TKT's app when the machine is fully assembled

// --------------------------------------------------------------------------------
// COMMUNICATION ------------------------------------------------------------------

// create AsyncWebServer object on port 80
AsyncWebServer server(80);
DNSServer dns;



// --------------------------------------------------------------------------------
// LEDS ---------------------------------------------------------------------------

void lightFinished()
{
	// lights up when the label has finished printing

	bool state = true;
	for (int i = 0; i < 10; i++)
	{
		analogWrite(ledFinish, state * 128);
		state = !state;
		delay(100);
	}
	for (int i = 128; i >= 0; i--)
	{
		analogWrite(ledFinish, i);
		state = !state;
		delay(25);
	}
	webProgress = 0;
}

void lightChar(float state)
{
	// lights up when the character is pressed against the tape
	int s = state * 128;

	analogWrite(ledChar, s);
}

// --------------------------------------------------------------------------------
// BUZZER -------------------------------------------------------------------------

void sound(int frequency = 2000, int duration = 1000)
{
	tone(buzzerPin, frequency, duration);
}

void labelMusic(String label)
{
	// plays a music according to the label letters

	int length = utf8Length(label);

	for (int i = 0; i < length; i++)
	{
		auto character = utf8CharAt(label, i);
		if (charSet.count(character) == 0)
		{
			// The character isn't on the wheel, so just ignore it.
			continue;
		}
		auto index = charSet.at(character);
#ifdef do_serial
		Serial.println(charNoteSet[index]);
#endif
		tone(buzzerPin, charNoteSet[index], 100);

		delay(50);
	}
}

void eggMusic(String notes, String durations)
{
	// ♪ By pressing down a special key ♪
	// ♪ It plays a little melody ♪

	int length = utf8Length(notes);

	for (int i = 0; i < length; i++)
	{
		auto character = utf8CharAt(notes, i);
		if (charSet.count(character) == 0)
		{
			// Ignore characters not on the wheel.
			continue;
		}
		auto index = charSet.at(character);
		char charDuration = durations[i];
		float duration = 2000 / atoi(&charDuration);
		tone(buzzerPin, charNoteSet[index], duration);
	}
}


// --------------------------------------------------------------------------------
// MECHANICS ----------------------------------------------------------------------

//Move inversion here to get cleaner code w less repetition
#ifdef INVERT_HALL_SENSOR_LOGIC
	const bool HOMING_INVERSION = INVERT_HALL_SENSOR_LOGIC;
#else
	const bool HOMING_INVERSION = 0;
#endif

bool doHomeMove()
{
	int triggerState = 0;

	// Move the carousel until the hall sensor triggers, and then treat wherever
	// that is as the new home position.
	stepperChar.move(stepsPerRevolutionChar * 1.5f);
	
	// int sensorState = analogRead(sensorPin);
	// while ((sensorState > threshold) ^ HOMING_INVERSION)
	while ((triggerState = (digitalRead(sensorPin) ^ HOMING_INVERSION)))
	{
		stepperChar.run();
		//feedLoopWDT();
		delayMicroseconds(100); // TODO: less intrusive way to avoid triggering watchdog?
		// sensorState = analogRead(sensorPin);
	}

	stepperChar.setCurrentPosition(0);

	// Return trigger result, indicating success or not (0 for success)
	return triggerState;
}

void setHome(int align = alignFactor)
{
	// runs the char stepper clockwise until triggering the hall sensor, then call it home at char 21

	float a = (align - 5.0f) / 10.0f;

#ifdef do_serial
	Serial.print("align: ");
	Serial.println(align);

	Serial.print("homing with a = ");
	Serial.println(a);
#endif

	//first home move. Will be skiped if already on trigger
	doHomeMove();

	//fine homing
	//move off again, 90% of a revolution
	stepperChar.runToNewPosition(stepsPerChar * 40);

	//Set slow speed
	stepperChar.setMaxSpeed(CHARACTER_STEPPER_FINE_HOMING_SPEED);

	doHomeMove();

	//reset to full speed
	stepperChar.setMaxSpeed(CHARACTER_STEPPER_MAX_SPEED);

	// TODO: Add a failure path for if the stepper moved a full rotation without trigerring
	// the sensor, inidcating that something is wrong with the hardware.

	// Align to refence point for homing
	//Always move forward so: 1 full rotation - offset
	stepperChar.runToNewPosition(stepsPerChar + stepsPerRevolutionChar - ((stepsPerChar * a) + (assemblyCalibrationAlign * stepsPerChar)));
	stepperChar.run();
	stepperChar.setCurrentPosition(0);
	currentCharPosition = charHome;
	prevChar = charHome;

	delay(100);
}

void feedLabel(int repeat = 1)
{
	// runs the feed stepper by a specific amount to push the tape forward

#ifdef do_serial
	Serial.print("feed ");
	Serial.print(repeat);
	Serial.println("x");
#endif

	stepperFeed.enableOutputs();
	delay(10);

	// for (int i = 0; i < repeat; i++)
	// {

#ifdef REVERSE_FEED_STEPPER_DIRECTION
		const int direction = 1;
#else
		const int direction = -1;
#endif
		stepperFeed.runToNewPosition(stepperFeed.currentPosition() + (stepsPerRevolutionFeed / 8) * direction * repeat);
		delay(10);
	// }

	stepperFeed.disableOutputs();

	delay(20);
}

void pressLabel(bool strong = false, int force = forceFactor, bool slow = false)
{
	// press the label

#ifdef do_serial
	Serial.println("press");
#endif

	int delayFactor = 0;

	peakAngle = targetAngle + 9 - assemblyCalibrationForce - force;

	if (strong)
	{
		delayFactor = 4;
	}
	else
	{
		delayFactor = slow ? 100 : 0;
	}
	lightChar(1.0f); // lights up the char led

	for (int pos = restAngle; pos >= peakAngle; pos--)
	{
		myServo.write(pos);
		delay(delayFactor);
	}
	for (int i = 0; i < 5; i++) // to make sure the servo has reached the peak position
	{
		myServo.write(peakAngle);
		delay(50);
	}

	for (int pos = peakAngle; pos <= restAngle; pos++)
	{
		myServo.write(pos);
		delay(delayFactor);
	}
	for (int i = 0; i < 5; i++) // to make sure the servo has reached the rest position
	{
		myServo.write(restAngle);
		delay(50);
	}

	lightChar(0.2f); // dims the char led
}

void goToCharacter(String c, int override = alignFactor)
{
	// reaches out for a specific character

#ifdef do_serial
	Serial.println("char");
#endif

	// calls home everytime to avoid accumulating errors
	setHome(override);

	// matches the character to the list and gets delta steps from home
	if (charSet.count(c) != 0)
	{
		auto index = charSet.at(c);
		deltaPosition = index - currentCharPosition;
		currentCharPosition = index;
	}

	if (deltaPosition < 0)
	{
		deltaPosition += 43;
	}

	// runs char stepper clockwise to reach the target position
	stepperChar.runToNewPosition(stepsPerChar * deltaPosition);

	delay(25);
}

void cutLabel()
{
	// moves to a specific char (*) then presses label three times (more vigorously)

	goToCharacter("*");

	for (int i = 0; i < 3; i++)
	{
		pressLabel(true);
	}
}

void setWebProgress(float progress, int labelLength)
{
		// Update the progress reported to the web app.
	webProgress = 100 * progress / labelLength;
	if (webProgress > 0)
	{
		webProgress -= 1; // avoid 100% progress while still finishing
	}
}

void writeLabel(String label)
{
	// manages entirely a particular label printing process, from start to end (task kill)

	// enables servo
	myServo.write(restAngle);
	delay(500);

#ifdef do_serial
	Serial.print("print ");
	Serial.println(label);
#endif

	// all possible characters: $-.23456789*abcdefghijklmnopqrstuvwxyz♡☆♪€@

	int labelLength = utf8Length(label);

	lightChar(0.2f);

	displayInitialize();
	
	setWebProgress(0, labelLength);
	displayProgress(0, label);

#ifdef do_sound
	if (label == " TASCHENRECHNER " || label == " POCKET CALCULATOR " || label == " DENTAKU " || label == " CALCULADORA " || label == " MINI CALCULATEUR ")
	{
		eggMusic("*4599845887*459984588764599845887*4599845887", "88843888484888438884848884388848488843888484"); // ♪ I'm the operator with my pocket calculator ♪
	}
	else
	{
		labelMusic(label);
	}
#else
	delay(200);
#endif

	// enable and home char stepper
	stepperChar.enableOutputs();
	setHome();

#ifdef do_feed
	feedLabel();
#else
	delay(500);
#endif

	for (int i = 0; i < labelLength; i++)
	{
		auto character = utf8CharAt(label, i);
		if (character != " " && character != prevChar) // skip char seeking on repeated characters or on spaces
		{
#ifdef do_char
			goToCharacter(character);
#else
			delay(500);
#endif
			prevChar = character;
		}

		if (character != " ") // skip pressing on label spaces
		{

#ifdef do_press
			pressLabel();
#else
			delay(500);
#endif
		}

#ifdef do_feed
		feedLabel();
#else
		delay(500);
#endif
		delay(500);
		setWebProgress(i + 1, labelLength);
		displayProgress(i + 1, label);
	}

	if (labelLength < 6 && labelLength != 1) // minimum label length to make sure the user can grab it
	{
		int spaceDelta = 6 - labelLength;
		for (int i = 0; i < spaceDelta; i++)
		{
#ifdef do_feed
			feedLabel();
#else
			delay(500);
#endif
		}
	}

#ifdef do_cut
	cutLabel();
#else
	delay(500);
#endif

	lightChar(0.0f);

	// reset server parameters
	parameter = "";
	value = "";
	myServo.write(restAngle);

	stepperChar.disableOutputs();

	webProgress = 100;
	displayFinished();
	lightFinished();
	busy = false;
	displayQRCode();
	vTaskDelete(processorTaskHandle); // delete task
}

// --------------------------------------------------------------------------------
// DATA ---------------------------------------------------------------------------

void loadSettings()
{
	// load settings from internal memory

	preferences.begin("calibration", false);

	// check if are there any align and force values stored in memory
	int a = preferences.getUInt("align", 0);
	if (a == 0)
	{
		preferences.putUInt("align", defaultAlign);
	}
	int f = preferences.getUInt("force", 0);
	if (f == 0)
	{
		preferences.putUInt("force", defaultForce);
	}

	alignFactor = preferences.getUInt("align", 0);
	forceFactor = preferences.getUInt("force", 0);
	preferences.end();

	combinedSettings = alignFactor;
	combinedSettings.concat(forceFactor);

#ifdef do_serial
	Serial.print("combined settings (align/force) ");
	Serial.println(combinedSettings);
#endif
}

void testSettings(int tempAlign, int tempForce, bool full = true)
{
	// test settings (align or align + force) without writing to the memory

#ifdef do_serial
	Serial.print("testing align ");
	Serial.print(tempAlign);
	Serial.print(" and force ");
	Serial.println(tempForce);
#endif

	stepperChar.enableOutputs();

	if (full)
	{
		// feedLabel(4);
		feedLabel(2);
		goToCharacter("E", tempAlign);
		pressLabel(false, tempForce, false);
		feedLabel();
		goToCharacter("-", tempAlign);
		pressLabel(false, tempForce, false);
		feedLabel();
		goToCharacter("T", tempAlign);
		pressLabel(false, tempForce, false);
		feedLabel();
		goToCharacter("K", tempAlign);
		pressLabel(false, tempForce, false);
		feedLabel();
		goToCharacter("T", tempAlign);
		pressLabel(false, tempForce, false);
		feedLabel(2);
		goToCharacter("*", tempAlign);
		pressLabel(true, tempForce, false);
		pressLabel(true, tempForce, false);
		pressLabel(true, tempForce, false);
	}
	else
	{
		goToCharacter("M", tempAlign);
		pressLabel(false, 1, true);
	}

	stepperChar.disableOutputs();
}

void saveSettings(int tempAlign, int tempForce)
{
	// save settings to memory

	preferences.begin("calibration", false);

	preferences.putUInt("align", tempAlign);
	preferences.putUInt("force", tempForce);

	alignFactor = tempAlign;
	forceFactor = tempForce;

#ifdef do_serial
	Serial.print("saved align ");
	Serial.print(alignFactor);
	Serial.print(" and force ");
	Serial.println(forceFactor);
#endif

	preferences.end();
}

void interpretSettings()
{
	// interpret strings coming from the web app
	String a = value.substring(0, 1);
	String f = value.substring(2, 3);
	newAlign = (int)a.toInt();
	newForce = (int)f.toInt();

#ifdef do_serial
	Serial.print("align: ");
	Serial.print(newAlign);
	Serial.print(", force: ");
	Serial.println(newForce);
#endif
}

void processor(void *parameters)
{
	// receives and treats commands from the webapp

	for (;;)
	{
		if (parameter == "feed")
		{
			// feeds the tape by 1 character length
			busy = true;

			displayInitialize();
			displayFeed();

			analogWrite(ledFinish, 32);
			myServo.write(restAngle);
			delay(500);

#ifdef do_feed
			feedLabel();
#else
			delay(500);
#endif

			parameter = "";
			value = "";
			stepperChar.disableOutputs();
			myServo.write(restAngle);

			busy = false;
			webProgress = 100;
			analogWrite(ledFinish, 0);
			lightChar(0.0f);
			displayQRCode();
			vTaskDelete(processorTaskHandle); // delete task
		}
		else if (parameter == "reel")
		{
			// feeds the tape in a fixed length from the cog to the press
			busy = true;

			displayInitialize();
			displayReel();
			analogWrite(ledFinish, 32);
			myServo.write(restAngle);
			delay(500);

			for (int i = 0; i < 16; i++)
			{
#ifdef do_feed
				feedLabel();
#else
				delay(500);
#endif
			}

			parameter = "";
			value = "";
			stepperChar.disableOutputs();
			busy = false;
			webProgress = 100;
			analogWrite(ledFinish, 0);
			lightChar(0.0f);
			displayQRCode();
			vTaskDelete(processorTaskHandle); // delete task
		}
		else if (parameter == "cut")
		{
			// manually cuts the tape in the current tape position
			busy = true;

			displayInitialize();
			displayCut();

			lightChar(0.2f);
			stepperChar.enableOutputs();
			myServo.write(restAngle);
			delay(500);

#ifdef do_cut
			cutLabel();
#else
			delay(500);
#endif

			parameter = "";
			value = "";
			stepperChar.disableOutputs();
			busy = false;
			webProgress = 100;
			lightChar(0.0f);
			displayQRCode();
			vTaskDelete(processorTaskHandle); // delete task
		}
		else if (parameter == "tag" && value != "")
		{
			// prints a tag

			busy = true;
			String label = value;
			label.toUpperCase();
			writeLabel(label);
			busy = false;
		}
		else if ((parameter == "testalign" || parameter == "testfull") && value != "")
		{
			// tests the settings chosen in the web app

			busy = true;
			interpretSettings();

			displayInitialize();
			displayTest(newAlign, newForce);
			analogWrite(ledFinish, 0);

			bool testFull = false;
			if (parameter == "testfull")
			{
				testFull = true;
#ifdef do_serial
				Serial.print("testing full temporary settings / align: ");
				Serial.print(newAlign);
				Serial.print(" / force: ");
				Serial.println(newForce);
#endif
			}
			else if (parameter == "testalign")
			{
				testFull = false;
#ifdef do_serial
				Serial.print("testing align temporary settings / align: ");
				Serial.print(newAlign);
				Serial.print(" / force: ");
				Serial.println(newForce);
#endif
			}

			testSettings(newAlign, newForce, testFull);

			newAlign = 0;
			newForce = 0;
			value = "";

			webProgress = 100;

			analogWrite(ledFinish, 0);
			lightChar(0.0f);
			displayQRCode();

			busy = false;
			vTaskDelete(processorTaskHandle); // delete task
		}
		else if (parameter == "save" && value != "")
		{
			// save the settings from the web app
			busy = true;
			interpretSettings();

#ifdef do_serial
			Serial.println("saving settings");
#endif

			displayInitialize();
			displaySettings(newAlign, newForce);
			analogWrite(ledFinish, 0);

			saveSettings(newAlign, newForce);

			newAlign = 0;
			newForce = 0;
			value = "";

			displayReboot();

			webProgress = 100;

			analogWrite(ledFinish, 0);
			lightChar(0.0f);
			busy = false;
			delay(500);
			ESP.restart(); // instead of deleting task, reboots
		}
	}
}

// --------------------------------------------------------------------------------
// COMMUNICATION ------------------------------------------------------------------

void notFound(AsyncWebServerRequest *request)
{
	request->send(404, "text/plain", "Not found");
}

void getStatus(AsyncWebServerRequest *request)
{
	AsyncJsonResponse *response = new AsyncJsonResponse();
	const JsonObject &root = response->getRoot();

	// TODO: There is a potential (but rare) race condition here if the
	// status is updated while being changed.  Consider blocking while
	// state changes happen.
	root["progress"] = webProgress;
	root["busy"] = busy;
	root["command"] = parameter;
	root["align"] = alignFactor;
	root["force"] = forceFactor;

	// TODO: The webserver should also indicate what the text of the current,
	// label is, however this will require substantial additional refactoring
	// of how the current tag is communicated.
	response->setLength();
	request->send(response);
}

void handleTaskRequest(AsyncWebServerRequest *request, JsonVariant &json)
{
	Serial.println("Got task request");
	const auto request_data = json.as<JsonObject>();
	auto response_data = new AsyncJsonResponse();
	const auto response_root = response_data->getRoot();
	if (!request_data.containsKey("value"))
	{
		response_root["error"] = "Please provide a value";
		response_data->setCode(400);
	}
	else if (!request_data.containsKey("parameter"))
	{
		response_root["error"] = "Please provide a parameter";
		response_data->setCode(400);
	}
	else if (busy)
	{
		response_root["error"] = "Printer is already busy";
		response_data->setCode(400);
	}
	else
	{
		parameter = request_data["parameter"].as<String>();
		value = request_data["value"].as<String>();
		Serial.println("Creating task");
		xTaskCreatePinnedToCore(
			processor,			  // the processor() function that processes the inputs
			"processorTask",	  // name of the task
			10000,				  // number of words to be allocated to use on task
			NULL,				  // parameter to be input on the task (can be NULL)
			1,					  // priority for the task (0 to N)
			&processorTaskHandle, // reference to the task (can be NULL)
			0);					  // core 0
		response_root["result"] = "success";
	}
	Serial.println("Request finished");
	response_data->setLength();
	request->send(response_data);
}

void initialize()
{
	// Initialize SPIFFS
	if (!SPIFFS.begin())
	{
#ifdef do_serial
		Serial.println("An Error has occurred while mounting SPIFFS");
#endif
		return;
	}

	// Handles all requests the control the device (print label, feed, cut, test, etc)
	// TODO: Break this method into several api calls, one for each task (eg api/feed,, ap/cut, etc)
	// That way each handler can validate only the arugments it needs.
	server.addHandler(new AsyncCallbackJsonWebHandler("/api/task", handleTaskRequest));

	// Check printing status
	server.on("/api/status", HTTP_GET, getStatus);

	// Serve static assets from the SPIFFS root directory.
	server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");

	// Start server
	server.begin();
}

void configModeCallback(AsyncWiFiManager *myWiFiManager)
{
	// captive portal to configure SSID and password

	displayConfig();

#ifdef do_serial
	Serial.println(myWiFiManager->getConfigPortalSSID());
#endif
}

void clearWifiCredentials()
{
	// load the flash-saved configs
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	esp_wifi_init(&cfg); // initiate and allocate wifi resources
	delay(2000);		 // wait a bit

	// clear credentials if button is pressed
	if (esp_wifi_restore() != ESP_OK)
	{
#ifdef do_serial
		Serial.println("WiFi is not initialized by esp_wifi_init ");
#endif
	}
	else
	{
#ifdef do_serial
		Serial.println("WiFi Configurations Cleared!");
#endif
	}
	displayReset();
	delay(1500);
	esp_restart();
}

void wifiManager()
{
	// local intialization. once its business is done, there is no need to keep it around
	AsyncWiFiManager wifiManager(&server, &dns);

	bool wifiReset = digitalRead(wifiResetPin);
	if (!wifiReset)
	{
		clearWifiCredentials();
	}

	// set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
	wifiManager.setAPCallback(configModeCallback);

#ifdef do_wifi_debug
	wifiManager.setDebugOutput(true);
#else
	wifiManager.setDebugOutput(false);
#endif

	// fetches ssid and pass and tries to connect
	// if it does not connect it starts an access point with the specified name
	// here  "AutoConnectAP"
	// and goes into a blocking loop awaiting configuration
	if (!wifiManager.autoConnect("E-TKT"))
	{
		// Serial.println("failed to connect and hit timeout");
		// reset and try again, or maybe put it to deep sleep
		ESP.restart();
		delay(1000);
	}

	// TODO: pending idea to access the device from a dns name, but Android doesn't support that yet
	// if (!MDNS.begin("e-tkt"))
	// {
	// 	// Serial.println("Error starting mDNS");
	// 	return;
	// }

	// if you get here you have connected to the WiFi
	displayIP = WiFi.localIP().toString();
	displaySSID = WiFi.SSID();

#ifdef do_wifi_debug
	Serial.print("connected at ");
	Serial.print(displayIP);
	Serial.print(" with password ");
	Serial.println(displaySSID);
#endif

	displayQRCode();

	initialize();
}

// --------------------------------------------------------------------------------
// CORE ---------------------------------------------------------------------------
void setup()
{
#ifdef do_serial
	Serial.begin(115200);
	Serial.println("setup");
#endif

	// turns of the leds
	analogWrite(ledChar, 0);
	analogWrite(ledFinish, 0);

	loadSettings();

	// set  display
	displayInitialize();
	displayClear();

	// set pins
	pinMode(sensorPin, INPUT_PULLUP);
	pinMode(wifiResetPin, INPUT_PULLUP);
	pinMode(ledChar, OUTPUT);
	pinMode(ledFinish, OUTPUT);
	pinMode(enableCharStepper, OUTPUT);

	// set feed stepper
	stepperFeed.setMaxSpeed(FEED_STEPPER_MAX_SPEED);
	stepperFeed.setAcceleration(FEED_STEPPER_MAX_ACCELERATION);

	// set char stepper
	digitalWrite(enableCharStepper, HIGH);
	stepperChar.setMaxSpeed(CHARACTER_STEPPER_MAX_SPEED);
	stepperChar.setAcceleration(CHARACTER_STEPPER_MAX_ACCELERATION);

	stepsPerChar = (float)stepsPerRevolutionChar / charQuantity;

	stepperChar.setPinsInverted(true, false, true);
	stepperChar.setEnablePin(enableCharStepper);
	// currentCharPosition = charHome;							//REMOVE ME, for manual homing
	setHome(); // initial home for reference
	stepperChar.disableOutputs();

	// set  servo
	myServo.attach(servoPin);
	myServo.write(restAngle);
	delay(100);

	// splash
	displaySplash();

	// start wifi > comment both to avoid entering the main loop
	wifiManager();
	delay(2000); // time for the task to start
}

void loop()
{
	// the loop should be empty

#ifdef do_display_debug
	debugDisplay();
#endif
}
