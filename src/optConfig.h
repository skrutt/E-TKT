// OPTIONAL CONFIGURATION ---------------------------------------------------------

// Depending on the hardware you've used to build your E-TKT, you might need to enable some of these constants
// to get the hardware into a working state

// If your feed motor moves in the wrong direction by default, uncomment the define below to reverse it.
// It should be obvious if this is happening since the tape gets fed in the wrong direction.

 #define REVERSE_FEED_STEPPER_DIRECTION true
 
 // If your char motor moves in the wrong direction by default, uncomment the define below to reverse it.
// If your print is wrong and you see mostly counterclockwise spinning 

 #define REVERSE_CHAR_STEPPER_DIRECTION true

// If your hall sensor has inverted logic (eg active LOW and neutral HIGH) uncomment this #define to invert the
// logic checking it. If you're affected by this then you'll see the character carousel move forward slightly
// and then stop when the E-TKT starts up instead of moving to the "J" position.

// For instance, invert if using a "3144" hall sensor but don't invert if using a "44E 402" hall sensor.

//  #define INVERT_HALL_SENSOR_LOGIC true

//if using a microswitch instead. NOTE THAT PIN34 DOES NOT SUPPORT INTERNAL PULLUPS!
#define NO_HALL_SENSOR 1

//Choose display 
#define DISPLAY32
//#define DISPLAY64