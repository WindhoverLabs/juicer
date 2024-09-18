/****************************************************************************
 *
 *   Copyright (c) 2017 Windhover Labs, L.L.C. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name Windhover Labs nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *****************************************************************************/

#include <argp.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <iostream>

#include "IDataContainer.h"
#include "Juicer.h"
#include "Logger.h"
#include "SQLiteDB.h"
#include "TestSymbolsA.h"
#include "TestSymbolsB.h"

const char *argp_program_version     = "juicer 0.1";
const char *argp_program_bug_address = "<mbenson@windhoverlabs.com>";

/* Program documentation. */
static char doc[] =
    "Juicer reads a binary file in ELF format, and generates a "
    "report containing the ABI and bit patterns of structures.";

/* A description of the arguments we accept. */
static char args_doc[] =
    "--input <FILE> --mode <MODE> (--output <FILE> | "
    "(--address <ADDR> --port <PORT> --project <PROJ>)) -x -gN";

/* The options we understand. */
static struct argp_option options[] = {{"input", 'i', "FILE", 0, "Input ELF file"},
                                       {"verbosity", 'v', "LEVEL", 0,
                                        "Set verbosity LEVEL, 0-4 (default 1). "
                                        "0=Silent, 1=Errors, 2=Warnings, 3=Info, "
                                        "4=Debug"},
                                       {"log", 'l', "FILE", 0, "Output log FILE"},
                                       {"mode", 'm', "MODE", 0, "Output mode.  SQLITE,CCDD"},
                                       {"output", 'o', "FILE", 0, "Sqlite3 database FILE.  Required for SQLITE mode."},
                                       {"address", 'a', "ADDRESS", 0, "Postgresql server address.  Required for CCDD mode."},
                                       {"port", 'p', "PORT", 0, "Postgresql server port.  Required for CCDD mode."},
                                       {"user", 'u', "USER", 0, "Postgresql user.  Required for CCDD mode."},
                                       {"project", 'j', "PROJECT", 0, "Postgresql CCDD project.  Required for CCDD mode."},
                                       {"extras", 'x', NULL, 0,
                                        "Extra DWARF and ELF data such as variables. Enabling this"
                                        "will cause juicer to take longer."},
                                        {"groupNumber", 'g', "group", 0,
                                        "Group number to extract data forom inside of DWARF section."
                                        "Useful for situations where debug sections (eg. debug_macros) are spreadout through different groups."
                                        " An example of this is when macros are split in different groups by gcc for unlinked ELF object files."},
                                       {0}};

/* Used by main to communicate with parse_opt. */
typedef struct
{
    char              *input;
    bool               input_set;
    int                verbosity;
    bool               verbosity_set;
    char              *log;
    bool               log_set;
    char              *outputMode;
    JuicerOutputMode_t outputModeEnum;
    bool               outputMode_set;
    char              *output;
    bool               output_set;
    char              *address;
    bool               address_set;
    int                port;
    bool               port_set;
    char              *user;
    bool               user_set;
    char              *project;
    bool               project_set;
    bool               extras;
    unsigned int       groupNumber;
} arguments_t;

/* Parse a single option. */
static error_t parse_opt(int key, char *arg, struct argp_state *state)
{
    /* Get the input argument from argp_parse, which we
     know is a pointer to our arguments structure. */
    arguments_t *arguments = (arguments_t *)state->input;

    switch (key)
    {
        case 'i':
        {
            arguments->input     = arg;
            arguments->input_set = true;
            break;
        }

        case 'v':
        {
            arguments->verbosity     = atoi(arg);
            arguments->verbosity_set = true;
            break;
        }

        case 'l':
        {
            arguments->log     = arg;
            arguments->log_set = true;
            break;
        }

        case 'm':
        {
            arguments->outputMode     = arg;
            arguments->outputMode_set = true;
            break;
        }

        case 'o':
        {
            arguments->output     = arg;
            arguments->output_set = true;
            break;
        }

        case 'a':
        {
            arguments->address     = arg;
            arguments->address_set = true;
            break;
        }

        case 'p':
        {
            arguments->port     = atoi(arg);
            arguments->port_set = true;
            break;
        }

        case 'u':
        {
            arguments->user     = arg;
            arguments->user_set = true;
            break;
        }

        case 'j':
        {
            arguments->project     = arg;
            arguments->project_set = true;

            break;
        }

        case 'x':
        {
            arguments->extras = true;
            break;
        }

        case 'g':
        {
            arguments->groupNumber = atoi(arg);
            break;
        }

        case ARGP_KEY_ARG:
        {
            //    	    if (state->arg_num >= 2)
            //    	    {
            //                /* Too many arguments. */
            //                argp_usage (state);
            //    	    }
            //            arguments->args[state->arg_num] = arg;

            break;
        }

        case ARGP_KEY_END:
        {
            /* Verify arguments, starting with the input file. */
            if (arguments->input_set == false)
            {
                printf("Error:  Input file not set.\n");
                argp_usage(state);
                return ARGP_KEY_ERROR;
            }

            /* Verify verbosity. */
            if (arguments->verbosity_set)
            {
                if ((arguments->verbosity < LOGGER_VERBOSITY_SILENT) || (arguments->verbosity > LOGGER_VERBOSITY_DEBUG))
                {
                    printf("Error:  Invalid verbosity.\n");
                    argp_usage(state);
                    return ARGP_KEY_ERROR;
                }
            }

            /* Verify output mode and convert the string to an enumeration. */
            if (arguments->outputMode_set)
            {
                if (strcmp(arguments->outputMode, "SQLITE") == 0)
                {
                    arguments->outputModeEnum = JUICER_OUTPUT_MODE_SQLITE;
                }

                if (strcmp(arguments->outputMode, "CCDD") == 0)
                {
                    arguments->outputModeEnum = JUICER_OUTPUT_MODE_CCDD;
                }

                if (arguments->outputModeEnum == JUICER_OUTPUT_MODE_UNKNOWN)
                {
                    /* The output mode provided was neither SQLITE nor CCDD. */
                    printf("Error:  Invalid output mode.\n");
                    argp_usage(state);
                    return ARGP_KEY_ERROR;
                }
            }
            else
            {
                printf("Error:  Output mode is required.\n");
                argp_usage(state);
                return ARGP_KEY_ERROR;
            }

            /* Now verify the output mode specific arguments. */
            if (JUICER_OUTPUT_MODE_SQLITE == arguments->outputModeEnum)
            {
                if (false == arguments->output_set)
                {
                    printf("Error:  Output file must be set when mode is set to SQLITE.\n");
                    argp_usage(state);
                    return ARGP_KEY_ERROR;
                }
            }
            else if (JUICER_OUTPUT_MODE_CCDD == arguments->outputModeEnum)
            {
                if (false == arguments->address_set)
                {
                    printf(
                        "Error:  Postgresql server address must be set when mode is set "
                        "to CCDD.\n");
                    argp_usage(state);
                    return ARGP_KEY_ERROR;
                }
                else if (false == arguments->port_set)
                {
                    printf(
                        "Error:  Postgresql server port must be set when mode is set to "
                        "CCDD.\n");
                    argp_usage(state);
                    return ARGP_KEY_ERROR;
                }
                else if (false == arguments->user_set)
                {
                    printf("Error:  Postgresql user must be set when mode is set to CCDD.\n");
                    argp_usage(state);
                    return ARGP_KEY_ERROR;
                }
                else if (false == arguments->project_set)
                {
                    printf(
                        "Error:  CCDD project name must be set when mode is set to "
                        "CCDD.\n");
                    argp_usage(state);
                    return ARGP_KEY_ERROR;
                }
            }

            //            /* Verify Extras. */
            //            if (arguments->extras)
            //            {
            //                //                printf("Error:  extras name must be set.\n");
            //                //                argp_usage(state);
            //                return ARGP_KEY_ERROR;
            //            }

            break;
        }

        default:
        {
            return ARGP_ERR_UNKNOWN;
        }
    }

    return 0;
}

/* Our argp parser. */
static struct argp argp         = {options, parse_opt, args_doc, doc};

int                main(int argc, char **argv)
{
    arguments_t arguments;
    error_t     parse_error;

    /* Set argument default values. */
    memset(&arguments, 0, sizeof(arguments));
    arguments.verbosity = 1;
    arguments.extras    = false;
    arguments.groupNumber = 0;

    /* Parse our arguments; every option seen by parse_opt will
     be reflected in arguments. */
    parse_error         = argp_parse(&argp, argc, argv, 0, 0, &arguments);
    if (parse_error == 0)
    {
        Juicer juicer;
        juicer.setExtras(arguments.extras);
        juicer.setGroupNumber(arguments.groupNumber);
        IDataContainer *idc    = 0;

        Logger          logger = Logger(arguments.verbosity);

        if (arguments.log_set)
        {
            logger.setLogFile(arguments.log);
            logger.logDebug("Log '%s' started", arguments.log);
        }

        logger.logDebug("Verbosity %u", arguments.verbosity);
        logger.logDebug("Input file '%s'", arguments.input);
        logger.logDebug("Output Mode %s", arguments.outputMode);
        if (arguments.outputModeEnum == JUICER_OUTPUT_MODE_SQLITE)
        {
            logger.logDebug("SQLITE output file '%s'", arguments.output);

            idc = IDataContainer::Create(IDC_TYPE_SQLITE, arguments.output);
        }
        else if (arguments.outputModeEnum == JUICER_OUTPUT_MODE_CCDD)
        {
            logger.logDebug("CCDD address '%s'", arguments.address);
            logger.logDebug("CCDD port '%i'", arguments.port);
            logger.logDebug("CCDD project '%s'", arguments.project);

            idc = IDataContainer::Create(IDC_TYPE_CCDD, "%s:%i:%s", arguments.address, arguments.port, arguments.project);
        }

        juicer.setIDC(idc);

        logger.logInfo("Parsing");

        std::string modernInput{arguments.input};

        juicer.parse(modernInput);

        logger.logInfo("Done");
    }
    else
    {
        /* An argument was wrong.  Return an error. */
        return (-1);
    }
    /*Initialize the logger*/
}
