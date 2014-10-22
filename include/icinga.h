/*
 * filename: icinga.h
 *
 * This file contains some defines which are used in other plugins by this
 * project.
 */

#ifndef __icinga_h
#define __icinga_h

#define OK          0
#define WARNING     1
#define CRITICAL    2
#define UNKNOWN     3

const char  state[4][9] = {
                "OK",
                "WARNING",
                "CRITICAL",
                "UNKNOWN"};

#endif
