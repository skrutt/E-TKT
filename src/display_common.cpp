
#include "display.h"

// --------------------------------------------------------------------------------
// UTF-8 String handling

// Returns the number of bytes making up the UTF-8 character starting at char index
// "position" of str.  For an explanation of the magic constants, please see:
// https://en.wikipedia.org/wiki/UTF-8#Encoding
int utf8CharLength(String str, int position)
{
	int start = str[position];
	if (start >> 3 == 30)
	{
		return 4;
	}
	else if (start >> 4 == 14)
	{
		return 3;
	}
	else if (start >> 5 == 6)
	{
		return 2;
	}
	else
	{
		return 1;
	}
}

// Returns the length of a UTF-8 encoded string, treating each UTF-8 code-point
// as a single character.
int utf8Length(String str)
{
	int position = 0;
	int length = 0;
	while (position < str.length())
	{
		position += utf8CharLength(str, position);
		length++;
	}
	return length;
}

// Returns the UTF-8 characters at the given position of the given string,
// treating multi-character code points as inidividual characters.
String utf8CharAt(String str, int position)
{
	int current = 0;
	while (current < str.length() && position > 0)
	{
		current += utf8CharLength(str, current);
		position--;
	}
	int charLength = utf8CharLength(str, current);
	return str.substring(current, current + charLength);
}