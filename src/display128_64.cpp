

#include "display.h"

#ifdef DISPLAY64

// oled display
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);
#define Lcd_X 128
#define Lcd_Y 64

QRCode qrcode;				  //  create the QR code

// --------------------------------------------------------------------------------
// DISPLAY ------------------------------------------------------------------------

void displayClear(int color)
{
	// paints all pixels according to the desired target color

	for (uint8_t y = 0; y < 64; y++)
	{
		for (uint8_t x = 0; x < 128; x++)
		{
			u8g2.setDrawColor(color);
			u8g2.drawPixel(x, y);
		}
	}
	delay(100);

	u8g2.setDrawColor(color == 0 ? 1 : 0);
	u8g2.setFont(u8g2_font_6x13_te);
}

void displayInitialize()
{
	// starts and sets up the display

	u8g2.begin();
	u8g2.clearBuffer();
	u8g2.setContrast(8); // 0 > 255
	displayClear();
	u8g2.setDrawColor(1);
}

void displaySplash()
{
	// initial start screen

	displayInitialize();

	// invert colors
	displayClear(1);

	u8g2.setFont(u8g2_font_nine_by_five_nbp_t_all);
	u8g2.setDrawColor(0);
	u8g2.drawStr(40, 53, "andrei.cc");
	u8g2.sendBuffer();

	u8g2.setDrawColor(1);

	int n = 1;

	// animated splash
	for (int i = 128; i > 7; i = i - 18)
	{
		for (int j = 0; j < 18; j += 9)
		{
			u8g2.drawXBM(i - j - 11, 8, 128, 32, etktLogo);
			u8g2.sendBuffer();
		}
		if (charNoteSet[etktNotes[n]] != 44)
		{
#ifdef do_sound
			sound(charNoteSet[etktNotes[n]], 200);
#else
			delay(200);
#endif
		}
		n++;
	}

	// draw a box with subtractive color
	u8g2.setDrawColor(2);
	u8g2.drawBox(0, 0, 128, 64);
	u8g2.sendBuffer();

#ifdef do_sound
	sound(3000, 150);
#else
	delay(150);
#endif
}

void displayConfig()
{
	// screen for the wifi configuration mode

	displayClear();

	u8g2.setFont(u8g2_font_nine_by_five_nbp_t_all);
	u8g2.drawStr(15, 12, "WI-FI SETUP");
	u8g2.drawStr(3, 32, "Please, connect to");
	u8g2.drawStr(3, 47, "the \"E-TKT\" network...");

	u8g2.setFont(u8g2_font_open_iconic_all_1x_t);
	u8g2.drawGlyph(3, 12, 0x011a);

	u8g2.sendBuffer();
}

void displayReset()
{
	// screen for the wifi configuration reset confirmation

	displayClear();

	u8g2.setFont(u8g2_font_nine_by_five_nbp_t_all);
	u8g2.drawStr(15, 12, "WI-FI RESET");
	u8g2.drawStr(3, 32, "Connection cleared!");
	u8g2.drawStr(3, 47, "Release the button.");

	u8g2.setFont(u8g2_font_open_iconic_all_1x_t);
	u8g2.drawGlyph(3, 12, 0x00cd);

	u8g2.sendBuffer();
}

void displayQRCode()
{
	// main screen with qr code, network and attributed ip

	displayClear();
	u8g2.setFont(u8g2_font_nine_by_five_nbp_t_all);

	uint8_t qrcodeData[qrcode_getBufferSize(QRcode_Version)];

	if (displayIP != "")
	{
		u8g2.setDrawColor(1);

		u8g2.drawStr(14, 15, "E-TKT");
		u8g2.setDrawColor(2);
		u8g2.drawFrame(3, 3, 50, 15);
		u8g2.setDrawColor(1);

		u8g2.drawStr(14, 31, "ready");

		String resizeSSID;
		if (displaySSID.length() > 8)
		{
			resizeSSID = displaySSID.substring(0, 7) + "...";
		}
		else
		{
			resizeSSID = displaySSID;
		}
		const char *d = resizeSSID.c_str();
		u8g2.drawStr(14, 46, d);

		const char *b = displayIP.c_str();
		u8g2.drawStr(3, 61, b);

		u8g2.setFont(u8g2_font_open_iconic_all_1x_t);
		u8g2.drawGlyph(3, 46, 0x00f8);
		u8g2.drawGlyph(3, 31, 0x0073);

		String ipFull = "http://" + displayIP;
		const char *c = ipFull.c_str();
		qrcode_initText(&qrcode, qrcodeData, QRcode_Version, QRcode_ECC, c);

		// qr code background
		for (uint8_t y = 0; y < 64; y++)
		{
			for (uint8_t x = 0; x < 64; x++)
			{
				u8g2.setDrawColor(0);
				u8g2.drawPixel(x + 128 - 64, y);
			}
		}

		// setup the top right corner of the QRcode
		uint8_t x0 = 128 - 64 + 6;
		uint8_t y0 = 3;

		// display QRcode
		for (uint8_t y = 0; y < qrcode.size; y++)
		{
			for (uint8_t x = 0; x < qrcode.size; x++)
			{

				int newX = x0 + (x * 2);
				int newY = y0 + (y * 2);

				if (qrcode_getModule(&qrcode, x, y))
				{
					u8g2.setDrawColor(1);
					u8g2.drawBox(newX, newY, 2, 2);
				}
				else
				{
					u8g2.setDrawColor(0);
					u8g2.drawBox(newX, newY, 2, 2);
				}
			}
		}
	}
	u8g2.sendBuffer();
	delay(1000);
}

/**
 * Renders the printing progress screen, where "progress" is the number of
 * characters already printed.
 */
void displayProgress(float progress, String label)
{
	displayClear();

	// Show "⚙️ PRINTING" header.
	u8g2.setDrawColor(1);
	u8g2.setFont(u8g2_font_nine_by_five_nbp_t_all);
	u8g2.drawStr(15, 12, "PRINTING");
	u8g2.setFont(u8g2_font_open_iconic_all_1x_t);
	u8g2.drawGlyph(3, 12, 0x0081);

	auto labelLength = utf8Length(label);
	int progress_width = 0;
	int total_width = 0;

	// Do a pass thorugh the label characters to see how much horizontal
	// space is needed to render all the characters and how wide the completed
	// progress bar will be.
	for (int i = 0; i < labelLength; i++)
	{
		auto character = utf8CharAt(label, i);
		int width = 7;
		if (glyphs.count(character) != 0)
		{
			// Its a glyph, so use its custom width.
			width = std::get<2>(glyphs.at(character));
		}
		total_width += width;
		if (i < progress)
		{
			progress_width += width;
		}
	}

	// Calculate the render offset, which keeps the currently printing location
	// visible if the label doesn't fit all on screen.
	int render_offset = 0;
	if (total_width > Lcd_X)
	{
		// If the "progress" location is off screen, offset the rendered label
		// so the progress indicator is centered.
		if (progress_width > Lcd_X / 2)
		{
			render_offset = progress_width - Lcd_X / 2;
		}

		// If centering the progress location woudl cause the right side of the
		// label to render before the right edge of the screen then realign so
		// it does, simulating a scrolling box's bounds.
		if (total_width - render_offset < Lcd_X)
		{
			render_offset = total_width - Lcd_X;
		}
	}

	// Iterate through the label again, this time drawing it on screen.  For
	// simplicity's sake always draw the entire label (even if its of screen)
	// and just let the screen buffer clip the edges.
	int x_position = 0;
	const int y_position = 36;
	for (int i = 0; i < labelLength; i++)
	{
		auto character = utf8CharAt(label, i);
		int width = 7;
		if (glyphs.count(character) == 0)
		{
			// print the character as a normal string.
			u8g2.setFont(u8g2_font_6x13_te);
			u8g2.drawStr(x_position - render_offset, y_position, character.c_str());
		}
		else
		{
			// Print the character as a glyph with its custom offsets.
			auto glyph_data = glyphs.at(character);
			// the glyph is a tuple of:
			// <0> font reference
			// <1> glyph index
			// <2> width
			// <3> x offset
			u8g2.setFont(std::get<0>(glyph_data));
			auto char_x = x_position + std::get<3>(glyph_data) - render_offset;
			auto char_y = y_position + std::get<4>(glyph_data);
			u8g2.drawGlyph(char_x, char_y, std::get<1>(glyph_data));
			width = std::get<2>(glyph_data);
		}
		x_position += width;
	}

	if (progress_width > 0)
	{
		// Render an inverted-color rectangle over the completed characters.
		u8g2.setDrawColor(2);
		u8g2.drawBox(0 - render_offset, 21, progress_width - 1, 21);
	}

	// Draw a box around the text, which looks like the edges of a label.
	u8g2.setDrawColor(1);
	u8g2.drawFrame(0 - render_offset, 21, total_width, 22);

	// If needed, draw ellipses on the right side of the screen to indicate the
	// label continues.
	if (total_width - render_offset > Lcd_X)
	{
		u8g2.setDrawColor(0);

		// Clear 14 pixels of space on the right side of the screen.
		u8g2.drawBox(Lcd_X - 14, 21, 14, 22);

		// Draw a triplet of 2x2 pixel dots, "...", in the middle of the
		// text line.
		u8g2.setDrawColor(1);
		for (int i = 1; i <= 3; i++)
		{
			u8g2.drawBox(Lcd_X - (i * 4) + 2, 31, 2, 2);
		}
	}

	// Update the progress reported to the web app.
	webProgress = 100 * progress / labelLength;
	if (webProgress > 0)
	{
		webProgress -= 1; // avoid 100% progress while still finishing
	}

	// Print "XX%" at the bottom of the screen.
	String progressString = String(webProgress, 0);
	progressString.concat("%");
	u8g2.setDrawColor(1);
	u8g2.drawStr(6, 60, progressString.c_str());

	u8g2.sendBuffer();
}

void displayFinished()
{
	// screen with finish confirmation

	displayClear(1);
	u8g2.setDrawColor(0);
	u8g2.setFont(u8g2_font_nine_by_five_nbp_t_all);

	u8g2.drawStr(42, 37, "FINISHED!");

	u8g2.setFont(u8g2_font_open_iconic_all_1x_t);
	u8g2.drawGlyph(27, 37, 0x0073);
	u8g2.drawGlyph(90, 37, 0x0073);

	u8g2.sendBuffer();
}

void displayCut()
{
	// screen for manual cut mode

	displayClear(0);
	u8g2.setDrawColor(1);
	u8g2.setFont(u8g2_font_nine_by_five_nbp_t_all);
	u8g2.drawStr(44, 37, "CUTTING");

	u8g2.setFont(u8g2_font_open_iconic_all_1x_t);
	u8g2.drawGlyph(26, 37, 0x00f2);
	u8g2.drawGlyph(90, 37, 0x00f2);

	u8g2.sendBuffer();
}

void displayFeed()
{
	// screen for manual feed mode

	displayClear(0);
	u8g2.setDrawColor(1);
	u8g2.setFont(u8g2_font_nine_by_five_nbp_t_all);
	u8g2.drawStr(44, 37, "FEEDING");

	u8g2.setFont(u8g2_font_open_iconic_all_1x_t);
	u8g2.drawGlyph(26, 37, 0x006e);
	u8g2.drawGlyph(90, 37, 0x006e);

	u8g2.sendBuffer();
}

void displayReel()
{
	// screen for reeling mode

	displayClear(0);
	u8g2.setDrawColor(1);
	u8g2.setFont(u8g2_font_nine_by_five_nbp_t_all);
	u8g2.drawStr(44, 37, "REELING");

	u8g2.setFont(u8g2_font_open_iconic_all_1x_t);
	u8g2.drawGlyph(26, 37, 0x00d5);
	u8g2.drawGlyph(90, 37, 0x00d5);

	u8g2.sendBuffer();
}

void displayTest(int a, int f)
{
	// screen for settings test mode

	displayClear(0);
	u8g2.setDrawColor(1);
	u8g2.setFont(u8g2_font_nine_by_five_nbp_t_all);
	u8g2.drawStr(44, 37, "TESTING");

	u8g2.setFont(u8g2_font_open_iconic_all_1x_t);
	u8g2.drawGlyph(26, 37, 0x0073);
	u8g2.drawGlyph(90, 37, 0x0073);

	u8g2.sendBuffer();
}

void displaySettings(int a, int f)
{
	// screen for settings save mode

	displayClear(0);
	u8g2.setDrawColor(1);
	u8g2.setFont(u8g2_font_nine_by_five_nbp_t_all);
	u8g2.drawStr(47, 17, "SAVED!");

	String alignString = "ALIGN: ";
	alignString.concat(a);
	const char *alignChar = alignString.c_str();
	u8g2.drawStr(44, 37, alignChar);

	String forceString = "FORCE: ";
	forceString.concat(f);
	const char *forceChar = forceString.c_str();
	u8g2.drawStr(42, 57, forceChar);

	u8g2.setFont(u8g2_font_open_iconic_all_1x_t);
	u8g2.drawGlyph(33, 16, 0x0073);
	u8g2.drawGlyph(83, 16, 0x0073);

	u8g2.sendBuffer();
	delay(3000);
}

void displayReboot()
{
	// screen for imminent reboot

	displayClear(0);

	u8g2.setFont(u8g2_font_nine_by_five_nbp_t_all);
	u8g2.drawStr(38, 37, "REBOOTING...");

	u8g2.sendBuffer();
	delay(2000);
}

void debugDisplay()
{
	// displaySplash();
	// delay(2000);

	// displayReset();
	// delay(2000);

	// displayConfig();
	// delay(2000);

	// displayIP = "192.168.69.69";
	// displaySSID = "Network";
	// displayQRCode();
	// delay(2000);

	// displayProgress(5, " E-TKT ");
	// delay(5000);

	// displayFinished();
	// delay(2000);

	// displayCut();
	// delay(2000);

	// displayFeed();
	// delay(2000);

	// displayReel();
	// delay(2000);

	// displaySettings(5, 2);
	// delay(2000);

	// displayReboot();
	// delay(2000);
}

#endif  //DISPLAY64