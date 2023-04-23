#ifndef DISPLAY_H
#define DISPLAY_H


#include <Arduino.h>
#include <qrcode.h>
#include <U8g2lib.h>
#include "etktLogo.h" // etkt logo in binary format
#include "pitches.h"
#include "Labelmaker.h"

#include <tuple>
#include <map>



// qr code for accessing the webapp
const int QRcode_Version = 3; //  set the version (range 1->40)
const int QRcode_ECC = 2;	  //  set the Error Correction level (range 0-3) or symbolic (ECC_LOW, ECC_MEDIUM, ECC_QUARTILE and ECC_HIGH)

//declares for music
extern const int etktNotes[];
extern const int charNoteSet[];


// For each non-ascii "glyph" character, maps it to a tuple of (font, symbol code,
// width, x offset, y offset).  These values are used to align the redered glyph
// with the rest of the label text which is from a font with different spacing.
const std::map<String, std::tuple<const uint8_t *, int, int, int, int>> glyphs = {
	{"♡", std::make_tuple(u8g2_font_6x12_t_symbols, 0x2664, 5, -1, -1)},
	{"☆", std::make_tuple(u8g2_font_6x12_t_symbols, 0x2605, 5, -1, -1)},
	{"♪", std::make_tuple(u8g2_font_siji_t_6x10, 0xE271, 5, -3, 0)},
	{"€", std::make_tuple(u8g2_font_6x12_t_symbols, 0x20AC, 6, -1, -1)}};



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