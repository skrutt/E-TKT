
#include "display.h"

#ifdef DISPLAY32

// oled display
U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);
#define LCD_X 128
#define LCD_Y 32
#define MIDDLE_ROW 20

QRCode qrcode;				  //  create the QR code

// --------------------------------------------------------------------------------
// DISPLAY ------------------------------------------------------------------------

void displayClear(int color)
{
	// paints all pixels according to the desired target color

	for (uint8_t y = 0; y < LCD_Y; y++)
	{
		for (uint8_t x = 0; x < LCD_X; x++)
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

	u8g2.setFont(u8g2_font_squeezed_r7_tr);
	u8g2.setDrawColor(0);
	u8g2.drawStr(40, 53, "andrei.cc");
	u8g2.sendBuffer();

	u8g2.setDrawColor(1);

	int n = 1;

	// animated splash
	for (int i = LCD_X; i > 7; i = i - 18)
	{
		for (int j = 0; j < 18; j += 9)
		{
			u8g2.drawXBM(i - j - 11, 0, LCD_X, LCD_Y, etktLogo);
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
	u8g2.drawBox(0, 0, LCD_X, LCD_Y);
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

	u8g2.setFont(u8g2_font_squeezed_r7_tr);
	u8g2.drawStr(15, 10, "WI-FI SETUP");
	u8g2.drawStr(3, MIDDLE_ROW, "Please, connect to");
	u8g2.drawStr(3, 31, "the \"E-TKT\" network...");

	u8g2.setFont(u8g2_font_open_iconic_all_1x_t);
	u8g2.drawGlyph(3, 10, 0x011a);

	u8g2.sendBuffer();
}

void displayReset()
{
	// screen for the wifi configuration reset confirmation

	displayClear();

	u8g2.setFont(u8g2_font_squeezed_r7_tr);
	u8g2.drawStr(15, 10, "WI-FI RESET");
	u8g2.drawStr(3, MIDDLE_ROW, "Connection cleared!");
	u8g2.drawStr(3, 31, "Release the button.");

	u8g2.setFont(u8g2_font_open_iconic_all_1x_t);
	u8g2.drawGlyph(3, 10, 0x00cd);

	u8g2.sendBuffer();
}

// main screen with qr code, network and attributed ip
void displayQRCode()
{

	displayClear();
	u8g2.setFont(u8g2_font_squeezed_r7_tr);

	uint8_t qrcodeData[qrcode_getBufferSize(QRcode_Version)];

	if (getIP() != "")
	{
		u8g2.setDrawColor(2);
		u8g2.drawFrame(0, 0, 50, 12);

		u8g2.setDrawColor(1);
		u8g2.drawStr((50 / 2 - 8), 10, "E-TKT");   //offset half text length from center of the frame

		u8g2.setDrawColor(1);
		u8g2.drawStr(11, MIDDLE_ROW, "ready");

		String resizeSSID;
        String displaySSID = getSSID();
		if (displaySSID.length() > 11)
		{
			resizeSSID = displaySSID.substring(0, 7) + "...";
		}
		else
		{
			resizeSSID = displaySSID;
		}
		const char *SSID_c_str = resizeSSID.c_str();
		u8g2.drawStr(11, 31, SSID_c_str);

        String displayIP = getIP();
		const char *IPc_str = displayIP.c_str();
		u8g2.drawStr(LCD_X / 2 - displayIP.length() * 3 / 2, 31, IPc_str);

		u8g2.setFont(u8g2_font_open_iconic_all_1x_t);
		u8g2.drawGlyph(0, 22, 0x0073);  //checkmark
		u8g2.drawGlyph(0, 31, 0x00f8);  //Wifi symbol

        //QR code

		String ipFull = "http://" + displayIP;
		const char *c = ipFull.c_str();
		qrcode_initText(&qrcode, qrcodeData, QRcode_Version, QRcode_ECC, c);

		// // qr code background
		// for (uint8_t y = 0; y < Lcd_Y; y++)
		// {
		// 	for (uint8_t x = 0; x < Lcd_Y; x++)
		// 	{
		// 		u8g2.setDrawColor(0);
		// 		u8g2.drawPixel(x + 128 - Lcd_Y, y);
		// 	}
		// }

		// setup the top right corner of the QRcode
		uint8_t x0 = LCD_X - LCD_Y + 3;
		uint8_t y0 = 0;

		// display QRcode
		for (uint8_t y = 0; y < qrcode.size; y++)
		{
			for (uint8_t x = 0; x < qrcode.size; x++)
			{

				int newX = x0 + (x );
				int newY = y0 + (y );

				if (qrcode_getModule(&qrcode, x, y))
				{
					u8g2.setDrawColor(1);
					u8g2.drawBox(newX, newY, 1, 1);
				}
				else
				{
					u8g2.setDrawColor(0);
					u8g2.drawBox(newX, newY, 1, 1);
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
	u8g2.setFont(u8g2_font_squeezed_r7_tr);
	u8g2.drawStr(15, 10, "PRINTING");
	u8g2.setFont(u8g2_font_open_iconic_all_1x_t);
	u8g2.drawGlyph(3, 10, 0x0081);

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
	if (total_width > LCD_X)
	{
		// If the "progress" location is off screen, offset the rendered label
		// so the progress indicator is centered.
		if (progress_width > LCD_X / 2)
		{
			render_offset = progress_width - LCD_X / 2;
		}

		// If centering the progress location woudl cause the right side of the
		// label to render before the right edge of the screen then realign so
		// it does, simulating a scrolling box's bounds.
		if (total_width - render_offset < LCD_X)
		{
			render_offset = total_width - LCD_X;
		}
	}

	// Iterate through the label again, this time drawing it on screen.  For
	// simplicity's sake always draw the entire label (even if its of screen)
	// and just let the screen buffer clip the edges.
	int x_position = 0;
	const int y_position = 26;
    
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

    const int labelY = 13;
    const int labelHeight = 17;

	if (progress_width > 0)
	{
		// Render an inverted-color rectangle over the completed characters.
		u8g2.setDrawColor(2);
		u8g2.drawBox(0 - render_offset, labelY, progress_width - 1, labelHeight);
	}

	// Draw a box around the text, which looks like the edges of a label.
	u8g2.setDrawColor(1);
	u8g2.drawFrame(0 - render_offset, labelY, total_width, labelHeight);

	// If needed, draw ellipses on the right side of the screen to indicate the
	// label continues.
	if (total_width - render_offset > LCD_X)
	{
		u8g2.setDrawColor(0);

		// Clear 14 pixels of space on the right side of the screen.
		u8g2.drawBox(LCD_X - 14, labelY, 14, labelHeight);

		// Draw a triplet of 2x2 pixel dots, "...", in the middle of the
		// text line.
		u8g2.setDrawColor(1);
		for (int i = 1; i <= 3; i++)
		{
			u8g2.drawBox(LCD_X - (i * 4) + 2, 20, 2, 2);
		}
	}

	// Print "XX%" at the bottom of the screen.
	String progressString = String(getwebProgress(), 0);
	progressString.concat("%");
	u8g2.setDrawColor(1);
	u8g2.drawStr(110, 10, progressString.c_str());

	u8g2.sendBuffer();
}

void displayFinished()
{
	// screen with finish confirmation

	displayClear(1);
	u8g2.setDrawColor(0);
	u8g2.setFont(u8g2_font_squeezed_r7_tr);

	u8g2.drawStr(50, MIDDLE_ROW, "FINISHED!");

	u8g2.setFont(u8g2_font_open_iconic_all_1x_t);
	u8g2.drawGlyph(27, MIDDLE_ROW, 0x0073);
	u8g2.drawGlyph(90, MIDDLE_ROW, 0x0073);

	u8g2.sendBuffer();
}

void displayCut()
{
	// screen for manual cut mode

	displayClear(0);
	u8g2.setDrawColor(1);
	u8g2.setFont(u8g2_font_squeezed_r7_tr);
	u8g2.drawStr(54, MIDDLE_ROW, "CUTTING");

	u8g2.setFont(u8g2_font_open_iconic_all_1x_t);
	u8g2.drawGlyph(26, MIDDLE_ROW, 0x00f2);
	u8g2.drawGlyph(90, MIDDLE_ROW, 0x00f2);

	u8g2.sendBuffer();
}

void displayFeed()
{
	// screen for manual feed mode

	displayClear(0);
	u8g2.setDrawColor(1);
	u8g2.setFont(u8g2_font_squeezed_r7_tr);
	u8g2.drawStr(54, MIDDLE_ROW, "FEEDING");

	u8g2.setFont(u8g2_font_open_iconic_all_1x_t);
	u8g2.drawGlyph(26, MIDDLE_ROW, 0x006e);
	u8g2.drawGlyph(90, MIDDLE_ROW, 0x006e);

	u8g2.sendBuffer();
}

void displayReel()
{
	// screen for reeling mode

	displayClear(0);
	u8g2.setDrawColor(1);
	u8g2.setFont(u8g2_font_squeezed_r7_tr);
	u8g2.drawStr(54, MIDDLE_ROW, "REELING");

	u8g2.setFont(u8g2_font_open_iconic_all_1x_t);
	u8g2.drawGlyph(26, MIDDLE_ROW, 0x00d5);
	u8g2.drawGlyph(90, MIDDLE_ROW, 0x00d5);

	u8g2.sendBuffer();
}

void displayTest(int a, int f)
{
	// screen for settings test mode

	displayClear(0);
	u8g2.setDrawColor(1);
	u8g2.setFont(u8g2_font_squeezed_r7_tr);
	u8g2.drawStr(54, MIDDLE_ROW, "TESTING");

	u8g2.setFont(u8g2_font_open_iconic_all_1x_t);
	u8g2.drawGlyph(26, MIDDLE_ROW, 0x0073);
	u8g2.drawGlyph(90, MIDDLE_ROW, 0x0073);

	u8g2.sendBuffer();
}

void displaySettings(int a, int f)
{
	// screen for settings save mode

	displayClear(0);
	u8g2.setDrawColor(1);
	u8g2.setFont(u8g2_font_squeezed_r7_tr);
	u8g2.drawStr(47, 10, "SAVED!");

	String alignString = "ALIGN: ";
	alignString.concat(a);
	const char *alignChar = alignString.c_str();
	u8g2.drawStr(44, MIDDLE_ROW, alignChar);

	String forceString = "FORCE: ";
	forceString.concat(f);
	const char *forceChar = forceString.c_str();
	u8g2.drawStr(42, 30, forceChar);

	u8g2.setFont(u8g2_font_open_iconic_all_1x_t);
	u8g2.drawGlyph(33, 10, 0x0073);
	u8g2.drawGlyph(83, 10, 0x0073);

	u8g2.sendBuffer();
	delay(3000);
}

void displayReboot()
{
	// screen for imminent reboot

	displayClear(0);

	u8g2.setFont(u8g2_font_squeezed_r7_tr);
	u8g2.drawStr(38, MIDDLE_ROW, "REBOOTING...");

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

#endif // DISPLAY32