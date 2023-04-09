
#ifndef LABELMAKER_H
#define LABELMAKER_H

// extension files
#include "pitches.h"	// list of notes and their frequencies

#include "optConfig.h" // opt-in external file for configuring motor direction and hall sensor logic


// char
#define charQuantity 43 // the amount of teeth/characters in the carousel

    
float getwebProgress();
String getSSID();
String getIP();

#endif