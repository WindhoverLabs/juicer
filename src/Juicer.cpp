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

#include "Juicer.h"
#include <string.h>
#include <errno.h>
#include <libelf.h>
#include <ctype.h>
#include <memory.h>
#include "IDataContainer.h"
#include "Symbol.h"
#include "Field.h"
#include "Enumeration.h"
#include "BitField.h"
#include "ElfFile.h"

Juicer::Juicer()
{
}

/**
 * Iterates through the CU lists of the dbg.
 */
int Juicer::readCUList(ElfFile& elf, Dwarf_Debug dbg)
{
    Dwarf_Unsigned cu_header_length = 0;
    Dwarf_Half version_stamp = 0;
    Dwarf_Unsigned abbrev_offset = 0;
    Dwarf_Half address_size = 0;
    Dwarf_Unsigned next_cu_header = 0;
    Dwarf_Error error = 0;
    int cu_number = 0;
    int return_value = JUICER_OK;

    while(1)
    {
        Dwarf_Die no_die = 0;
        Dwarf_Die cu_die = 0;
        int res = DW_DLV_ERROR;

        ++cu_number;

        logger.logDebug("Reading CU %u.", cu_number);

        res = dwarf_next_cu_header(dbg, &cu_header_length, &version_stamp,
                &abbrev_offset, &address_size, &next_cu_header, &error);
        if(res == DW_DLV_ERROR)
        {
            logger.logError("Error in dwarf_next_cu_header. errno=%u %s", dwarf_errno(error),
                    dwarf_errmsg(error));
            return_value = JUICER_ERROR;
        }
        else if(res == DW_DLV_NO_ENTRY)
        {
            /* Done. */
            return_value = JUICER_OK;
            break;
        }

        if(JUICER_OK == return_value)
        {
            /* The CU will have a single sibling, a cu_die. */
            res = dwarf_siblingof(dbg, no_die, &cu_die, &error);
            if(res == DW_DLV_ERROR)
            {
                logger.logError("Error in dwarf_siblingof on CU die. errno=%u %s", dwarf_errno(error),
                    dwarf_errmsg(error));
                return_value = JUICER_ERROR;
            }
            else if(res == DW_DLV_NO_ENTRY)
            {
                /* Impossible case. */
                logger.logError("no entry! in dwarf_siblingof on CU die. errno=%u %s", dwarf_errno(error),
                    dwarf_errmsg(error));
                return_value = JUICER_ERROR;
            }
        }

        if(JUICER_OK == return_value)
        {
            return_value = getDieAndSiblings(elf, dbg, cu_die, 0);
        }

        if(JUICER_OK != return_value)
        {
        	logger.logError("Error on siblings func");
        }

        dwarf_dealloc(dbg, cu_die, DW_DLA_DIE);
    }
    return return_value;
}


char * Juicer::dwarfStringToChar(char *dwarfString)
{
    uint32_t length = strlen(dwarfString);
    char *inOut = 0;

    for(uint32_t i = 0; i < length; ++i)
    {
        if(!isdigit(dwarfString[i]))
        {
            inOut = &dwarfString[i];
            break;
        }
    }

    return inOut;
}

/**
 * @brief Processes an array that belongs to the die inDie.
 *
 *@param elf The elf to write the new array symbol to.
 *@param dbg the Dwarf Debug section structure
 *@param inDie the die that contains the array.
 *
 * @todo Store the array data(its name, size, etc) in the elf for the
 * database to store it. Make sure to add a "multiplicity" column to the
 * symbols table.
 *
 * @return If the array symbol is added successfully to the elf, then DW_DLV_OK is returned.
 * Otherwise, DW_DLV_ERROR is returned. Please note that just because DW_DLV_ERROR is returned
 * it does NOT mean that no array was found. There are cases where an array is found on the die,
 * however, because it has no name we decide to not add it to the elf at all.
 */
int Juicer::process_DW_TAG_array_type(ElfFile& elf, Symbol &symbol, Dwarf_Debug dbg, Dwarf_Die inDie)
{
	Dwarf_Die 		dieSubrangeType;
	Dwarf_Unsigned 	dwfUpperBound = 0;
	uint32_t 		multiplicity = 0;
	Dwarf_Error     error = 0;
	Dwarf_Attribute attr_struct = 0;
	char* 			arrayName = nullptr;
	int 			res = 0;
	Dwarf_Die 		sib_die = 0;
	Symbol* 		outSymbol  = nullptr;

	return res;

	/* Now lets get the array size.  Get the array size by getting
	 * the first child, which should be the subrange_type. */
		res = dwarf_child(inDie, &dieSubrangeType, &error);
		if(res == DW_DLV_ERROR)
		{
			logger.logError("Error in dwarf_child. errno=%u %s", dwarf_errno(error),
					dwarf_errmsg(error));
		}

     DisplayDie(inDie);

	/* Make sure this is the subrange_type tag. */
	if(res == DW_DLV_OK)
	{
		Dwarf_Half childTag;

		res = dwarf_tag(dieSubrangeType, &childTag, &error);
		if(res != DW_DLV_OK)
		{
			logger.logError("Error in dwarf_tag.  %u  errno=%u %s", __LINE__, dwarf_errno(error),
				dwarf_errmsg(error));
		}
		else
		{
			if(childTag != DW_TAG_subrange_type)
			{
				logger.logError("Unexpected child in array.  tag=%u", childTag);

				res = DW_DLV_ERROR;
			}
		}
	}

	/* Get the upper bound. */
	if(res == DW_DLV_OK)
	{
		res = dwarf_attr(dieSubrangeType, DW_AT_upper_bound, &attr_struct, &error);
		if(res != DW_DLV_OK)
		{
			logger.logError("Error in dwarf_attr(DW_AT_upper_bound).  %u  errno=%u %s", __LINE__, dwarf_errno(error),
				dwarf_errmsg(error));
		}

		if(res == DW_DLV_OK)
		{
			res = dwarf_formudata(attr_struct, &dwfUpperBound, &error);
			if(res != DW_DLV_OK)
			{
				logger.logError("Error in dwarf_formudata.  errno=%u %s", dwarf_errno(error),
						dwarf_errmsg(error));
			}
		}

		/* Set the multiplicity, the array's size. */
		if(res == DW_DLV_OK)
		{
			multiplicity = dwfUpperBound + 1;
			logger.logInfo("size of array:%d", multiplicity);
		}
	}

    res = dwarf_siblingof(dbg, inDie, &sib_die, &error);
     if(res == DW_DLV_ERROR)
     {
         logger.logError("Error in dwarf_siblingof , function process_DW_TAG_array_type.  errno=%u %s" , dwarf_errno(error),
                 dwarf_errmsg(error));;
     }
     else
     {
		res = dwarf_diename(sib_die, &arrayName, &error);
		if(DW_DLV_ERROR == res || DW_DLV_NO_ENTRY == res )
		{
			logger.logError("Error in dwarf_diename , function process_DW_TAG_array_type.  errno=%u %s", dwarf_errno(error),
						dwarf_errmsg(error));
		}
		else
		{
			/**
			 *@todo Logic needs to be cleaned up.
			 *I think we need to handle the case when arraySymbol is nullptr.
			 */
			std::string stdString{arrayName};

			logger.logInfo("Name for array-->%s", arrayName);

			Symbol* 	arraySymbol =  getBaseTypeSymbol(elf, inDie, multiplicity);

			if(nullptr == arraySymbol)
			{
				res = DW_DLV_ERROR;

				logger.logError("Base type not found for %s", arrayName);
			}
			else
			{
				std::string arrayBaseType{arraySymbol->getName().c_str()};
				outSymbol = elf.getSymbol(arrayBaseType);
				outSymbol->addField(stdString, 0, *outSymbol, multiplicity, elf.isLittleEndian());
			}

			logger.logInfo("Name for array-->%s", arrayName);
		}
     }

     return res;
}

char * Juicer::getFirstAncestorName(Dwarf_Die inDie)
{
    Dwarf_Attribute attr_struct;
    Dwarf_Off       typeOffset = 0;
    Dwarf_Die       typeDie;
    char            *outName = nullptr;
    Dwarf_Bool      hasName = false;

    /* Get the type attribute. */
    res = dwarf_attr(inDie, DW_AT_type, &attr_struct, &error);

    /* Get the offset to the type Die. */
    if(res == DW_DLV_OK)
    {
        res = dwarf_global_formref(attr_struct, &typeOffset, &error);
        if(res != DW_DLV_OK)
        {
            logger.logError("Error in dwarf_formref.  errno=%u %s", dwarf_errno(error),
                    dwarf_errmsg(error));
        }
    }

    /* Get the type Die. */
    if(res == DW_DLV_OK)
    {
        res = dwarf_offdie(dbg, typeOffset, &typeDie, &error);
        if(res != DW_DLV_OK)
        {
            logger.logError("Error in dwarf_offdie.  errno=%u %s", dwarf_errno(error),
                dwarf_errmsg(error));
        }
    }

    /* Does this die have a name? */
    if(res == DW_DLV_OK)
    {
        res = dwarf_hasattr(typeDie, DW_AT_name, &hasName, &error);
        if(res != DW_DLV_OK)
        {
            logger.logError("Error in dwarf_hasattr(DW_AT_name).  %u  errno=%u %s", __LINE__, dwarf_errno(error),
                    dwarf_errmsg(error));
        }
    }

    if(res == DW_DLV_OK)
    {
        if(hasName == false)
        {
            outName = getFirstAncestorName(typeDie);
        }
        else
        {
            /* Get the name of the type Die. */
            res = dwarf_attr(typeDie, DW_AT_name, &attr_struct, &error);
            if(res != DW_DLV_OK)
            {
                logger.logError("Error in dwarf_attr(DW_AT_name).  %u  errno=%u %s", __LINE__, dwarf_errno(error),
                    dwarf_errmsg(error));
            }

            if(res == DW_DLV_OK)
            {
                res = dwarf_formstring(attr_struct, &outName, &error);
                if(res != DW_DLV_OK)
                {
                    logger.logError("Error in dwarf_formstring.  errno=%u %s", dwarf_errno(error),
                            dwarf_errmsg(error));
                }
            }
        }
    }

    return outName;
}



Symbol * Juicer::process_DW_TAG_pointer_type(ElfFile& elf, Dwarf_Debug dbg, Dwarf_Die inDie)
{
    Symbol          *outSymbol = 0;
    Dwarf_Attribute attr_struct;
    Dwarf_Off       typeOffset = 0;
    Dwarf_Die       typeDie;
    char            *typeDieName;

    /* Get the type attribute. */
    res = dwarf_attr(inDie, DW_AT_type, &attr_struct, &error);
    if(res != DW_DLV_OK)
    {
        logger.logDebug("Ignoring error in dwarf_attr(DW_AT_type). %u  errno=%u %s", __LINE__, dwarf_errno(error),
            dwarf_errmsg(error));
    }

    /* Get the offset to the type Die. */
    if(res == DW_DLV_OK)
    {
        res = dwarf_global_formref(attr_struct, &typeOffset, &error);
        if(res != DW_DLV_OK)
        {
            logger.logError("Error in dwarf_formref.  errno=%u %s", dwarf_errno(error),
                    dwarf_errmsg(error));
        }
    }

    /* Get the type Die. */
    if(res == DW_DLV_OK)
    {
        res = dwarf_offdie(dbg, typeOffset, &typeDie, &error);
        if(res != DW_DLV_OK)
        {
            logger.logError("Error in dwarf_offdie.  errno=%u %s", dwarf_errno(error),
                dwarf_errmsg(error));
        }
    }

    /* Get the name of the type Die. */
    typeDieName = getFirstAncestorName(inDie);

    if(res == DW_DLV_OK)
    {
        Dwarf_Unsigned byteSize;
        std::string name = typeDieName;
        name = name + "*";

        res = dwarf_bytesize(inDie, &byteSize, &error);
        if(res != DW_DLV_OK)
        {
            logger.logError("Error in dwarf_bytesize.  %u  errno=%u %s", __LINE__, dwarf_errno(error),
                dwarf_errmsg(error));
        }

        if(res == DW_DLV_OK)
        {
            outSymbol = elf.addSymbol(name, byteSize);
        }
    }

    return outSymbol;
}



Symbol * Juicer::getBaseTypeSymbol(ElfFile &elf, Dwarf_Die inDie, uint32_t &multiplicity)
{
    int             res = DW_DLV_OK;
    Dwarf_Attribute attr_struct;
    Dwarf_Die       typeDie = 0;
    Dwarf_Off       typeOffset = 0;
    Symbol *        outSymbol = 0;
    char            *dieName = 0;
    Dwarf_Half      tag;
    std::string     cName;

    /* Get the type attribute. */
    res = dwarf_attr(inDie, DW_AT_type, &attr_struct, &error);

    DisplayDie(inDie);

    if(res != DW_DLV_OK)
    {
        logger.logWarning("Cannot find data type.  Skipping.  %u  errno=%u %s ", __LINE__, dwarf_errno(error),
            dwarf_errmsg(error));
    }

    /* Get the offset to the type Die. */
    if(res == DW_DLV_OK)
    {
        res = dwarf_global_formref(attr_struct, &typeOffset, &error);
        if(res != DW_DLV_OK)
        {
            logger.logError("Error in dwarf_formref.  errno=%u %s", dwarf_errno(error),
                    dwarf_errmsg(error));
        }
    }

    /* Get the type Die. */
    if(res == DW_DLV_OK)
    {
        res = dwarf_offdie(dbg, typeOffset, &typeDie, &error);
        if(res != DW_DLV_OK)
        {
            logger.logError("Error in dwarf_offdie.  errno=%u %s", dwarf_errno(error),
                dwarf_errmsg(error));
        }
    }

    /* Get the tag so we know how to process it. */

    if(res == DW_DLV_OK)
    {
    	DisplayDie(typeDie);

        res = dwarf_tag(typeDie, &tag, &error);
        if(res != DW_DLV_OK)
        {
            logger.logError("Error in dwarf_tag.  %u  errno=%u %s", __LINE__, dwarf_errno(error),
                dwarf_errmsg(error));
        }
    }

    if(res == DW_DLV_OK)
    {
    	logger.logDebug("res == DW_DLV_OK");
        switch(tag)
        {

            case DW_TAG_pointer_type:
            {
                outSymbol = process_DW_TAG_pointer_type(elf, dbg, typeDie);
                logger.logDebug("DW_TAG_pointer_type");
                break;
            }

            case DW_TAG_structure_type:
            {
                Dwarf_Bool     structHasName = false;
                Dwarf_Bool     parentHasName = false;
                Dwarf_Unsigned byteSize = 0;

                logger.logDebug("DW_TAG_structure_type");

                /* Does the structure type itself have the name? */
                res = dwarf_hasattr(typeDie, DW_AT_name, &structHasName, &error);
                if(res != DW_DLV_OK)
                {
                    logger.logError("Error in dwarf_hasattr(DW_AT_name).  errno=%u %s", dwarf_errno(error),
                        dwarf_errmsg(error));
                }

                res = dwarf_hasattr(inDie, DW_AT_name, &parentHasName, &error);
                if(res != DW_DLV_OK)
                {
                    logger.logError("Error in dwarf_hasattr(DW_AT_name).  errno=%u %s", dwarf_errno(error),
                        dwarf_errmsg(error));
                }

                /* Read the name from the Die that has it. */
                if(structHasName)
                {
                    res = dwarf_attr(typeDie, DW_AT_name, &attr_struct, &error);
                    if(res != DW_DLV_OK)
                    {
                        logger.logError("Error in dwarf_attr(DW_AT_name).  %u  errno=%u %s", __LINE__, dwarf_errno(error),
                            dwarf_errmsg(error));
                    }

                    if(res == DW_DLV_OK)
                    {
                        res = dwarf_formstring(attr_struct, &dieName, &error);
                        if(res != DW_DLV_OK)
                        {
                            logger.logError("Error in dwarf_formstring.  errno=%u %s", dwarf_errno(error),
                                    dwarf_errmsg(error));
                        }
                    }
                }
                else if(parentHasName)
                {
                    res = dwarf_attr(inDie, DW_AT_name, &attr_struct, &error);
                    if(res != DW_DLV_OK)
                    {
                        logger.logError("Error in dwarf_attr(DW_AT_name).  %u  errno=%u %s", __LINE__, dwarf_errno(error),
                            dwarf_errmsg(error));
                    }

                    if(res == DW_DLV_OK)
                    {
                        res = dwarf_formstring(attr_struct, &dieName, &error);
                        if(res != DW_DLV_OK)
                        {
                            logger.logError("Error in dwarf_formstring.  errno=%u %s", dwarf_errno(error),
                                    dwarf_errmsg(error));
                        }
                    }
                }
                else
                {
                    /* Couldn't find the name at all. */
                    res = DW_DLV_ERROR;
                }

                if(res == DW_DLV_OK)
                {
                    res = dwarf_bytesize(typeDie, &byteSize, &error);
                    if(res != DW_DLV_OK)
                    {
                        logger.logWarning("Skipping '%s'.  Error in dwarf_bytesize.  %u  errno=%u %s", dieName, __LINE__, dwarf_errno(error),
                            dwarf_errmsg(error));
                    }
                }

                if(res == DW_DLV_OK)
                {
                    std::string cName = dieName;
                    outSymbol = elf.addSymbol(cName, byteSize);

                    if(nullptr != outSymbol)
                    {
                    	process_DW_TAG_structure_type(elf, *outSymbol, dbg, typeDie);
                    }
                }
                break;
            }

            case DW_TAG_base_type:
            {
                outSymbol = process_DW_TAG_base_type(elf, dbg, typeDie);
                logger.logDebug("DW_TAG_base_type");
                break;
            }

            case DW_TAG_typedef:
            {
                outSymbol = process_DW_TAG_typedef(elf, dbg, typeDie);
                logger.logDebug("DW_TAG_typedef");
                break;
            }

            case DW_TAG_enumeration_type:
            {
                Dwarf_Bool     structHasName = false;
                Dwarf_Bool     parentHasName = false;
                Dwarf_Unsigned byteSize = 0;

                logger.logDebug("DW_TAG_enumeration_type");

                /* Does the structure type itself have the name? */
                res = dwarf_hasattr(typeDie, DW_AT_name, &structHasName, &error);
                if(res != DW_DLV_OK)
                {
                    logger.logError("Error in dwarf_hasattr(DW_AT_name).  errno=%u %s", dwarf_errno(error),
                        dwarf_errmsg(error));
                }

                res = dwarf_hasattr(inDie, DW_AT_name, &parentHasName, &error);
                if(res != DW_DLV_OK)
                {
                    logger.logError("Error in dwarf_hasattr(DW_AT_name).  errno=%u %s", dwarf_errno(error),
                        dwarf_errmsg(error));
                }

                /* Read the name from the Die that has it. */
                if(structHasName)
                {
                    res = dwarf_attr(typeDie, DW_AT_name, &attr_struct, &error);
                    if(res != DW_DLV_OK)
                    {
                        logger.logError("Error in dwarf_attr(DW_AT_name).  %u  errno=%u %s", __LINE__, dwarf_errno(error),
                            dwarf_errmsg(error));
                    }

                    if(res == DW_DLV_OK)
                    {
                        res = dwarf_formstring(attr_struct, &dieName, &error);
                        if(res != DW_DLV_OK)
                        {
                            logger.logError("Error in dwarf_formstring.  errno=%u %s", dwarf_errno(error),
                                    dwarf_errmsg(error));
                        }
                    }
                }
                else if(parentHasName)
                {
                    res = dwarf_attr(inDie, DW_AT_name, &attr_struct, &error);
                    if(res != DW_DLV_OK)
                    {
                        logger.logError("Error in dwarf_attr(DW_AT_name).  %u  errno=%u %s", __LINE__, dwarf_errno(error),
                            dwarf_errmsg(error));
                    }

                    if(res == DW_DLV_OK)
                    {
                        res = dwarf_formstring(attr_struct, &dieName, &error);
                        if(res != DW_DLV_OK)
                        {
                            logger.logError("Error in dwarf_formstring.  errno=%u %s", dwarf_errno(error),
                                    dwarf_errmsg(error));
                        }
                    }
                }
                else
                {
                    /* Couldn't find the name at all. */
                    res = DW_DLV_ERROR;
                }

                if(res == DW_DLV_OK)
                {
                    res = dwarf_bytesize(typeDie, &byteSize, &error);
                    if(res != DW_DLV_OK)
                    {
                        logger.logError("Error in dwarf_bytesize.  %u  errno=%u %s", __LINE__, dwarf_errno(error),
                            dwarf_errmsg(error));
                    }
                }

                if(res == DW_DLV_OK)
                {
                    std::string cName = dieName;
                    outSymbol = elf.addSymbol(cName, byteSize);
                    process_DW_TAG_enumeration_type(elf, *outSymbol, dbg, typeDie);
                }
                break;
            }

            case DW_TAG_array_type:
            {
                Dwarf_Die dieSubrangeType;
                Dwarf_Unsigned dwfUpperBound = 0;

                logger.logDebug("DW_TAG_array_type");

                /* First get the base type itself. */
                outSymbol = getBaseTypeSymbol(elf, typeDie, multiplicity);

                /* Now lets get the array size.  Get the array size by getting
                 * the first child, which should be the subrange_type. */
                if(res == DW_DLV_OK)
                {
                    res = dwarf_child(typeDie, &dieSubrangeType, &error);
                    if(res == DW_DLV_ERROR)
                    {
                        logger.logError("Error in dwarf_child. errno=%u %s", dwarf_errno(error),
                                dwarf_errmsg(error));
                    }
                }

                /* Make sure this is the subrange_type tag. */
                if(res == DW_DLV_OK)
                {
                    Dwarf_Half childTag;

                    res = dwarf_tag(dieSubrangeType, &childTag, &error);
                    if(res != DW_DLV_OK)
                    {
                        logger.logError("Error in dwarf_tag.  %u  errno=%u %s", __LINE__, dwarf_errno(error),
                            dwarf_errmsg(error));
                    }
                    else
                    {
                        if(childTag != DW_TAG_subrange_type)
                        {
                            logger.logError("Unexpected child in array.  tag=%u", tag);

                            res = DW_DLV_ERROR;
                        }
                    }
                }

                /* Get the upper bound. */
                if(res == DW_DLV_OK)
                {
                    res = dwarf_attr(dieSubrangeType, DW_AT_upper_bound, &attr_struct, &error);
                    if(res != DW_DLV_OK)
                    {
                        logger.logError("Error in dwarf_attr(DW_AT_upper_bound).  %u  errno=%u %s", __LINE__, dwarf_errno(error),
                            dwarf_errmsg(error));
                    }

                    if(res == DW_DLV_OK)
                    {
                        res = dwarf_formudata(attr_struct, &dwfUpperBound, &error);
                        if(res != DW_DLV_OK)
                        {
                            logger.logError("Error in dwarf_formudata.  errno=%u %s", dwarf_errno(error),
                                    dwarf_errmsg(error));
                        }
                    }

                    /* Set the multiplicity argument. */
                    if(res == DW_DLV_OK)
                    {
                        multiplicity = dwfUpperBound + 1;
                    }
                }

                break;
            }

            case DW_TAG_class_type:
            {
                /* TODO */
            	logger.logDebug("DW_TAG_class_type");
                break;
            }

            case DW_TAG_const_type:
            {
                /* TODO */
                /* Get the type attribute. */
                res = dwarf_attr(inDie, DW_AT_type, &attr_struct, &error);

                getBaseTypeSymbol(elf, typeDie, multiplicity);

                break;
            }

            case DW_TAG_reference_type:
            {
                /* TODO */
            	logger.logDebug("DW_TAG_reference_type");
                break;
            }

            case DW_TAG_union_type:
            {
                /* TODO */
            	logger.logDebug("DW_TAG_union_type");
                break;
            }


            /* Fallthru */
            case DW_TAG_unspecified_type:
            {
            	logger.logDebug("DW_TAG_unspecified_type");
            	break;
            }
            case DW_TAG_rvalue_reference_type:
            {
                /* Ignore these tags. */
            	logger.logDebug("DW_TAG_rvalue_reference_type");
                break;
            }

            default:
            {
                //outSymbol = getBaseTypeSymbol(elf, typeDie);//
                logger.logWarning("Unsupported Tag found. 0x%02x", tag);
                break;
            }
        }
    }

    if(nullptr == outSymbol)
    {
    	logger.logDebug("outSymbol is null!");
    }

    return outSymbol;
}


const char SPARE_FIELD_NAME[]="";


void Juicer::DisplayDie(Dwarf_Die inDie)
{
    int             res = DW_DLV_OK;
    Dwarf_Attribute *attribs;
    Dwarf_Signed    attribCount = 20;
    Dwarf_Off       globalOffset;
    Dwarf_Off       localOffset;
    Dwarf_Attribute attr_struct;
    char            *dieName;
    Dwarf_Half      tag = 0;
    int             abbrevCode = 0;
    Dwarf_Half      hasChildrenFlag = 0;

    if(inDie != 0)
    {
        res = dwarf_tag(inDie, &tag, &error);
        if(res == DW_DLV_OK)
        {
            switch(tag)
            {
                case DW_TAG_typedef:
                    printf("DW_TAG_typedef");
                    break;

                case DW_TAG_structure_type:
                    printf("DW_TAG_structure_type");
                    break;

                default:
                    printf("0x%02x", tag);
                    break;
            }
        }
        else
        {
            printf("   ");
        }

        res = dwarf_attr(inDie, DW_AT_name, &attr_struct, &error);
        if(res == DW_DLV_OK)
        {
            res = dwarf_formstring(attr_struct, &dieName, &error);
            if(res != DW_DLV_OK)
            {
                logger.logError("Error in dwarf_formstring.  errno=%u %s", dwarf_errno(error),
                        dwarf_errmsg(error));
            }
            else
            {
                printf(":%s\n", dieName);
            }
        }
        else
        {
            printf("\n");
        }

        res = dwarf_die_offsets(inDie, &globalOffset, &localOffset, &error);
        if(res != DW_DLV_OK)
        {
            logger.logError("Error in dwarf_die_offsets.  errno=%u %s", dwarf_errno(error),
                    dwarf_errmsg(error));
        }
        else
        {
            printf("  DIE offset (local) : 0x%08llx\n", localOffset);
            printf("  DIE offset (global): 0x%08llx\n", globalOffset);
        }

        abbrevCode = dwarf_die_abbrev_code(inDie);
        printf("  Abbrev code:         %i\n", abbrevCode);

        res = dwarf_die_abbrev_children_flag(inDie, &hasChildrenFlag);
        if(res != DW_DLV_OK)
        {
            logger.logError("Error in dwarf_die_abbrev_children_flag.  errno=%u %s", dwarf_errno(error),
                    dwarf_errmsg(error));
        }
        else
        {
            printf("  Has children:        %s\n", hasChildrenFlag ? "True" : "False");
        }

        int dwarf_die_abbrev_children_flag(Dwarf_Die /*die*/,
            Dwarf_Half * /*ab_has_child*/);

        res = dwarf_attrlist(inDie, &attribs, &attribCount, &error);
        if(res != DW_DLV_OK)
        {
            logger.logError("Error in dwarf_attrlist.  errno=%u %s", dwarf_errno(error),
                    dwarf_errmsg(error));
        }
        else
        {
            if(attribCount > 0)
            {
                printf("  Attributes:\n");
                for(uint32_t i = 0; i < attribCount; ++i)
                {
                    Dwarf_Half attrNum;
                    res = dwarf_whatattr(attribs[i], &attrNum, &error);
                    if(res != DW_DLV_OK)
                    {
                        logger.logError("Error in dwarf_whatattr.  errno=%u %s", dwarf_errno(error),
                                dwarf_errmsg(error));
                    }
                    else
                    {
                        Dwarf_Half formID;

                        switch(attrNum)
                        {
                            case DW_AT_sibling:
                                printf("    DW_AT_sibling");
                                break;

                            case DW_AT_location:
                                printf("    DW_AT_location");
                                break;

                            case DW_AT_name:
                                printf("    DW_AT_name");
                                break;

                            case DW_AT_ordering:
                                printf("    DW_AT_ordering");
                                break;

                            case DW_AT_subscr_data:
                                printf("    DW_AT_subscr_data");
                                break;

                            case DW_AT_byte_size:
                                printf("    DW_AT_byte_size");
                                break;

                            case DW_AT_decl_file:
                                printf("    DW_AT_decl_file");
                                break;

                            case DW_AT_decl_line:
                                printf("    DW_AT_decl_line");
                                break;

                            case DW_AT_type:
                                printf("    DW_AT_type");
                                break;

                            default:
                                printf("    0x%02x", attrNum);
                                break;
                        }

                        res = dwarf_whatform(attribs[i], &attrNum, &error);
                        if(res != DW_DLV_OK)
                        {
                            logger.logError("Error in dwarf_whatattr.  errno=%u %s", dwarf_errno(error),
                                    dwarf_errmsg(error));
                        }
                        else
                        {
                            res = dwarf_whatform(attribs[i], &formID, &error);
                            if(res != DW_DLV_OK)
                            {
                                logger.logError("Error in dwarf_whatform.  errno=%u %s", dwarf_errno(error),
                                        dwarf_errmsg(error));
                            }
                            else
                            {
                                switch(formID)
                                {
                                    case DW_FORM_addr:
                                        printf(":DW_FORM_addr\n");
                                        break;

                                    case DW_FORM_block2:
                                        printf(":DW_FORM_block2\n");
                                        break;

                                    case DW_FORM_block4:
                                        printf(":DW_FORM_block4\n");
                                        break;

                                    case DW_FORM_data1:
                                    {
                                        Dwarf_Unsigned udata = 0;
                                        res = dwarf_formudata(attribs[i], &udata, &error);
                                        if(res != DW_DLV_OK)
                                        {
                                            logger.logError("Error in dwarf_formudata.  errno=%u %s", dwarf_errno(error),
                                                    dwarf_errmsg(error));
                                        }
                                        else
                                        {
                                            char data = (char) udata;
                                            printf(":DW_FORM_data1:%u\n", data);
                                        }
                                        break;
                                    }

                                    case DW_FORM_data2:
                                    {
                                        Dwarf_Unsigned udata = 0;
                                        res = dwarf_formudata(attribs[i], &udata, &error);
                                        if(res != DW_DLV_OK)
                                        {
                                            logger.logError("Error in dwarf_formudata.  errno=%u %s", dwarf_errno(error),
                                                    dwarf_errmsg(error));
                                        }
                                        else
                                        {
                                            unsigned short data = (unsigned short) udata;
                                            printf(":DW_FORM_data2:%u\n", data);
                                        }
                                        break;
                                    }

                                    case DW_FORM_data4:
                                    {
                                        Dwarf_Unsigned udata = 0;
                                        res = dwarf_formudata(attribs[i], &udata, &error);
                                        if(res != DW_DLV_OK)
                                        {
                                            logger.logError("Error in dwarf_formudata.  errno=%u %s", dwarf_errno(error),
                                                    dwarf_errmsg(error));
                                        }
                                        else
                                        {
                                            unsigned int data = (unsigned int) udata;
                                            printf(":DW_FORM_data4:%u\n", data);
                                        }
                                        break;
                                    }

                                    case DW_FORM_data8:
                                    {
                                        Dwarf_Unsigned udata = 0;
                                        res = dwarf_formudata(attribs[i], &udata, &error);
                                        if(res != DW_DLV_OK)
                                        {
                                            logger.logError("Error in dwarf_formudata.  errno=%u %s", dwarf_errno(error),
                                                    dwarf_errmsg(error));
                                        }
                                        else
                                        {
                                            printf(":DW_FORM_data8:%llu\n", udata);
                                        }
                                        break;
                                    }

                                    case DW_FORM_string:
                                    {
                                        char *str = 0;
                                        res = dwarf_formstring(attribs[i], &str, &error);
                                        if(res != DW_DLV_OK)
                                        {
                                            logger.logError("Error in dwarf_formstring.  errno=%u %s", dwarf_errno(error),
                                                    dwarf_errmsg(error));
                                        }
                                        else
                                        {
                                            printf(":DW_FORM_string:%s\n", str);
                                        }
                                        break;
                                    }

                                    case DW_FORM_block:
                                        printf(":DW_FORM_block\n");
                                        break;

                                    case DW_FORM_sdata:
                                        printf(":DW_FORM_sdata\n");
                                        break;

                                    case DW_FORM_strp:
                                    {
                                        char *strp = 0;
                                        res = dwarf_formstring(attribs[i], &strp, &error);
                                        if(res != DW_DLV_OK)
                                        {
                                            logger.logError("Error in dwarf_formstring.  errno=%u %s", dwarf_errno(error),
                                                    dwarf_errmsg(error));
                                        }
                                        else
                                        {
                                            char *text = dwarfStringToChar(strp);
                                            printf(":DW_FORM_strp:%s\n", text);
                                        }
                                        break;
                                    }

                                    case DW_FORM_udata:
                                    {
                                        Dwarf_Unsigned udata = 0;
                                        res = dwarf_formudata(attribs[i], &udata, &error);
                                        if(res != DW_DLV_OK)
                                        {
                                            logger.logError("Error in dwarf_formudata.  errno=%u %s", dwarf_errno(error),
                                                    dwarf_errmsg(error));
                                        }
                                        else
                                        {
                                            printf(":DW_FORM_udata:%llu\n", udata);
                                        }
                                        break;
                                    }

                                    case DW_FORM_ref_addr:
                                        printf(":DW_FORM_ref_addr\n");
                                        break;

                                    case DW_FORM_ref1:
                                    {
                                        Dwarf_Off ref = 0;
                                        res = dwarf_formref(attribs[i], &ref, &error);
                                        if(res != DW_DLV_OK)
                                        {
                                            logger.logError("Error in dwarf_formref.  errno=%u %s", dwarf_errno(error),
                                                    dwarf_errmsg(error));
                                        }
                                        else
                                        {
                                            char data = (char) ref;
                                            printf(":DW_FORM_ref1:%u\n", data);
                                        }
                                        break;
                                    }

                                    case DW_FORM_ref2:
                                    {
                                        Dwarf_Off ref = 0;
                                        res = dwarf_formref(attribs[i], &ref, &error);
                                        if(res != DW_DLV_OK)
                                        {
                                            logger.logError("Error in dwarf_formref.  errno=%u %s", dwarf_errno(error),
                                                    dwarf_errmsg(error));
                                        }
                                        else
                                        {
                                            unsigned short int data = (unsigned short int) ref;
                                            printf(":DW_FORM_ref2:%u\n", data);
                                        }
                                        break;
                                    }

                                    case DW_FORM_ref4:
                                    {
                                        Dwarf_Off ref = 0;
                                        res = dwarf_formref(attribs[i], &ref, &error);
                                        if(res != DW_DLV_OK)
                                        {
                                            logger.logError("Error in dwarf_formref.  errno=%u %s", dwarf_errno(error),
                                                    dwarf_errmsg(error));
                                        }
                                        else
                                        {
                                            unsigned int data = (unsigned int) ref;
                                            printf(":DW_FORM_ref4:%u\n", data);
                                        }
                                        break;
                                    }

                                    case DW_FORM_ref8:
                                    {
                                        Dwarf_Off ref = 0;
                                        res = dwarf_formref(attribs[i], &ref, &error);
                                        if(res != DW_DLV_OK)
                                        {
                                            logger.logError("Error in dwarf_formref.  errno=%u %s", dwarf_errno(error),
                                                    dwarf_errmsg(error));
                                        }
                                        else
                                        {
                                            printf(":DW_FORM_ref4:%llu\n", ref);
                                        }
                                        break;
                                    }

                                    case DW_FORM_ref_udata:
                                        printf(":DW_FORM_ref_udata\n");
                                        break;

                                    case DW_FORM_indirect:
                                        printf(":DW_FORM_indirect\n");
                                        break;

                                    case DW_FORM_sec_offset:
                                        printf(":DW_FORM_sec_offset\n");
                                        break;

                                    case DW_FORM_exprloc:
                                        printf(":DW_FORM_exprloc\n");
                                        break;

                                    case DW_FORM_flag_present:
                                        printf(":DW_FORM_flag_present\n");
                                        break;

                                    case DW_FORM_ref_sig8:
                                        printf(":DW_FORM_ref_sig8\n");
                                        break;

                                    default:
                                        printf(":0x%02x\n", formID);
                                        break;

                                }
                            }
                        }
                    }
                }
                printf("\n");
            }
        }
    }
}



Symbol * Juicer::process_DW_TAG_base_type(ElfFile& elf, Dwarf_Debug dbg, Dwarf_Die inDie)
{
    int             res = DW_DLV_OK;
    Dwarf_Unsigned  byteSize = 0;
    char            *dieName = 0;
    Dwarf_Attribute attr_struct;
    Symbol          *outSymbol;
    std::string     cName;

    /* Get the name attribute of this Die. */
    res = dwarf_attr(inDie, DW_AT_name, &attr_struct, &error);
    if(res != DW_DLV_OK)
    {
        logger.logError("Error in dwarf_attr(DW_AT_name).  %u  errno=%u %s", __LINE__, dwarf_errno(error),
                dwarf_errmsg(error));
    }

    /* Get the actual name of this Die. */
    if(res == DW_DLV_OK)
    {
        res = dwarf_formstring(attr_struct, &dieName, &error);
        if(res != DW_DLV_OK)
        {
            logger.logError("Error in dwarf_formstring.  errno=%u %s", dwarf_errno(error),
                    dwarf_errmsg(error));
        }
    }

    /* See if we already have this symbol. */
    cName = dieName;
    outSymbol = elf.getSymbol(cName);
    if(outSymbol == 0)
    {
        /* No.  This is new.  Process it. */

        /* Get the size of this datatype. */
        if(res == DW_DLV_OK)
        {
            res = dwarf_bytesize(inDie, &byteSize, &error);
            if(res != DW_DLV_OK)
            {
                logger.logError("Error in dwarf_bytesize.  %u  errno=%u %s", __LINE__, dwarf_errno(error),
                    dwarf_errmsg(error));
            }
        }

        /* We have everything we need.  Add this to the elf. */
        if(res == DW_DLV_OK)
        {
            std::string sDieName = dieName;
            outSymbol = elf.addSymbol(sDieName, byteSize);
        }
    }

    return outSymbol;
}


void Juicer::process_DW_TAG_enumeration_type(ElfFile& elf, Symbol &symbol, Dwarf_Debug dbg, Dwarf_Die inDie)
{
    int             res = DW_DLV_OK;
    Dwarf_Attribute attr_struct = 0;
    Dwarf_Die       enumeratorDie = 0;

    /* Get the fields by getting the first child. */
    if(res == DW_DLV_OK)
    {
        res = dwarf_child(inDie, &enumeratorDie, &error);
        if(res == DW_DLV_ERROR)
        {
            logger.logError("Error in dwarf_child. errno=%u %s", dwarf_errno(error),
                    dwarf_errmsg(error));
        }
    }

    /* Start processing the fields. */
    for(;;)
    {
        char           *enumeratorName = 0;
        Dwarf_Unsigned enumeratorValue = 0;

        /* Make sure this is a member tag. */
        if(res == DW_DLV_OK)
        {
            Dwarf_Half tag;

            res = dwarf_tag(enumeratorDie, &tag, &error);
            if(res == DW_DLV_ERROR)
            {
                logger.logError("Error in dwarf_tag. errno=%u %s", dwarf_errno(error),
                        dwarf_errmsg(error));
            }
            else
            {
                if(tag != DW_TAG_enumerator)
                {
                    logger.logError("Die is not an enumerator.");
                    res = DW_DLV_ERROR;
                }
            }
        }

        /* Get the name attribute of this Die. */
        if(res == DW_DLV_OK)
        {
            res = dwarf_attr(enumeratorDie, DW_AT_name, &attr_struct, &error);
            if(res != DW_DLV_OK)
            {
                logger.logError("Error in dwarf_attr(DW_AT_name).  %u  errno=%u %s", __LINE__, dwarf_errno(error),
                    dwarf_errmsg(error));
            }
        }

        /* Get the actual name of this enumerator. */
        if(res == DW_DLV_OK)
        {
            res = dwarf_formstring(attr_struct, &enumeratorName, &error);
            if(res != DW_DLV_OK)
            {
                logger.logError("Error in dwarf_formstring.  errno=%u %s", dwarf_errno(error),
                        dwarf_errmsg(error));
            }
        }

        /* Get the value attribute of this enumerator. */
        if(res == DW_DLV_OK)
        {
            res = dwarf_attr(enumeratorDie, DW_AT_const_value, &attr_struct, &error);
            if(res != DW_DLV_OK)
            {
                logger.logError("Error in dwarf_attr(DW_AT_data_member_location).  %u  errno=%u %s", __LINE__, dwarf_errno(error),
                    dwarf_errmsg(error));
            }
        }

        /* Get the actual value of this enumerator. */
        if(res == DW_DLV_OK)
        {
            res = dwarf_formudata(attr_struct, &enumeratorValue, &error);
            if(res != DW_DLV_OK)
            {
                logger.logError("Error in dwarf_formudata.  errno=%u %s", dwarf_errno(error),
                    dwarf_errmsg(error));
            }
        }

        /* We have everything we need.  Add this enumerator. */
        if(res == DW_DLV_OK)
        {
            std::string sEnumeratorName = enumeratorName;
            Dwarf_Die siblingDie = 0;

            symbol.addEnumeration(sEnumeratorName, enumeratorValue);

            res = dwarf_siblingof(dbg, enumeratorDie, &siblingDie, &error);
            if(res == DW_DLV_ERROR)
            {
                logger.logError("Error in dwarf_siblingof.  errno=%u %s", dwarf_errno(error),
                        dwarf_errmsg(error));
            }
            else if(res == DW_DLV_NO_ENTRY)
            {
                /* We wrapped around.  We're done processing the member fields. */
                break;
            }

            enumeratorDie = siblingDie;
        }

        /* Don't continue looping if there was a problem. */
        if(res != DW_DLV_OK)
        {
            break;
        }
    }
}


/**
 * @brief Inspects the data on the die and its own children recursively.
 * @param in_die the die entry that has the dwarf data.
 * @param in_level The current level on the dbg structure.
 * @return 0 if the die, its children and siblings are scanned successfully.
 * 1 if there is a problem with dies or any of its children.
 */
Symbol * Juicer::process_DW_TAG_typedef(ElfFile& elf, Dwarf_Debug dbg, Dwarf_Die inDie)
{
    int             res = DW_DLV_OK;
    uint32_t        byteSize = 0;
    Symbol          *baseTypeSymbol = 0;
    char            *dieName = 0;
    Dwarf_Attribute attr_struct;
    Symbol          *outSymbol = nullptr;

    /* Get the name attribute of this Die. */
    res = dwarf_attr(inDie, DW_AT_name, &attr_struct, &error);
    if(res != DW_DLV_OK)
    {
        logger.logError("Error in dwarf_attr(DW_AT_name).  %u  errno=%u %s", __LINE__, dwarf_errno(error),
                dwarf_errmsg(error));
    }

    /* Get the actual name of this Die. */
    if(res == DW_DLV_OK)
    {
        res = dwarf_formstring(attr_struct, &dieName, &error);
        if(res != DW_DLV_OK)
        {
            logger.logError("Error in dwarf_formstring.  errno=%u %s", dwarf_errno(error),
                    dwarf_errmsg(error));
        }
    }

    /* Get the base type die. */
    if(res == DW_DLV_OK)
    {
        uint32_t multiplicity;

        baseTypeSymbol = getBaseTypeSymbol(elf ,inDie, multiplicity);
        if(baseTypeSymbol == 0)
        {
            /* Set the error code so we don't do anymore processing. */
            res = DW_DLV_ERROR;
        }
    }

    /* Get the size of this datatype. */
    if(res == DW_DLV_OK)
    {
        byteSize = baseTypeSymbol->getByteSize();
    }

    /* We have everything we need.  Add this to the elf. */
    if(res == DW_DLV_OK)
    {
        std::string sDieName = dieName;
        outSymbol = elf.addSymbol(sDieName, byteSize);
        logger.logDebug("name for this Symbol-->%s\n",outSymbol->getName().c_str());
    }

    return outSymbol;
}

/**
 * @brief Inspects the data on the die and its own children recursively.
 * @param in_die the die entry that has the dwarf data.
 * @param in_level The current level on the dbg structure.
 * @return 0 if the die, its children and siblings are scanned successfully.
 * 1 if there is a problem with dies or any of its children.
 */
void Juicer::process_DW_TAG_structure_type(ElfFile& elf, Symbol& symbol, Dwarf_Debug dbg, Dwarf_Die inDie)
{
    int             res = DW_DLV_OK;
    Dwarf_Attribute attr_struct;
    Dwarf_Die       memberDie = 0;

    /*@note Used for keeping track of fields used for padding. */

    /* Get the fields by getting the first child. */
    if(res == DW_DLV_OK)
    {
        res = dwarf_child(inDie, &memberDie, &error);
        if(res == DW_DLV_ERROR)
        {
            logger.logError("Error in dwarf_child. errno=%u %s", dwarf_errno(error),
                    dwarf_errmsg(error));
        }
    }

    /* Start processing the fields. */
    for(;;)
    {
        char           *memberName = 0;
        Symbol         *memberBaseTypeSymbol = 0;
        Dwarf_Unsigned memberLocation = 0;

        if(res == DW_DLV_OK)
        {
            Dwarf_Half tag;
            Dwarf_Die siblingDie = 0;

            res = dwarf_tag(memberDie, &tag, &error);
            if(res == DW_DLV_ERROR)
            {
                logger.logError("Error in dwarf_tag. errno=%u %s", dwarf_errno(error),
                        dwarf_errmsg(error));
            }
            else
            {
                switch(tag)
                {
                    case DW_TAG_union_type:
                    {
                        logger.logWarning("TODO:  Union members are not yet supported.");
                        break;
                    }

                    case DW_TAG_inheritance:
                    {
                        logger.logWarning("TODO:  Inherited members not yet supported.");
                        break;
                    }

                    case DW_TAG_typedef:
                    {
                        logger.logWarning("TODO:  Typedef member attributes not yet supported.");
                        break;
                    }

                    case DW_TAG_member:
                    {
                        uint32_t multiplicity = 0;

                        /* Get the name attribute of this Die. */
                        if(res == DW_DLV_OK)
                        {
                            res = dwarf_attr(memberDie, DW_AT_name, &attr_struct, &error);
                            if(res != DW_DLV_OK)
                            {
                                logger.logError("Error in dwarf_attr(DW_AT_name).  %u  errno=%u %s", __LINE__, dwarf_errno(error),
                                    dwarf_errmsg(error));
                            }
                        }

                        /* Get the actual name of this member. */
                        if(res == DW_DLV_OK)
                        {
                            res = dwarf_formstring(attr_struct, &memberName, &error);
                            if(res != DW_DLV_OK)
                            {
                                logger.logError("Error in dwarf_formstring.  errno=%u %s", dwarf_errno(error),
                                        dwarf_errmsg(error));
                            }
                        }

                        /* Get the data member location attribute of this member. */
                        if(res == DW_DLV_OK)
                        {
                            res = dwarf_attr(memberDie, DW_AT_data_member_location, &attr_struct, &error);
                            if(res != DW_DLV_OK)
                            {
                                logger.logWarning("Skipping %s.  Error in dwarf_attr(DW_AT_data_member_location).  %u  errno=%u %s", memberName, __LINE__, dwarf_errno(error),
                                    dwarf_errmsg(error));
                            }
                        }

                        /* Get the actual data member location of this member. */
                        if(res == DW_DLV_OK)
                        {
                            res = dwarf_formudata(attr_struct, &memberLocation, &error);
                            if(res != DW_DLV_OK)
                            {
                                logger.logError("Error in dwarf_formudata , level %d.  errno=%u %s", dwarf_errno(error),
                                    dwarf_errmsg(error));
                            }
                        }

                        /* Get the base type die. */
                        if(res == DW_DLV_OK)
                        {
                            memberBaseTypeSymbol = getBaseTypeSymbol(elf, memberDie, multiplicity);
                            if(memberBaseTypeSymbol == 0)
                            {
                                logger.logWarning("Couldn't find base type for %s:%s.", symbol.getName().c_str(), memberName);

                                /* Set the error code so we don't do anymore processing. */
                                res = DW_DLV_ERROR;
                            }
                            logger.logDebug("memberBaseTypeSymbol=%d", memberBaseTypeSymbol);
                        }

                        /* We have everything we need.  Add this field. */
                        if(res == DW_DLV_OK)
                        {

							std::string sMemberName = memberName;

                        	/*Handle any padding that needs to be added. */

							symbol.addField(sMemberName, (uint32_t)memberLocation, *memberBaseTypeSymbol, multiplicity, elf.isLittleEndian());

                        }

                        break;
                    }

                    /* Fall through */
                    case DW_TAG_template_type_parameter:
                    case DW_TAG_template_value_parameter:
                    case DW_TAG_GNU_template_parameter_pack:
                    case DW_TAG_subprogram:
                    {
                        /* Ignore these */
                        break;
                    }

                    default:
                    {
                        logger.logError("Unexpected Die found in struct. 0x%02x", tag);
                        res = DW_DLV_ERROR;
                        break;
                    }
                }
            }

            res = dwarf_siblingof(dbg, memberDie, &siblingDie, &error);
            if(res == DW_DLV_ERROR)
            {
                logger.logError("Error in dwarf_siblingof.  errno=%u %s", dwarf_errno(error),
                        dwarf_errmsg(error));
            }
            else if(res == DW_DLV_NO_ENTRY)
            {
            	addPaddingToStruct(symbol);
                /* We wrapped around.  We're done processing the member fields. */

                break;
            }

            memberDie = siblingDie;
        }

        /* Don't continue looping if there was a problem. */
        if(res != DW_DLV_OK)
        {
            break;
        }
    }
}

/**
 *Scans the symbol and inserts a field called "_spare" if there is padding at the end of structures.
 *@param symbol The symbol to be scanned for padding.
 */
void Juicer::addPaddingToStruct(Symbol& symbol)
{
	uint32_t spareCount{0};

	/*Add padding between fields */
	if (symbol.getFields().size()>0)
	{
		uint32_t fieldsSize= symbol.getFields().size();

		for(uint32_t i= 1;i<fieldsSize;i++)
		{
			/*@note I know the fields container access is ugly this way,
			 * but it is a lot safer than something like std::vector.back() */

			uint32_t previousFieldSize = symbol.getFields().at(i-1)->getType().getByteSize();

			if(symbol.getFields().at(i-1)->getMultiplicity()>0)
			{
				previousFieldSize = symbol.getFields().at(i-1)->getMultiplicity() * previousFieldSize ;
			}

			uint32_t lastFieldOffset = symbol.getFields().at(i-1)->getByteOffset();

			uint32_t memberLocationDelta = symbol.getFields().at(i)->getByteOffset() - lastFieldOffset ;

			uint32_t memberLocation = lastFieldOffset + previousFieldSize;

			if(memberLocationDelta>previousFieldSize)
			{
				uint32_t paddingSize = memberLocationDelta - previousFieldSize;

				std::string spareName{"_spare"};

				spareName += std::to_string(spareCount);

				std::string paddingType{"_padding"};

				paddingType += std::to_string(paddingSize*8);

				Symbol* paddingSymbol = symbol.getElf().getSymbol(paddingType);

				if(paddingSymbol == nullptr)
				{
					paddingSymbol = symbol.getElf().addSymbol(paddingType, paddingSize);
				}

				auto&& fields  = symbol.getFields();

				auto fields_it = fields.begin();

				fields.insert(fields_it+i, std::make_unique<Field>(symbol,spareName, (uint32_t)memberLocation,
						*paddingSymbol, 0, symbol.getElf().isLittleEndian()));

				fieldsSize++;
				i++;
				spareCount++;

				memberLocation += paddingSize;
			}
			memberLocation += memberLocationDelta;

		}

	}

	addPaddingEndToSturct(symbol);
}

void Juicer::addPaddingEndToSturct(Symbol& symbol)
{
	uint32_t correctCurrentSize = 0;
	int tempFieldSize 	   = 0;

	int newFieldByteOffset = 0;

	std::string spareName{"_spare_end"};

	/*@todo Save this string in a macro or something like that. */
	std::string paddingType{"_padding"};

	for(auto&& field: symbol.getFields())
	{
		tempFieldSize = field->getType().getByteSize();

		if(field->getMultiplicity()> 0)
		{
			tempFieldSize = tempFieldSize * field->getMultiplicity();
		}

		correctCurrentSize = correctCurrentSize + tempFieldSize;
	}

	if(correctCurrentSize>0)
	{
		if(correctCurrentSize<symbol.getByteSize())
		{
			int paddingSize =  symbol.getByteSize() - correctCurrentSize;

			paddingType += std::to_string(paddingSize*8);

			Symbol* paddingSymbol = symbol.getElf().getSymbol(paddingType);

			if(paddingSymbol == nullptr)
			{
				paddingSymbol = symbol.getElf().addSymbol(paddingType, paddingSize);
			}

			newFieldByteOffset = symbol.getFields().back()->getByteOffset() + symbol.getFields().back()->getType().getByteSize() ;

			symbol.addField(spareName,newFieldByteOffset, *paddingSymbol, 0, symbol.getElf().isLittleEndian());


		}
	}
}


/**
 * @brief Inspects the data on the die and its own children recursively.
 * @param in_die the die entry that has the dwarf data.
 * @param in_level The current level on the dbg structure.
 * @return 0 if the die, its children and siblings are scanned successfully.
 * 1 if there is a problem with dies or any of its children.
 */
int Juicer::getDieAndSiblings(ElfFile& elf, Dwarf_Debug dbg, Dwarf_Die in_die, int in_level)
{
    int res = DW_DLV_ERROR;
    Dwarf_Die cur_die = in_die;
    Dwarf_Die child = 0;
    Dwarf_Error error = 0;
    char        *dieName;
    Dwarf_Attribute attr_struct;
    int return_value = JUICER_OK;

    printDieData(dbg, in_die, in_level);

    for(;;)
    {
        Dwarf_Die sib_die = 0;
        Dwarf_Half tag = 0;

        res = dwarf_tag(cur_die, &tag, &error);
        if(res != DW_DLV_OK)
        {
            logger.logError("Error in dwarf_tag , level %d.  errno=%u %s", in_level, dwarf_errno(error),
                    dwarf_errmsg(error));
            return_value = JUICER_ERROR;
        }

        switch(tag)
        {
            case DW_TAG_base_type:
            {
                process_DW_TAG_base_type(elf, dbg, cur_die);

                break;
            }

            case DW_TAG_typedef:
            {
                process_DW_TAG_typedef(elf, dbg, cur_die);

                break;
            }

            case DW_TAG_structure_type:
            {
                res = dwarf_attr(cur_die, DW_AT_name, &attr_struct, &error);
                if(res == DW_DLV_OK)
                {
                    res = dwarf_formstring(attr_struct, &dieName, &error);
                    if(res != DW_DLV_OK)
                    {
                        logger.logError("Error in dwarf_formstring.  errno=%u %s", dwarf_errno(error),
                                dwarf_errmsg(error));
                    }
                    else
                    {
                    	Dwarf_Unsigned bytesize;
                    	res = dwarf_bytesize(cur_die, &bytesize, &error);
                    	std::string stdString{dieName};

                        Symbol* outSymbol = elf.addSymbol(stdString,(uint32_t) bytesize);

                        process_DW_TAG_structure_type(elf, *outSymbol, dbg, cur_die);

                    }
                }
                else
                {
                    printf("\n");
                }

            	DisplayDie(cur_die);

            	break;
            }
            case DW_TAG_array_type:
            {
				Symbol s{elf};
            	res = process_DW_TAG_array_type(elf,s, dbg ,cur_die);

                break;
            }

            case DW_TAG_variable:
            {
            	/**
            	 * @todo implement.
            	 */
            }
        }

        res = dwarf_child(cur_die, &child, &error);
        if(res == DW_DLV_ERROR)
        {
            logger.logError("Error in dwarf_child , level %d.  errno=%u %s", in_level, dwarf_errno(error),
                    dwarf_errmsg(error));
            return_value = JUICER_ERROR;
        }
        else if(res == DW_DLV_OK)
        {
        	logger.logInfo("CHILD");
        	getDieAndSiblings(elf, dbg, child, in_level + 1);
        }

        /* res == DW_DLV_NO_ENTRY */
        res = dwarf_siblingof(dbg, cur_die, &sib_die, &error);
        if(res == DW_DLV_ERROR)
        {
            logger.logError("Error in dwarf_siblingof , level %d.  errno=%u %s", in_level, dwarf_errno(error),
                    dwarf_errmsg(error));
            return_value = JUICER_ERROR;
        }

        if(res == DW_DLV_NO_ENTRY)
        {
            /* Done at this level. */
            break;
        }

        /* res == DW_DLV_OK */
        if(cur_die != in_die)
        {
            dwarf_dealloc(dbg, cur_die, DW_DLA_DIE);
        }
        cur_die = sib_die;
        logger.logInfo("SIBBLING");
        printDieData(dbg, cur_die, in_level);
    }
    return return_value;
}

/**
 * @brief prints the data on the DIE dwarf node.
 * @param print_me the DIE dwarf node containing the DWARF data to explore.
 * @param level The current level on the DIE tree of print_me die.
 * @return 0 if the data was printed successfully. -1 if an error occurred.
 */
int Juicer::printDieData(Dwarf_Debug dbg, Dwarf_Die print_me, uint32_t level)
{
    /* Initialize all variables we'll use for printing data off the Die. */
    char *name = 0;
    Dwarf_Error error = 0;
    Dwarf_Half tag = 0;
    Dwarf_Half in_attr = DW_AT_byte_size;
    Dwarf_Attribute attr_struct;
    Dwarf_Unsigned bytesize = 0;
    const char *tagname = 0;
    int localname = 0;
    std::string outputText;

    int return_value = JUICER_OK;

    res = dwarf_diename(print_me, &name, &error);
    if(DW_DLV_ERROR == res)
    {
        logger.logError("Error in dwarf_diename , level %d.  errno=%u %s", level, dwarf_errno(error),
                    dwarf_errmsg(error));
        return_value = JUICER_ERROR;
    }
    else if(DW_DLV_NO_ENTRY == res)
    {
        /* Lorenzo, you can't do this line below.  Watch your warnings.  It complained
         * that this is a warning.  This is a bug and can result in random behavior.
         * See me.
         * @todo We'll investigate what is the best way to deal with the name variable when
         * we come back to work on analyzing the DWARF.
         */
        //name = "<no DW_AT_name attr>";
        name = (char*) malloc(20);
        strcpy(name, "<no DW_AT_name attr>");
        localname = 1;
    }

    else
    {
        /* Do nothing */
    }

    if(tag != DW_TAG_structure_type)
    {
        res = dwarf_tag(print_me, &tag, &error);
        if(res != DW_DLV_OK)
        {
            logger.logError("Error in dwarf_tag , level %d.  errno=%u %s", level, dwarf_errno(error),
                    dwarf_errmsg(error));
            return_value = JUICER_ERROR;
        }
    }

    res = dwarf_get_TAG_name(tag, &tagname);

    if(res != DW_DLV_OK)
    {
        logger.logError("Error in dwarf_get_TAG_name , level %d.  errno=%u %s", level, dwarf_errno(error),
                    dwarf_errmsg(error));
        return_value = JUICER_ERROR;
    }

    res = dwarf_bytesize(print_me, &bytesize, &error);
    if(DW_DLV_OK == res)
    {
        logger.logDebug(" byte size for %s is %u", name, bytesize);
    }

    res = dwarf_bitsize(print_me, &bytesize, &error);
    if(DW_DLV_OK == res)
    {
        logger.logDebug(" bit size for %s is %u", name, bytesize);
    }

    if(DW_TAG_member == tag)
    {
        //Get the size and type of this struct member

        res = dwarf_attr(print_me, in_attr, &attr_struct, &error);

        if(DW_DLV_OK == res)
        {
            /**
             * @todo We have to investigate this further when we come back to work
             * on the DWARF. On this block we should be inspecting attr_struct to
             * get attributes that interest us from the struct member
             * such as DW_AT_byte_size.
             */
        }
    }

    /* Log a nicely formatted string with indentation representing levels. */
    for(uint32_t i = 0; i < level; ++i)
    {
        outputText.append("  ");
    }
    outputText.append("tag: %d %s  name: \"%s\"");
    logger.logDebug(outputText.c_str(), tag, tagname, name);

    if(!localname)
    {
        dwarf_dealloc(dbg, name, DW_DLA_STRING);
    }

    return return_value;
}

/**
 *@brief Gets the endianness from the elfFile.
 */
JuicerEndianness_t Juicer::getEndianness()
{
    Elf *elf = NULL;
    unsigned char *ident_buffer = NULL;
    char* buffer = NULL;
    size_t size = 0;
    JuicerEndianness_t rc;

    Elf64_Ehdr* elf_hdr_64 = 0;
    Elf32_Ehdr* elf_hdr_32 = 0;

    elf_version(EV_CURRENT);

    elf = elf_begin(elfFile, ELF_C_READ, NULL);

    buffer = elf_getident(elf, &size);

    if(buffer[EI_CLASS] == ELFCLASS64)
    {
		if(elf != NULL)
		{
			elf_hdr_64 = elf64_getehdr(elf);

			ident_buffer = elf_hdr_64->e_ident;

			if(ident_buffer[EI_DATA] == 0)
			{
				rc = JUICER_ENDIAN_BIG;
			}
			else if(ident_buffer[EI_DATA] == 1)
			{
				rc = JUICER_ENDIAN_LITTLE;
			}
			else
			{
				rc = JUICER_ENDIAN_UNKNOWN;
			}
			elf_end(elf);
		}
		else
		{
			logger.logError("elf_begin failed.  errno=%d  %s", errno,
					strerror(errno));
		}

    }
    else if(buffer[EI_CLASS] == ELFCLASS32)
    {
		if(elf != NULL)
		{
			elf_hdr_32 = elf32_getehdr(elf);

			ident_buffer = elf_hdr_32->e_ident;

			if(ident_buffer[EI_DATA] == 0)
			{
				rc = JUICER_ENDIAN_BIG;
			}
			else if(ident_buffer[EI_DATA] == 1)
			{
				rc = JUICER_ENDIAN_LITTLE;
			}
			else
			{
				rc = JUICER_ENDIAN_UNKNOWN;
			}
			elf_end(elf);
		}
		else
		{
			logger.logError("elf_begin failed.  errno=%d  %s", errno,
					strerror(errno));
		}
    }
    else
    {
    	// empty
    }


    return rc;

}

Juicer::~Juicer()
{
// TODO Auto-generated destructor stub
}

bool Juicer::isIDCSet(void)
{
    bool rc = true;

    if(idc == 0)
    {
        logger.logError("IDC is not set");
        rc = false;
    }

    return rc;
}

/**
 *@brief Loads the ELF file into Juicer and finds out its endianness.
 *@param elfFilePath The path of the ELF file.
 *@return JUICER_OK if the file was opened successfully
 *and its endianness is identified. Returns JUICER_ERROR if either there was
 *an error opening the file or its endianness is unknown.
 */
int Juicer::parse( std::string& elfFilePath)
{
    int return_value = JUICER_OK;

    /* Don't even continue if the IDC is not set. */
    if(isIDCSet())
    {
        JuicerEndianness_t      endianness;
        int                     dwarf_value = DW_DLV_OK;
        /**@note elf's lifetime is tied to parser's scope. */
        std::unique_ptr<ElfFile> elf = std::make_unique<ElfFile>(elfFilePath);

        elfFile = open(elfFilePath.c_str(), O_RDONLY);
        if(elfFile < 0)
        {
            logger.logError("Failed to load '%s'.  (%d) %s.", elfFilePath.c_str(),
                errno, strerror(errno));
            return_value = JUICER_ERROR;
        }
        else
        {
            logger.logDebug("Opened file '%s'.  fd=%u", elfFilePath.c_str(), elfFile);
        }

        if(JUICER_OK == return_value)
        {
            /* Initialize the Dwarf library.  This will open the file. */
            dwarf_value = dwarf_init(elfFile, DW_DLC_READ, errhand, errarg, &dbg,
                    &error);
            if(dwarf_value != DW_DLV_OK)
            {
                logger.logError("Failed to read the dwarf");
                return_value = JUICER_ERROR;
            }
        }

        if(JUICER_OK == return_value)
        {
            /* Get the endianness. */
            endianness = getEndianness();

            /**
             *@note For now, the checksum is always done.
             */
            int checkSum = 0;
            std::string date {""};

            elf->setChecksum(checkSum);
            elf->isLittleEndian(JUICER_ENDIAN_BIG == endianness?
            					false: true);
            elf->setDate(date);

            if(JUICER_ENDIAN_BIG == endianness)
            {
                logger.logDebug("Detected big endian.");
                elf->isLittleEndian(false);
            }
            else if(JUICER_ENDIAN_LITTLE == endianness)
            {
                logger.logDebug("Detected little endian.");
                elf->isLittleEndian( true);
            }
            else
            {
                logger.logError("Endian is unknown. Aborting parse.");
                return_value = JUICER_ERROR;
            }
        }

        if(JUICER_OK == return_value)
        {
            return_value = readCUList(*elf.get(), dbg);

            dwarf_value = dwarf_finish(dbg, &error);
            if(dwarf_value != DW_DLV_OK)
            {
                logger.logWarning("dwarf_finish failed.  errno=%u  %s", errno, strerror(errno));
            }

            close(elfFile);
        }

        if(JUICER_OK == return_value)
        {
            /* All done.  Write it out. */
            logger.logDebug("Parsing of elf file '%s' is complete.  Writing to data container.", elfFilePath.c_str());
            return_value  = idc->write(*elf.get());
        }
    }

    return return_value;
}

void Juicer::setIDC(IDataContainer *inIdc)
{
    idc = inIdc;
}
