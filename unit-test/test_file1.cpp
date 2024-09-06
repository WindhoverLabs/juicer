/*
 * test_file.cpp
 *
 *  Created on: Jul 29, 2020
 *      Author: vagrant
 *This file is used by the testing suite. It is meant to be used as an
 *object file that will be executed by Juicer and checked for correctness
 *on our unit tests.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "test_file1.h"

#include <fcntl.h>
#include <libdwarf.h>
#include <libelf.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

Square                                         sq              = {};
Circle                                         ci              = {};

__attribute__((used)) CFE_ES_HousekeepingTlm_t hk              = {};

/*
** Local Structure Declarations
*/
static __attribute__((used)) CFE_TBL_FileDef_t CFE_TBL_FileDef = {
    /* Content format: ObjName[64], TblName[38], Desc[32], TgtFileName[20], ObjSize
    **    ObjName - variable name of config table, e.g., CI_ConfigDefTbl[]
    **    TblName - app's table name, e.g., CI.CONFIG_TBL, where CI is the same app name
    **              used in cfe_es_startup.scr, and CI_defConfigTbl is the same table
    **              name passed in to CFE_TBL_Register()
    **    Desc - description of table in string format
    **    TgtFileName[20] - table file name, compiled as .tbl file extension
    **    ObjSize - size of the entire table
    */

    "ADC_ConfigTbl", "ADC.CONFIG_TBL", "ADC default config table", "adc_config.tbl", (sizeof(CFE_ES_HousekeepingTlm_t))};

int            vector_x               = 100;

unsigned int   some_unsiged_int       = 12;

int8_t         precise_int8           = 110;

int16_t        precise_int16          = 110;

int32_t        precise_int32          = 110;

int64_t        precise_int64          = 110;

uint8_t        precise_unsigned_int8  = 112;

uint16_t       precise_unsigned_int16 = 112;

uint32_t       precise_unsigned_int32 = 112;

uint64_t       precise_unsigned_int64 = 112;

char           character              = '2';

int            flat_array[]           = {1, 2, 3, 4, 5, 6};

float          some_float             = 1.5;

short          some_short             = 20;

unsigned short some_signed_short      = 14;

long           a_long_value           = 0;

long long      a_very_long_value      = 0;

double         some_double            = 4.5;

int            vector_y               = 30;

char           alphabet[]             = {'a', 'b', 'c'};

enum Color     rainbow                = RED;

int            another_array[]        = {20, 21, 22, 34};
#ifdef __cplusplus
}
#endif
