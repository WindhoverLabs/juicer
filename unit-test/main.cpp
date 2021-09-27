/*
 * main.cpp
 *
 *  Created on: Aug 12, 2020
 *      Author: lgomez
 *      Email:  lgomez@windhoverlabs.com
 */

/*This tells Catch to provide a main() - only do this in one cpp file*/
#define CATCH_CONFIG_MAIN


/**This disables coloring output so that Eclipse CDT(Version: 9.7.0.201903092251)
 *will be able to render it. If you really like colored output, you'll have to use
 *something else other than Eclipse's console(such as GNOME shell) to run the tests
 *and comment out the CATCH_CONFIG_COLOUR_NONE macro.
 */
//#define CATCH_CONFIG_COLOUR_NONE

#include "catch.hpp"
