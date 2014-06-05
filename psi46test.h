/* -------------------------------------------------------------
 *
 *  file:        psi46test.h
 *
 *  description: globals for PSI46V2 Wafer tester
 *
 *  author:      Beat Meier
 *  modified:    7.8.2013
 *
 *  rev:
 *
 * -------------------------------------------------------------
 */

#ifndef PSI46TEST_H
#define PSI46TEST_H

#include "config.h"
#include "pixel_dtb.h"
#include "settings.h"
#include "prober.h"
#include "protocol.h"
#include "pixelmap.h"
#include "test.h"
#include "chipdatabase.h"
#include "Nucleus.h"                   // CG
#define DEBUG_NODTB false               // CG

#define VERSIONINFO TITLE " " VERSION " (" TIMESTAMP ")"


// global variables
extern int nEntry; // counts the entries in the log file

extern CTestboard tb;
extern CSettings settings;  // global settings
extern CProber prober; // prober
extern CProtocol Log;  // log file
extern CNucleus prober_nucleus;        // CG

extern CChip g_chipdata;

void cmd();


#endif
