#ifndef DISPLAY_H
#define DISPLAY_H


#include <Arduino.h>
#include <qrcode.h>
#include <U8g2lib.h>


void displayClear(int color = 0);
void displayInitialize();
void displaySplash();
void displayConfig();
void displayReset();

void displayQRCode();

/**
 * Renders the printing progress screen, where "progress" is the number of
 * characters already printed.
 */
void displayProgress(float progress, String label);

void displayFinished();
void displayCut();

void displayFeed();

void displayReel();

void displayTest(int a, int f);

void displaySettings(int a, int f);

void displayReboot();

void debugDisplay();


//Common display related calls
int utf8CharLength(String str, int position);

// Returns the length of a UTF-8 encoded string, treating each UTF-8 code-point
// as a single character.
int utf8Length(String str);

// Returns the UTF-8 characters at the given position of the given string,
// treating multi-character code points as inidividual characters.
String utf8CharAt(String str, int position);

#endif