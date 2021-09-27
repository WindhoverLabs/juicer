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

#include <string.h>
#include <errno.h>
#include <libelf.h>
#include <ctype.h>
#include <memory.h>
#include <cmath>
#include <iostream>
#include <vector>
#include <numeric>
#include <string>
#include <functional>

#include "Juicer.h"
#include "IDataContainer.h"
#include "Symbol.h"
#include "Field.h"
#include "Enumeration.h"
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
	std::vector<Dimension> 		dimList{};
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
				logger.logError("Error in dwarf_formudata.  line=%u  errno=%u %s", __LINE__, dwarf_errno(error),
						dwarf_errmsg(error));
			}
		}

		/* Set the multiplicity, the array's size. */
		if(res == DW_DLV_OK)
		{
			dimList = getDimList(dbg, dieSubrangeType);
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

			Symbol* 	arraySymbol =  getBaseTypeSymbol(elf, inDie, dimList);

			if(nullptr == arraySymbol)
			{
				res = DW_DLV_ERROR;

				logger.logError("Base type not found for %s", arrayName);
			}
			else
			{
				std::string arrayBaseType{arraySymbol->getName().c_str()};
				outSymbol = elf.getSymbol(arrayBaseType);
				outSymbol->addField(stdString, 0, *outSymbol, dimList, elf.isLittleEndian());
			}
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
    Dwarf_Attribute attr_struct = nullptr;
    Dwarf_Off       typeOffset = 0;
    Dwarf_Die       typeDie = nullptr;
    char            *typeDieName;

    /* Get the type attribute. */

    res = dwarf_attr(inDie, DW_AT_type, &attr_struct, &error);
    if(res != DW_DLV_OK)
    {
        logger.logDebug("Ignoring error in dwarf_attr(DW_AT_type). %u  errno=%u %s", __LINE__, dwarf_errno(error),
            dwarf_errmsg(error));

        int voidRes = dwarf_attr(inDie, DW_AT_byte_size, &attr_struct, &error);

        if(voidRes != DW_DLV_OK)
        {
            logger.logDebug("Ignoring error in dwarf_attr(DW_AT_byte_size). %u  errno=%u %s", __LINE__, dwarf_errno(error),
                dwarf_errmsg(error));
        }
        else
        {
            Dwarf_Unsigned byteSize = 0;
            std::string voidType{"void*"};
            voidRes = dwarf_formudata(attr_struct, &byteSize, &error );

        	if(DW_DLV_OK == voidRes)
        	{
            	/* This branch represents a "void*" since there is no valid type.
            	 * Read section 5.2 of DWARF4 for details on this.*/
                outSymbol = elf.addSymbol(voidType, byteSize);

        	}
        }

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



Symbol * Juicer::getBaseTypeSymbol(ElfFile &elf, Dwarf_Die inDie, std::vector<Dimension> &dimList)
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
        res = dwarf_tag(typeDie, &tag, &error);
        if(res != DW_DLV_OK)
        {
            logger.logError("Error in dwarf_tag.  %u  errno=%u %s", __LINE__, dwarf_errno(error),
                dwarf_errmsg(error));
        }
    }

    if(res == DW_DLV_OK)
    {
        switch(tag)
        {

            case DW_TAG_pointer_type:
            {
                outSymbol = process_DW_TAG_pointer_type(elf, dbg, typeDie);
                break;
            }

            case DW_TAG_structure_type:
            {
                Dwarf_Bool     structHasName = false;
                Dwarf_Bool     parentHasName = false;
                Dwarf_Unsigned byteSize = 0;

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
                break;
            }

            case DW_TAG_typedef:
            {
                outSymbol = process_DW_TAG_typedef(elf, dbg, typeDie);
                break;
            }

            case DW_TAG_enumeration_type:
            {
                Dwarf_Bool     structHasName = false;
                Dwarf_Bool     parentHasName = false;
                Dwarf_Unsigned byteSize = 0;

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
                /* First get the base type itself. */
                outSymbol = getBaseTypeSymbol(elf, typeDie, dimList);

				/* Set the multiplicity argument. */
				if(res == DW_DLV_OK)
				{
					dimList = getDimList(dbg, typeDie);
				}

                break;
            }

            case DW_TAG_class_type:
            {
                /* TODO */
                break;
            }

            case DW_TAG_const_type:
            {
                /* TODO */
                /* Get the type attribute. */
                res = dwarf_attr(inDie, DW_AT_type, &attr_struct, &error);

                getBaseTypeSymbol(elf, typeDie, dimList);

                break;
            }

            case DW_TAG_reference_type:
            {
                /* TODO */
                break;
            }

            case DW_TAG_union_type:
            {
                /* TODO */
                break;
            }


            /* Fallthru */
            case DW_TAG_unspecified_type:
            {
            	break;
            }
            case DW_TAG_rvalue_reference_type:
            {
                /* Ignore these tags. */
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

void Juicer::DisplayDie(Dwarf_Die inDie, uint32_t level)
{
    int             res = DW_DLV_OK;
    Dwarf_Attribute *attribs;
    Dwarf_Signed    attribCount = 0;
    Dwarf_Off       globalOffset;
    Dwarf_Off       localOffset;
    Dwarf_Attribute attr_struct;
    char            *dieName = 0;
    Dwarf_Half      tag = 0;
    int             abbrevCode = 0;
    Dwarf_Half      hasChildrenFlag = 0;
    char            tagName[255];
    char            output[2000];
    char            line[255];

    if(inDie != 0)
    {
        res = dwarf_tag(inDie, &tag, &error);
        if(res == DW_DLV_OK)
        {
            switch(tag)
            {
                case DW_TAG_array_type:
                	sprintf(tagName, "DW_TAG_array_type");
                    break;

                case DW_TAG_class_type:
                	sprintf(tagName, "DW_TAG_class_type");
                    break;

                case DW_TAG_entry_point:
                	sprintf(tagName, "DW_TAG_entry_point");
                    break;

                case DW_TAG_enumeration_type:
                	sprintf(tagName, "DW_TAG_enumeration_type");
                    break;

                case DW_TAG_formal_parameter:
                	sprintf(tagName, "DW_TAG_formal_parameter");
                    break;

                case DW_TAG_imported_declaration:
                	sprintf(tagName, "DW_TAG_imported_declaration");
                    break;

                case DW_TAG_label:
                	sprintf(tagName, "DW_TAG_label");
                    break;

                case DW_TAG_lexical_block:
                	sprintf(tagName, "DW_TAG_lexical_block");
                    break;

                case DW_TAG_member:
                	sprintf(tagName, "DW_TAG_member");
                    break;

                case DW_TAG_pointer_type:
                	sprintf(tagName, "DW_TAG_pointer_type");
                    break;

                case DW_TAG_reference_type:
                	sprintf(tagName, "DW_TAG_reference_type");
                    break;

                case DW_TAG_compile_unit:
                	sprintf(tagName, "DW_TAG_compile_unit");
                    break;

                case DW_TAG_string_type:
                	sprintf(tagName, "DW_TAG_string_type");
                    break;

                case DW_TAG_structure_type:
                	sprintf(tagName, "DW_TAG_structure_type");
                    break;

                case DW_TAG_subroutine_type:
                	sprintf(tagName, "DW_TAG_subroutine_type");
                    break;

                case DW_TAG_typedef:
                	sprintf(tagName, "DW_TAG_typedef");
                    break;

                case DW_TAG_union_type:
                	sprintf(tagName, "DW_TAG_union_type");
                    break;

                case DW_TAG_unspecified_parameters:
                	sprintf(tagName, "DW_TAG_unspecified_parameters");
                    break;

                case DW_TAG_variant:
                	sprintf(tagName, "DW_TAG_variant");
                    break;

                case DW_TAG_common_block:
                	sprintf(tagName, "DW_TAG_common_block");
                    break;

                case DW_TAG_common_inclusion:
                	sprintf(tagName, "DW_TAG_common_inclusion");
                    break;

                case DW_TAG_inheritance:
                	sprintf(tagName, "DW_TAG_inheritance");
                    break;

                case DW_TAG_inlined_subroutine:
                	sprintf(tagName, "DW_TAG_inlined_subroutine");
                    break;

                case DW_TAG_module:
                	sprintf(tagName, "DW_TAG_module");
                    break;

                case DW_TAG_ptr_to_member_type:
                	sprintf(tagName, "DW_TAG_ptr_to_member_type");
                    break;

                case DW_TAG_set_type:
                	sprintf(tagName, "DW_TAG_set_type");
                    break;

                case DW_TAG_subrange_type:
                	sprintf(tagName, "DW_TAG_subrange_type");
                    break;

                case DW_TAG_with_stmt:
                	sprintf(tagName, "DW_TAG_with_stmt");
                    break;

                case DW_TAG_access_declaration:
                	sprintf(tagName, "DW_TAG_access_declaration");
                    break;

                case DW_TAG_base_type:
                	sprintf(tagName, "DW_TAG_base_type");
                    break;

                case DW_TAG_catch_block:
                	sprintf(tagName, "DW_TAG_catch_block");
                    break;

                case DW_TAG_const_type:
                	sprintf(tagName, "DW_TAG_const_type");
                    break;

                case DW_TAG_constant:
                	sprintf(tagName, "DW_TAG_constant");
                    break;

                case DW_TAG_enumerator:
                	sprintf(tagName, "DW_TAG_enumerator");
                    break;

                case DW_TAG_file_type:
                	sprintf(tagName, "DW_TAG_file_type");
                    break;

                case DW_TAG_friend:
                	sprintf(tagName, "DW_TAG_friend");
                    break;

                case DW_TAG_namelist:
                	sprintf(tagName, "DW_TAG_namelist");
                    break;

                case DW_TAG_namelist_item:
                	sprintf(tagName, "DW_TAG_namelist_item");
                    break;

                case DW_TAG_packed_type:
                	sprintf(tagName, "DW_TAG_packed_type");
                    break;

                case DW_TAG_subprogram:
                	sprintf(tagName, "DW_TAG_subprogram");
                    break;

                case DW_TAG_template_type_parameter:
                	sprintf(tagName, "DW_TAG_template_type_parameter");
                    break;

                case DW_TAG_template_value_parameter:
                	sprintf(tagName, "DW_TAG_template_value_parameter");
                    break;

                case DW_TAG_thrown_type:
                	sprintf(tagName, "DW_TAG_thrown_type");
                    break;

                case DW_TAG_try_block:
                	sprintf(tagName, "DW_TAG_try_block");
                    break;

                case DW_TAG_variant_part:
                	sprintf(tagName, "DW_TAG_variant_part");
                    break;

                case DW_TAG_variable:
                	sprintf(tagName, "DW_TAG_variable");
                    break;

                case DW_TAG_volatile_type:
                	sprintf(tagName, "DW_TAG_volatile_type");
                    break;

                case DW_TAG_dwarf_procedure:
                	sprintf(tagName, "DW_TAG_dwarf_procedure");
                    break;

                case DW_TAG_restrict_type:
                	sprintf(tagName, "DW_TAG_restrict_type");
                    break;

                case DW_TAG_interface_type:
                	sprintf(tagName, "DW_TAG_interface_type");
                    break;

                case DW_TAG_namespace:
                	sprintf(tagName, "DW_TAG_namespace");
                    break;

                case DW_TAG_imported_module:
                	sprintf(tagName, "DW_TAG_imported_module");
                    break;

                case DW_TAG_unspecified_type:
                	sprintf(tagName, "DW_TAG_unspecified_type");
                    break;

                case DW_TAG_partial_unit:
                	sprintf(tagName, "DW_TAG_partial_unit");
                    break;

                case DW_TAG_imported_unit:
                	sprintf(tagName, "DW_TAG_imported_unit");
                    break;

                case DW_TAG_mutable_type:
                	sprintf(tagName, "DW_TAG_mutable_type");
                    break;

                case DW_TAG_condition:
                	sprintf(tagName, "DW_TAG_condition");
                    break;

                case DW_TAG_shared_type:
                	sprintf(tagName, "DW_TAG_shared_type");
                    break;

                case DW_TAG_type_unit:
                	sprintf(tagName, "DW_TAG_type_unit");
                    break;

                case DW_TAG_rvalue_reference_type:
                	sprintf(tagName, "DW_TAG_rvalue_reference_type");
                    break;

                case DW_TAG_template_alias:
                	sprintf(tagName, "DW_TAG_template_alias");
                    break;

                case DW_TAG_coarray_type:
                	sprintf(tagName, "DW_TAG_coarray_type");
                    break;

                case DW_TAG_generic_subrange:
                	sprintf(tagName, "DW_TAG_generic_subrange");
                    break;

                case DW_TAG_dynamic_type:
                	sprintf(tagName, "DW_TAG_dynamic_type");
                    break;

                case DW_TAG_atomic_type:
                	sprintf(tagName, "DW_TAG_dynamic_type");
                    break;

                case DW_TAG_call_site:
                	sprintf(tagName, "DW_TAG_call_site");
                    break;

                case DW_TAG_call_site_parameter:
                	sprintf(tagName, "DW_TAG_call_site_parameter");
                    break;

                case DW_TAG_skeleton_unit:
                	sprintf(tagName, "DW_TAG_skeleton_unit");
                    break;

                case DW_TAG_immutable_type:
                	sprintf(tagName, "DW_TAG_immutable_type");
                    break;

                default:
                	sprintf(tagName, "UNKNOWN (0x%04x)", tag);
                    break;
            }
        }
        else
        {
        	sprintf(tagName, "<< error >>");
        }

        res = dwarf_die_offsets(inDie, &globalOffset, &localOffset, &error);
        if(res != DW_DLV_OK)
        {
            logger.logError("Error in dwarf_die_offsets.  errno=%u %s", dwarf_errno(error),
                    dwarf_errmsg(error));
        }

        abbrevCode = dwarf_die_abbrev_code(inDie);


        res = dwarf_die_abbrev_children_flag(inDie, &hasChildrenFlag);
        if(res != DW_DLV_OK)
        {
            logger.logError("Error in dwarf_die_abbrev_children_flag.  errno=%u %s", dwarf_errno(error),
                    dwarf_errmsg(error));
        }
        else
        {
        	logger.logDebug("  Has children:        %s\n", hasChildrenFlag ? "True" : "False");
        }

        res = dwarf_die_abbrev_children_flag(inDie, &hasChildrenFlag);
        if(res != DW_DLV_OK)
        {
            logger.logError("Error in dwarf_die_abbrev_children_flag.  errno=%u %s", dwarf_errno(error),
                    dwarf_errmsg(error));
        }
        else
        {
        	logger.logDebug("  Has children:        %s\n", hasChildrenFlag ? "True" : "False");
        }

        sprintf(line, "<%u><%x>: Abbrev Number: %u (%s)\n", level, globalOffset, abbrevCode, tagName);
        strcpy(output, line);

        res = dwarf_attrlist(inDie, &attribs, &attribCount, &error);
        if(res != DW_DLV_OK)
        {
        	if(res == DW_DLV_ERROR)
        	{
        		logger.logError("Error in dwarf_attrlist.  errno=%u %s", dwarf_errno(error),
        				dwarf_errmsg(error));
        	}
        	else if(res == DW_DLV_NO_ENTRY)
        	{
        		logger.logWarning("No Entry in dwarf_attrlist.  errno=%u %s", dwarf_errno(error),
        		        				dwarf_errmsg(error));
        	}
        }
        else
        {
            if(attribCount > 0)
            {
                for(uint32_t i = 0; i < attribCount; ++i)
                {
                    Dwarf_Half attrNum;
                    char       attribName[255];
                    char       formName[50];
                    char       value[50];

                    strcpy(value, "<< Form Not Supported >>");

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
                            	strcpy(attribName, "DW_AT_sibling");
                                break;

                            case DW_AT_location:
                            	strcpy(attribName, "DW_AT_location");
                                break;

                            case DW_AT_name:
                            	strcpy(attribName, "DW_AT_name");
                                break;

                            case DW_AT_ordering:
                            	strcpy(attribName, "DW_AT_ordering");
                                break;

                            case DW_AT_subscr_data:
                            	strcpy(attribName, "DW_AT_subscr_data");
                                break;

                            case DW_AT_byte_size:
                            	strcpy(attribName, "DW_AT_byte_size");
                                break;

                            case DW_AT_bit_offset:
                            	strcpy(attribName, "DW_AT_bit_offset");
                                break;

                            case DW_AT_bit_size:
                            	strcpy(attribName, "DW_AT_bit_size");
                                break;

                            case DW_AT_element_list:
                            	strcpy(attribName, "DW_AT_element_list");
                                break;

                            case DW_AT_stmt_list:
                            	strcpy(attribName, "DW_AT_stmt_list");
                                break;

                            case DW_AT_low_pc:
                            	strcpy(attribName, "DW_AT_low_pc");
                                break;

                            case DW_AT_high_pc:
                            	strcpy(attribName, "DW_AT_high_pc");
                                break;

                            case DW_AT_language:
                            	strcpy(attribName, "DW_AT_language");
                                break;

                            case DW_AT_member:
                            	strcpy(attribName, "DW_AT_member");
                                break;

                            case DW_AT_discr:
                            	strcpy(attribName, "DW_AT_discr");
                                break;

                            case DW_AT_discr_value:
                            	strcpy(attribName, "DW_AT_discr_value");
                                break;

                            case DW_AT_visibility:
                            	strcpy(attribName, "DW_AT_visibility");
                                break;

                            case DW_AT_import:
                            	strcpy(attribName, "DW_AT_import");
                                break;

                            case DW_AT_string_length:
                            	strcpy(attribName, "DW_AT_string_length");
                                break;

                            case DW_AT_common_reference:
                            	strcpy(attribName, "DW_AT_common_reference");
                                break;

                            case DW_AT_comp_dir:
                            	strcpy(attribName, "DW_AT_comp_dir");
                                break;

                            case DW_AT_const_value:
                            	strcpy(attribName, "DW_AT_const_value");
                                break;

                            case DW_AT_containing_type:
                            	strcpy(attribName, "DW_AT_containing_type");
                                break;

                            case DW_AT_default_value:
                            	strcpy(attribName, "DW_AT_default_value");
                                break;

                            case DW_AT_inline:
                            	strcpy(attribName, "DW_AT_inline");
                                break;

                            case DW_AT_is_optional:
                            	strcpy(attribName, "DW_AT_is_optional");
                                break;

                            case DW_AT_lower_bound:
                            	strcpy(attribName, "DW_AT_lower_bound");
                                break;

                            case DW_AT_producer:
                            	strcpy(attribName, "DW_AT_producer");
                                break;

                            case DW_AT_prototyped:
                            	strcpy(attribName, "DW_AT_prototyped");
                                break;

                            case DW_AT_return_addr:
                            	strcpy(attribName, "DW_AT_return_addr");
                                break;

                            case DW_AT_start_scope:
                            	strcpy(attribName, "DW_AT_start_scope");
                                break;

                            case DW_AT_bit_stride:
                            	strcpy(attribName, "DW_AT_bit_stride");
                                break;

                            case DW_AT_upper_bound:
                            	strcpy(attribName, "DW_AT_upper_bound");
                                break;

                            case DW_AT_abstract_origin:
                            	strcpy(attribName, "DW_AT_abstract_origin");
                                break;

                            case DW_AT_accessibility:
                            	strcpy(attribName, "DW_AT_accessibility");
                                break;

                            case DW_AT_address_class:
                            	strcpy(attribName, "DW_AT_address_class");
                                break;

                            case DW_AT_artificial:
                            	strcpy(attribName, "DW_AT_artificial");
                                break;

                            case DW_AT_base_types:
                            	strcpy(attribName, "DW_AT_base_types");
                                break;

                            case DW_AT_calling_convention:
                            	strcpy(attribName, "DW_AT_calling_convention");
                                break;

                            case DW_AT_count:
                            	strcpy(attribName, "DW_AT_count");
                                break;

                            case DW_AT_data_member_location:
                            	strcpy(attribName, "DW_AT_data_member_location");
                                break;

                            case DW_AT_decl_column:
                            	strcpy(attribName, "DW_AT_decl_column");
                                break;

                            case DW_AT_decl_file:
                            	strcpy(attribName, "DW_AT_decl_file");
                                break;

                            case DW_AT_decl_line:
                            	strcpy(attribName, "DW_AT_decl_line");
                                break;

                            case DW_AT_declaration:
                            	strcpy(attribName, "DW_AT_declaration");
                                break;

                            case DW_AT_discr_list:
                            	strcpy(attribName, "DW_AT_discr_list");
                                break;

                            case DW_AT_encoding:
                            	strcpy(attribName, "DW_AT_encoding");
                                break;

                            case DW_AT_external:
                            	strcpy(attribName, "DW_AT_external");
                                break;

                            case DW_AT_frame_base:
                            	strcpy(attribName, "DW_AT_frame_base");
                                break;

                            case DW_AT_friend:
                            	strcpy(attribName, "DW_AT_friend");
                                break;

                            case DW_AT_identifier_case:
                            	strcpy(attribName, "DW_AT_identifier_case");
                                break;

                            case DW_AT_macro_info:
                            	strcpy(attribName, "DW_AT_macro_info");
                                break;

                            case DW_AT_namelist_item:
                            	strcpy(attribName, "DW_AT_namelist_item");
                                break;

                            case DW_AT_priority:
                            	strcpy(attribName, "DW_AT_priority");
                                break;

                            case DW_AT_segment:
                            	strcpy(attribName, "DW_AT_segment");
                                break;

                            case DW_AT_specification:
                            	strcpy(attribName, "DW_AT_specification");
                                break;

                            case DW_AT_static_link:
                            	strcpy(attribName, "DW_AT_static_link");
                                break;

                            case DW_AT_type:
                            	strcpy(attribName, "DW_AT_type");
                                break;

                            case DW_AT_use_location:
                            	strcpy(attribName, "DW_AT_use_location");
                                break;

                            case DW_AT_variable_parameter:
                            	strcpy(attribName, "DW_AT_variable_parameter");
                                break;

                            case DW_AT_virtuality:
                            	strcpy(attribName, "DW_AT_virtuality");
                                break;

                            case DW_AT_vtable_elem_location:
                            	strcpy(attribName, "DW_AT_vtable_elem_location");
                                break;

                            case DW_AT_allocated:
                            	strcpy(attribName, "DW_AT_allocated");
                                break;

                            case DW_AT_associated:
                            	strcpy(attribName, "DW_AT_associated");
                                break;

                            case DW_AT_data_location:
                            	strcpy(attribName, "DW_AT_data_location");
                                break;

                            case DW_AT_byte_stride:
                            	strcpy(attribName, "DW_AT_byte_stride");
                                break;

                            case DW_AT_entry_pc:
                            	strcpy(attribName, "DW_AT_entry_pc");
                                break;

                            case DW_AT_use_UTF8:
                            	strcpy(attribName, "DW_AT_use_UTF8");
                                break;

                            case DW_AT_extension:
                            	strcpy(attribName, "DW_AT_extension");
                                break;

                            case DW_AT_ranges:
                            	strcpy(attribName, "DW_AT_ranges");
                                break;

                            case DW_AT_trampoline:
                            	strcpy(attribName, "DW_AT_trampoline");
                                break;

                            case DW_AT_call_column:
                            	strcpy(attribName, "DW_AT_call_column");
                                break;

                            case DW_AT_call_file:
                            	strcpy(attribName, "DW_AT_call_file");
                                break;

                            case DW_AT_call_line:
                            	strcpy(attribName, "DW_AT_call_line");
                                break;

                            case DW_AT_description:
                            	strcpy(attribName, "DW_AT_description");
                                break;

                            case DW_AT_binary_scale:
                            	strcpy(attribName, "DW_AT_binary_scale");
                                break;

                            case DW_AT_decimal_scale:
                            	strcpy(attribName, "DW_AT_decimal_scale");
                                break;

                            case DW_AT_small:
                            	strcpy(attribName, "DW_AT_small");
                                break;

                            case DW_AT_decimal_sign:
                            	strcpy(attribName, "DW_AT_decimal_sign");
                                break;

                            case DW_AT_digit_count:
                            	strcpy(attribName, "DW_AT_digit_count");
                                break;

                            case DW_AT_picture_string:
                            	strcpy(attribName, "DW_AT_picture_string");
                                break;

                            case DW_AT_mutable:
                            	strcpy(attribName, "DW_AT_mutable");
                                break;

                            case DW_AT_threads_scaled:
                            	strcpy(attribName, "DW_AT_threads_scaled");
                                break;

                            case DW_AT_explicit:
                            	strcpy(attribName, "DW_AT_explicit");
                                break;

                            case DW_AT_object_pointer:
                            	strcpy(attribName, "DW_AT_object_pointer");
                                break;

                            case DW_AT_endianity:
                            	strcpy(attribName, "DW_AT_endianity");
                                break;

                            case DW_AT_elemental:
                            	strcpy(attribName, "DW_AT_elemental");
                                break;

                            case DW_AT_pure:
                            	strcpy(attribName, "DW_AT_pure");
                                break;

                            case DW_AT_recursive:
                            	strcpy(attribName, "DW_AT_recursive");
                                break;

                            case DW_AT_signature:
                            	strcpy(attribName, "DW_AT_signature");
                                break;

                            case DW_AT_main_subprogram:
                            	strcpy(attribName, "DW_AT_main_subprogram");
                                break;

                            case DW_AT_data_bit_offset:
                            	strcpy(attribName, "DW_AT_data_bit_offset");
                                break;

                            case DW_AT_const_expr:
                            	strcpy(attribName, "DW_AT_const_expr");
                                break;

                            case DW_AT_enum_class:
                            	strcpy(attribName, "DW_AT_enum_class");
                                break;

                            case DW_AT_linkage_name:
                            	strcpy(attribName, "DW_AT_linkage_name");
                                break;

                            case DW_AT_string_length_bit_size:
                            	strcpy(attribName, "DW_AT_string_length_bit_size");
                                break;

                            case DW_AT_string_length_byte_size:
                            	strcpy(attribName, "DW_AT_string_length_byte_size");
                                break;

                            case DW_AT_rank:
                            	strcpy(attribName, "DW_AT_rank");
                                break;

                            case DW_AT_str_offsets_base:
                            	strcpy(attribName, "DW_AT_str_offsets_base");
                                break;

                            case DW_AT_addr_base:
                            	strcpy(attribName, "DW_AT_addr_base");
                                break;

                            case DW_AT_rnglists_base:
                            	strcpy(attribName, "DW_AT_rnglists_base");
                                break;

                            case DW_AT_dwo_id:
                            	strcpy(attribName, "DW_AT_dwo_id");
                                break;

                            case DW_AT_dwo_name:
                            	strcpy(attribName, "DW_AT_dwo_name");
                                break;

                            case DW_AT_reference:
                            	strcpy(attribName, "DW_AT_reference");
                                break;

                            case DW_AT_rvalue_reference:
                            	strcpy(attribName, "DW_AT_rvalue_reference");
                                break;

                            case DW_AT_macros:
                            	strcpy(attribName, "DW_AT_macros");
                                break;

                            case DW_AT_call_all_calls:
                            	strcpy(attribName, "DW_AT_call_all_calls");
                                break;

                            case DW_AT_call_all_source_calls:
                            	strcpy(attribName, "DW_AT_call_all_source_calls");
                                break;

                            case DW_AT_call_all_tail_calls:
                            	strcpy(attribName, "DW_AT_call_all_tail_calls");
                                break;

                            case DW_AT_call_return_pc:
                            	strcpy(attribName, "DW_AT_call_return_pc");
                                break;

                            case DW_AT_call_value:
                            	strcpy(attribName, "DW_AT_call_value");
                                break;

                            case DW_AT_call_origin:
                            	strcpy(attribName, "DW_AT_call_origin");
                                break;

                            case DW_AT_call_parameter:
                            	strcpy(attribName, "DW_AT_call_parameter");
                                break;

                            case DW_AT_call_pc:
                            	strcpy(attribName, "DW_AT_call_pc");
                                break;

                            case DW_AT_call_tail_call:
                            	strcpy(attribName, "DW_AT_call_tail_call");
                                break;

                            case DW_AT_call_target:
                            	strcpy(attribName, "DW_AT_call_target");
                                break;

                            case DW_AT_call_target_clobbered:
                            	strcpy(attribName, "DW_AT_call_target_clobbered");
                                break;

                            case DW_AT_call_data_location:
                            	strcpy(attribName, "DW_AT_call_data_location");
                                break;

                            case DW_AT_call_data_value:
                            	strcpy(attribName, "DW_AT_call_data_value");
                                break;

                            case DW_AT_noreturn:
                            	strcpy(attribName, "DW_AT_noreturn");
                                break;

                            case DW_AT_alignment:
                            	strcpy(attribName, "DW_AT_alignment");
                                break;

                            case DW_AT_export_symbols:
                            	strcpy(attribName, "DW_AT_export_symbols");
                                break;

                            case DW_AT_deleted:
                            	strcpy(attribName, "DW_AT_deleted");
                                break;

                            case DW_AT_defaulted:
                            	strcpy(attribName, "DW_AT_defaulted");
                                break;

                            case DW_AT_loclists_base:
                            	strcpy(attribName, "DW_AT_loclists_base");
                                break;

                            default:
							    sprintf(attribName, "UNKNOWN (%x)", attrNum);
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
                                    {
                                    	Dwarf_Addr addr = 0;

                                    	strcpy(formName, "DW_FORM_addr");

                                        res = dwarf_formaddr(attribs[i], &addr, &error);
                                        if(res != DW_DLV_OK)
                                        {
                            				logger.logError("Error in DW_FORM_addr.  line=%u  errno=%u %s", __LINE__, dwarf_errno(error),
                                                    dwarf_errmsg(error));
                                        }
                                        else
                                        {
                                            unsigned long long data = (unsigned int) addr;
                                            sprintf(value, "0x%016x", data);
                                        }

                                        break;
                                    }

                                    case DW_FORM_block2:
                                    {
                                        Dwarf_Unsigned udata = 0;

                                    	strcpy(formName, "DW_FORM_block2");

                                        res = dwarf_formudata(attribs[i], &udata, &error);
                                        if(res != DW_DLV_OK)
                                        {
                            				logger.logError("Error in dwarf_formudata.  line=%u  errno=%u %s", __LINE__, dwarf_errno(error),
                                                    dwarf_errmsg(error));
                                        }
                                        else
                                        {
                                            unsigned short data = (unsigned short) udata;
                                            sprintf(value, "%d", data);
                                        }

                                        break;
                                    }

                                    case DW_FORM_block4:
                                    {
                                        Dwarf_Unsigned udata = 0;

                                    	strcpy(formName, "DW_FORM_block4");

                                        res = dwarf_formudata(attribs[i], &udata, &error);
                                        if(res != DW_DLV_OK)
                                        {
                            				logger.logError("Error in DW_FORM_block4.  line=%u  errno=%u %s", __LINE__, dwarf_errno(error),
                                                    dwarf_errmsg(error));
                                        }
                                        else
                                        {
                                            unsigned int data = (unsigned int) udata;
                                            sprintf(value, "%d", data);
                                        }

                                        break;
                                    }

                                    case DW_FORM_data2:
                                    {
                                        Dwarf_Unsigned udata = 0;

                                    	strcpy(formName, "DW_FORM_data2");

                                        res = dwarf_formudata(attribs[i], &udata, &error);
                                        if(res != DW_DLV_OK)
                                        {
                            				logger.logError("Error in DW_FORM_data2.  line=%u  errno=%u %s", __LINE__, dwarf_errno(error),
                                                    dwarf_errmsg(error));
                                        }
                                        else
                                        {
                                            unsigned short int data = (unsigned short int) udata;
                                            sprintf(value, "%d", data);
                                        }

                                        break;
                                    }

                                    case DW_FORM_data4:
                                    {
                                        Dwarf_Unsigned udata = 0;

                                    	strcpy(formName, "DW_FORM_data4");
                                        res = dwarf_formudata(attribs[i], &udata, &error);
                                        if(res != DW_DLV_OK)
                                        {
                            				logger.logError("Error in dwarf_formudata.  line=%u  errno=%u %s", __LINE__, dwarf_errno(error),
                                                    dwarf_errmsg(error));
                                        }
                                        else
                                        {
                                            unsigned int data = (unsigned int) udata;
                                            sprintf(value, "%d", data);
                                        }

                                        break;
                                    }

                                    case DW_FORM_data8:
                                    {
                                        Dwarf_Unsigned udata = 0;

                                    	strcpy(formName, "DW_FORM_data8");

                                        res = dwarf_formudata(attribs[i], &udata, &error);
                                        if(res != DW_DLV_OK)
                                        {
                            				logger.logError("Error in dwarf_formudata.  line=%u  errno=%u %s", __LINE__, dwarf_errno(error),
                                                    dwarf_errmsg(error));
                                        }
                                        else
                                        {
                                            sprintf(value, "%d", udata);
                                        }

                                        break;
                                    }

                                    case DW_FORM_string:
                                    {
                                        char *str = 0;

                                    	strcpy(formName, "DW_FORM_string");

                                        res = dwarf_formstring(attribs[i], &str, &error);
                                        if(res != DW_DLV_OK)
                                        {
                                            logger.logError("Error in dwarf_formstring.  errno=%u %s", dwarf_errno(error),
                                                    dwarf_errmsg(error));
                                        }
                                        else
                                        {
                                            sprintf(value, "%s", str);
                                        }

                                        break;
                                    }

                                    case DW_FORM_block:
                                    	strcpy(formName, "DW_FORM_block");
                                        break;

                                    case DW_FORM_block1:
                                    {
                                        Dwarf_Block *bdata = 0;
                                    	strcpy(formName, "DW_FORM_block1");

                                        res = dwarf_formblock(attribs[i], &bdata, &error);
                                        if(res != DW_DLV_OK)
                                        {
                            				logger.logError("Error in dwarf_formblock.  line=%u  errno=%u %s", __LINE__, dwarf_errno(error),
                                                    dwarf_errmsg(error));
                                        }
                                        else
                                        {
                                            sprintf(value, "len: %d", bdata->bl_len);
                                        }

                                        break;
                                    }


                                    case DW_FORM_data1:
                                    {
                                        Dwarf_Unsigned udata = 0;
                                    	strcpy(formName, "DW_FORM_data1");

                                        res = dwarf_formudata(attribs[i], &udata, &error);
                                        if(res != DW_DLV_OK)
                                        {
                            				logger.logError("Error in dwarf_formudata.  line=%u  errno=%u %s", __LINE__, dwarf_errno(error),
                                                    dwarf_errmsg(error));
                                        }
                                        else
                                        {
                                            uint8_t data = (uint8_t) udata;
                                            sprintf(value, "%u", data);
                                        }

                                        break;
                                    }

                                    case DW_FORM_flag:
                                    {
                                    	Dwarf_Bool dwarfBool = false;
                                    	strcpy(formName, "DW_FORM_flag");
                                        char *strp = 0;

                                        res = dwarf_formflag(attribs[i], &dwarfBool, &error);
                                        if(res != DW_DLV_OK)
                                        {
                                            logger.logError("Error in DW_FORM_flag.  errno=%u %s", dwarf_errno(error),
                                                    dwarf_errmsg(error));
                                        }
                                        else
                                        {
                                        	if(dwarfBool)
                                        	{
                                                sprintf(value, "TRUE");
                                        	}
                                        	else
                                        	{
                                                sprintf(value, "FALSE");
                                        	}
                                        }

                                        break;
                                    }

                                    case DW_FORM_sdata:
                                    {
                                        Dwarf_Signed sdata = 0;

                                    	strcpy(formName, "DW_FORM_sdata");

                                        res = dwarf_formsdata(attribs[i], &sdata, &error);
                                        if(res != DW_DLV_OK)
                                        {
                            				logger.logError("Error in dwarf_formudata.  line=%u  errno=%u %s", __LINE__, dwarf_errno(error),
                                                    dwarf_errmsg(error));
                                        }
                                        else
                                        {
                                            sprintf(value, "%lli", sdata);
                                        }

                                        break;
                                    }

                                    case DW_FORM_strp:
                                    {
                                        char *strp = 0;

                                    	strcpy(formName, "DW_FORM_strp");

                                        res = dwarf_formstring(attribs[i], &strp, &error);
                                        if(res != DW_DLV_OK)
                                        {
                                            logger.logError("Error in dwarf_formstring.  errno=%u %s", dwarf_errno(error),
                                                    dwarf_errmsg(error));
                                        }
                                        else
                                        {
                                            char *text = dwarfStringToChar(strp);
                                            sprintf(value, "%s", text);
                                        }

                                        break;
                                    }

                                    case DW_FORM_udata:
                                    {
                                        Dwarf_Unsigned udata = 0;

                                    	strcpy(formName, "DW_FORM_udata");

                                        res = dwarf_formudata(attribs[i], &udata, &error);
                                        if(res != DW_DLV_OK)
                                        {
                            				logger.logError("Error in dwarf_formudata.  line=%u  errno=%u %s", __LINE__, dwarf_errno(error),
                                                    dwarf_errmsg(error));
                                        }
                                        else
                                        {
                                            sprintf(value, "%llu", udata);
                                        }

                                        break;
                                    }

                                    case DW_FORM_ref_addr:
                                    	strcpy(formName, "DW_FORM_ref_addr");
                                        break;

                                    case DW_FORM_ref1:
                                    {
                                        Dwarf_Off ref = 0;

                                    	strcpy(formName, "DW_FORM_ref1");
                                        res = dwarf_formref(attribs[i], &ref, &error);
                                        if(res != DW_DLV_OK)
                                        {
                            				logger.logError("Error in dwarf_formudata.  line=%u  errno=%u %s", __LINE__, dwarf_errno(error),
                                                    dwarf_errmsg(error));
                                        }
                                        else
                                        {
                                            uint8_t data = (uint8_t) ref;
                                            sprintf(value, "%u", data);
                                        }

                                        break;
                                    }

                                    case DW_FORM_ref2:
                                    {
                                        Dwarf_Off ref = 0;

                                    	strcpy(formName, "DW_FORM_ref1");
                                        res = dwarf_formref(attribs[i], &ref, &error);
                                        if(res != DW_DLV_OK)
                                        {
                            				logger.logError("Error in dwarf_formudata.  line=%u  errno=%u %s", __LINE__, dwarf_errno(error),
                                                    dwarf_errmsg(error));
                                        }
                                        else
                                        {
                                            unsigned short int data = (unsigned short int) ref;
                                            sprintf(value, "%u", data);
                                        }

                                        break;
                                    }

                                    case DW_FORM_ref4:
                                    {
                                        Dwarf_Off ref = 0;

                                    	strcpy(formName, "DW_FORM_ref1");
                                        res = dwarf_formref(attribs[i], &ref, &error);
                                        if(res != DW_DLV_OK)
                                        {
                            				logger.logError("Error in dwarf_formudata.  line=%u  errno=%u %s", __LINE__, dwarf_errno(error),
                                                    dwarf_errmsg(error));
                                        }
                                        else
                                        {
                                            unsigned int data = (unsigned int) ref;
                                            sprintf(value, "%u", data);
                                        }

                                        break;
                                    }

                                    case DW_FORM_ref8:
                                    {
                                        Dwarf_Off ref = 0;

                                    	strcpy(formName, "DW_FORM_ref1");
                                        res = dwarf_formref(attribs[i], &ref, &error);
                                        if(res != DW_DLV_OK)
                                        {
                            				logger.logError("Error in dwarf_formudata.  line=%u  errno=%u %s", __LINE__, dwarf_errno(error),
                                                    dwarf_errmsg(error));
                                        }
                                        else
                                        {
                                            sprintf(value, "%llu", ref);
                                        }

                                        break;
                                    }

                                    case DW_FORM_ref_udata:
                                    	strcpy(formName, "DW_FORM_ref_udata");
                                        break;

                                    case DW_FORM_indirect:
                                    	strcpy(formName, "DW_FORM_indirect");
                                        break;

                                    case DW_FORM_sec_offset:
                                    	strcpy(formName, "DW_FORM_sec_offset");
                                        break;

                                    case DW_FORM_exprloc:
                                    	strcpy(formName, "DW_FORM_exprloc");
                                        break;

                                    case DW_FORM_flag_present:
                                    	strcpy(formName, "DW_FORM_flag_present");
                                        break;

                                    case DW_FORM_strx:
                                    	strcpy(formName, "DW_FORM_strx");
                                        break;

                                    case DW_FORM_addrx:
                                    	strcpy(formName, "DW_FORM_addrx");
                                        break;

                                    case DW_FORM_ref_sup4:
                                    	strcpy(formName, "DW_FORM_ref_sup4");
                                        break;

                                    case DW_FORM_strp_sup:
                                    	strcpy(formName, "DW_FORM_strp_sup");
                                        break;

                                    case DW_FORM_data16:
                                    	strcpy(formName, "DW_FORM_data16");
                                        break;

                                    case DW_FORM_line_strp:
                                    	strcpy(formName, "DW_FORM_line_strp");
                                        break;

                                    case DW_FORM_ref_sig8:
                                    	strcpy(formName, "DW_FORM_ref_sig8");
                                        break;

                                    case DW_FORM_implicit_const:
                                    	strcpy(formName, "DW_FORM_implicit_const");
                                        break;

                                    case DW_FORM_loclistx:
                                    	strcpy(formName, "DW_FORM_loclistx");
                                        break;

                                    case DW_FORM_rnglistx:
                                    	strcpy(formName, "DW_FORM_rnglistx");
                                        break;

                                    case DW_FORM_ref_sup8:
                                    	strcpy(formName, "DW_FORM_ref_sup8");
                                        break;

                                    case DW_FORM_strx1:
                                    	strcpy(formName, "DW_FORM_strx1");
                                        break;

                                    case DW_FORM_strx2:
                                    	strcpy(formName, "DW_FORM_strx2");
                                        break;

                                    case DW_FORM_strx3:
                                    	strcpy(formName, "DW_FORM_strx3");
                                        break;

                                    case DW_FORM_strx4:
                                    	strcpy(formName, "DW_FORM_strx4");
                                        break;

                                    case DW_FORM_addrx1:
                                    	strcpy(formName, "DW_FORM_addrx1");
                                        break;

                                    case DW_FORM_addrx2:
                                    	strcpy(formName, "DW_FORM_addrx2");
                                        break;

                                    case DW_FORM_addrx3:
                                    	strcpy(formName, "DW_FORM_addrx3");
                                        break;

                                    case DW_FORM_addrx4:
                                    	strcpy(formName, "DW_FORM_addrx4");
                                        break;

                                    case DW_FORM_GNU_addr_index:
                                    	strcpy(formName, "DW_FORM_GNU_addr_index");
                                        break;

                                    case DW_FORM_GNU_str_index:
                                    	strcpy(formName, "DW_FORM_GNU_str_index");
                                        break;

                                    case DW_FORM_GNU_ref_alt:
                                    	strcpy(formName, "DW_FORM_GNU_ref_alt");
                                        break;

                                    case DW_FORM_GNU_strp_alt:
                                    	strcpy(formName, "DW_FORM_GNU_strp_alt");
                                        break;

                                    default:
                                    	sprintf(formName, "UNKNOWN (%x)", formID);
                                        break;
                                }
                            }
                        }
                    }

                    sprintf(line, "                       %-20s : %-20s (%s)\n", attribName, value, formName);
                    strcat(output, line);
                }
            }
        }



//        res = dwarf_attr(inDie, DW_AT_name, &attr_struct, &error);
//        if(res == DW_DLV_OK)
//        {
//            res = dwarf_formstring(attr_struct, &dieName, &error);
//            if(res != DW_DLV_OK)
//            {
//                logger.logError("Error in dwarf_formstring.  errno=%u %s", dwarf_errno(error),
//                        dwarf_errmsg(error));
//            }
//            else
//            {
//                sprintf(line, "            DW_AT_name        : %s", dieName);
//                strcpy(output, line);
//            }
//        }
//
//        res = dwarf_die_abbrev_children_flag(inDie, &hasChildrenFlag);
//        if(res != DW_DLV_OK)
//        {
//            logger.logError("Error in dwarf_die_abbrev_children_flag.  errno=%u %s", dwarf_errno(error),
//                    dwarf_errmsg(error));
//        }

//        else
//        {
//        	logger.logDebug("  Has children:        %s", hasChildrenFlag ? "True" : "False");
//        }
//
//        int dwarf_die_abbrev_children_flag(Dwarf_Die /*die*/,
//            Dwarf_Half * /*ab_has_child*/);
//
//        res = dwarf_attrlist(inDie, &attribs, &attribCount, &error);
//        if(res != DW_DLV_OK)
//        {
//            logger.logError("Error in dwarf_attrlist.  errno=%u %s", dwarf_errno(error),
//                    dwarf_errmsg(error));
//        }
//        else
//        {
//            if(attribCount > 0)
//            {
//            	logger.logDebug("  Attributes:");
//                for(uint32_t i = 0; i < attribCount; ++i)
//                {
//                    Dwarf_Half attrNum;
//                    res = dwarf_whatattr(attribs[i], &attrNum, &error);
//                    if(res != DW_DLV_OK)
//                    {
//                        logger.logError("Error in dwarf_whatattr.  errno=%u %s", dwarf_errno(error),
//                                dwarf_errmsg(error));
//                    }
//                    else
//                    {
//                        Dwarf_Half formID;
//
//                        switch(attrNum)
//                        {
//                            case DW_AT_sibling:
//                            	logger.logDebug("    DW_AT_sibling");
//                                break;
//
//                            case DW_AT_location:
//                            	logger.logDebug("    DW_AT_location");
//                                break;
//
//                            case DW_AT_name:
//                            	logger.logDebug("    DW_AT_name");
//                                break;
//
//                            case DW_AT_ordering:
//                            	logger.logDebug("    DW_AT_ordering");
//                                break;
//
//                            case DW_AT_subscr_data:
//                            	logger.logDebug("    DW_AT_subscr_data");
//                                break;
//
//                            case DW_AT_byte_size:
//                            	logger.logDebug("    DW_AT_byte_size");
//                                break;
//
//                            case DW_AT_decl_file:
//                            	logger.logDebug("    DW_AT_decl_file");
//                                break;
//
//                            case DW_AT_decl_line:
//                            	logger.logDebug("    DW_AT_decl_line");
//                                break;
//
//                            case DW_AT_type:
//                            	logger.logDebug("    DW_AT_type");
//                                break;
//
//                            default:
//                            	logger.logDebug("    0x%02x", attrNum);
//                                break;
//                        }
//
//                        res = dwarf_whatform(attribs[i], &attrNum, &error);
//                        if(res != DW_DLV_OK)
//                        {
//                            logger.logError("Error in dwarf_whatattr.  errno=%u %s", dwarf_errno(error),
//                                    dwarf_errmsg(error));
//                        }
//                        else
//                        {
//                            res = dwarf_whatform(attribs[i], &formID, &error);
//                            if(res != DW_DLV_OK)
//                            {
//                                logger.logError("Error in dwarf_whatform.  errno=%u %s", dwarf_errno(error),
//                                        dwarf_errmsg(error));
//                            }
//                            else
//                            {
//                                switch(formID)
//                                {
//                                    case DW_FORM_addr:
//                                    	logger.logDebug(":DW_FORM_addr");
//                                        break;
//
//                                    case DW_FORM_block2:
//                                    	logger.logDebug(":DW_FORM_block2");
//                                        break;
//
//                                    case DW_FORM_block4:
//                                    	logger.logDebug(":DW_FORM_block4");
//                                        break;
//
//                                    case DW_FORM_data1:
//                                    {
//                                        Dwarf_Unsigned udata = 0;
//                                        res = dwarf_formudata(attribs[i], &udata, &error);
//                                        if(res != DW_DLV_OK)
//                                        {
//                            				logger.logError("Error in dwarf_formudata.  line=%u  errno=%u %s", __LINE__, dwarf_errno(error),
//                                                    dwarf_errmsg(error));
//                                        }
//                                        else
//                                        {
//                                            char data = (char) udata;
//                                            logger.logDebug(":DW_FORM_data1:%u", data);
//                                        }
//                                        break;
//                                    }
//
//                                    case DW_FORM_data2:
//                                    {
//                                        Dwarf_Unsigned udata = 0;
//                                        res = dwarf_formudata(attribs[i], &udata, &error);
//                                        if(res != DW_DLV_OK)
//                                        {
//                            				logger.logError("Error in dwarf_formudata.  line=%u  errno=%u %s", __LINE__, dwarf_errno(error),
//                                                    dwarf_errmsg(error));
//                                        }
//                                        else
//                                        {
//                                            unsigned short data = (unsigned short) udata;
//                                            logger.logDebug(":DW_FORM_data2:%u", data);
//                                        }
//                                        break;
//                                    }
//
//                                    case DW_FORM_data4:
//                                    {
//                                        Dwarf_Unsigned udata = 0;
//                                        res = dwarf_formudata(attribs[i], &udata, &error);
//                                        if(res != DW_DLV_OK)
//                                        {
//                            				logger.logError("Error in dwarf_formudata.  line=%u  errno=%u %s", __LINE__, dwarf_errno(error),
//                                                    dwarf_errmsg(error));
//                                        }
//                                        else
//                                        {
//                                            unsigned int data = (unsigned int) udata;
//                                            logger.logDebug(":DW_FORM_data4:%u", data);
//                                        }
//                                        break;
//                                    }
//
//                                    case DW_FORM_data8:
//                                    {
//                                        Dwarf_Unsigned udata = 0;
//                                        res = dwarf_formudata(attribs[i], &udata, &error);
//                                        if(res != DW_DLV_OK)
//                                        {
//                            				logger.logError("Error in dwarf_formudata.  line=%u  errno=%u %s", __LINE__, dwarf_errno(error),
//                                                    dwarf_errmsg(error));
//                                        }
//                                        else
//                                        {
//                                        	logger.logDebug(":DW_FORM_data8:%llu", udata);
//                                        }
//                                        break;
//                                    }
//
//                                    case DW_FORM_string:
//                                    {
//                                        char *str = 0;
//                                        res = dwarf_formstring(attribs[i], &str, &error);
//                                        if(res != DW_DLV_OK)
//                                        {
//                                            logger.logError("Error in dwarf_formstring.  errno=%u %s", dwarf_errno(error),
//                                                    dwarf_errmsg(error));
//                                        }
//                                        else
//                                        {
//                                        	logger.logDebug(":DW_FORM_string:%s", str);
//                                        }
//                                        break;
//                                    }
//
//                                    case DW_FORM_block:
//                                    	logger.logDebug(":DW_FORM_block");
//                                        break;
//
//                                    case DW_FORM_sdata:
//                                    	logger.logDebug(":DW_FORM_sdata");
//                                        break;
//
//                                    case DW_FORM_strp:
//                                    {
//                                        char *strp = 0;
//                                        res = dwarf_formstring(attribs[i], &strp, &error);
//                                        if(res != DW_DLV_OK)
//                                        {
//                                            logger.logError("Error in dwarf_formstring.  errno=%u %s", dwarf_errno(error),
//                                                    dwarf_errmsg(error));
//                                        }
//                                        else
//                                        {
//                                            char *text = dwarfStringToChar(strp);
//                                            logger.logDebug(":DW_FORM_strp:%s", text);
//                                        }
//                                        break;
//                                    }
//
//                                    case DW_FORM_udata:
//                                    {
//                                        Dwarf_Unsigned udata = 0;
//                                        res = dwarf_formudata(attribs[i], &udata, &error);
//                                        if(res != DW_DLV_OK)
//                                        {
//                            				logger.logError("Error in dwarf_formudata.  line=%u  errno=%u %s", __LINE__, dwarf_errno(error),
//                                                    dwarf_errmsg(error));
//                                        }
//                                        else
//                                        {
//                                        	logger.logDebug(":DW_FORM_udata:%llu", udata);
//                                        }
//                                        break;
//                                    }
//
//                                    case DW_FORM_ref_addr:
//                                    	logger.logDebug(":DW_FORM_ref_addr");
//                                        break;
//
//                                    case DW_FORM_ref1:
//                                    {
//                                        Dwarf_Off ref = 0;
//                                        res = dwarf_formref(attribs[i], &ref, &error);
//                                        if(res != DW_DLV_OK)
//                                        {
//                            				logger.logError("Error in dwarf_formudata.  line=%u  errno=%u %s", __LINE__, dwarf_errno(error),
//                                                    dwarf_errmsg(error));
//                                        }
//                                        else
//                                        {
//                                            char data = (char) ref;
//                                            logger.logDebug(":DW_FORM_ref1:%u", data);
//                                        }
//                                        break;
//                                    }
//
//                                    case DW_FORM_ref2:
//                                    {
//                                        Dwarf_Off ref = 0;
//                                        res = dwarf_formref(attribs[i], &ref, &error);
//                                        if(res != DW_DLV_OK)
//                                        {
//                                            logger.logError("Error in dwarf_formref.  errno=%u %s", dwarf_errno(error),
//                                                    dwarf_errmsg(error));
//                                        }
//                                        else
//                                        {
//                                            unsigned short int data = (unsigned short int) ref;
//                                            logger.logDebug(":DW_FORM_ref2:%u", data);
//                                        }
//                                        break;
//                                    }
//
//                                    case DW_FORM_ref4:
//                                    {
//                                        Dwarf_Off ref = 0;
//                                        res = dwarf_formref(attribs[i], &ref, &error);
//                                        if(res != DW_DLV_OK)
//                                        {
//                                            logger.logError("Error in dwarf_formref.  errno=%u %s", dwarf_errno(error),
//                                                    dwarf_errmsg(error));
//                                        }
//                                        else
//                                        {
//                                            unsigned int data = (unsigned int) ref;
//                                            logger.logDebug(":DW_FORM_ref4:%u", data);
//                                        }
//                                        break;
//                                    }
//
//                                    case DW_FORM_ref8:
//                                    {
//                                        Dwarf_Off ref = 0;
//                                        res = dwarf_formref(attribs[i], &ref, &error);
//                                        if(res != DW_DLV_OK)
//                                        {
//                                            logger.logError("Error in dwarf_formref.  errno=%u %s", dwarf_errno(error),
//                                                    dwarf_errmsg(error));
//                                        }
//                                        else
//                                        {
//                                        	logger.logDebug(":DW_FORM_ref4:%llu", ref);
//                                        }
//                                        break;
//                                    }
//
//                                    case DW_FORM_ref_udata:
//                                    	logger.logDebug(":DW_FORM_ref_udata");
//                                        break;
//
//                                    case DW_FORM_indirect:
//                                    	logger.logDebug(":DW_FORM_indirect");
//                                        break;
//
//                                    case DW_FORM_sec_offset:
//                                    	logger.logDebug(":DW_FORM_sec_offset");
//                                        break;
//
//                                    case DW_FORM_exprloc:
//                                    	logger.logDebug(":DW_FORM_exprloc");
//                                        break;
//
//                                    case DW_FORM_flag_present:
//                                    	logger.logDebug(":DW_FORM_flag_present");
//                                        break;
//
//                                    case DW_FORM_ref_sig8:
//                                    	logger.logDebug(":DW_FORM_ref_sig8");
//                                        break;
//
//                                    default:
//                                    	logger.logDebug(":0x%02x", formID);
//                                        break;
//
//                                }
//                            }
//                        }
//                    }
//                }
//                logger.logDebug("\n");
//            }
//        }

        logger.logDebug(output);
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
        else
        {
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
				logger.logError("Error in dwarf_formudata.  line=%u  errno=%u %s", __LINE__, dwarf_errno(error),
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
        std::vector<Dimension> dimensionList{};

        baseTypeSymbol = getBaseTypeSymbol(elf ,inDie, dimensionList);
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
    Dwarf_Attribute attr_struct = nullptr;
    Dwarf_Die       memberDie = 0;

    Dwarf_Unsigned udata = 0;

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
        char           *memberName = nullptr;
        Symbol         *memberBaseTypeSymbol = nullptr;
        uint32_t        memberLocation = 0;

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
                        std::vector<Dimension> dimensionList{};

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
							res = dwarf_formudata(attr_struct, &udata, &error);
							if(res != DW_DLV_OK)
							{
								logger.logError("Error in dwarf_formudata , level %d.  errno=%u %s", dwarf_errno(error),
									dwarf_errmsg(error));
							}
							else
							{
								memberLocation = (uint32_t) udata;
							}
						}

                        /* Get the base type die. */
                        if(res == DW_DLV_OK)
                        {
                            memberBaseTypeSymbol = getBaseTypeSymbol(elf, memberDie, dimensionList);
                            if(memberBaseTypeSymbol == 0)
                            {
                                logger.logWarning("Couldn't find base type for %s:%s.", symbol.getName().c_str(), memberName);

                                /* Set the error code so we don't do anymore processing. */
                                res = DW_DLV_ERROR;
                            }
                        }

                        /* We have everything we need.  Add this field. */
                        if(res == DW_DLV_OK)
                        {

							std::string sMemberName = memberName;

							Field memberField{symbol, sMemberName, (uint32_t) memberLocation, *memberBaseTypeSymbol, dimensionList, elf.isLittleEndian()};

							addBitFields(memberDie, memberField);
							symbol.addField(memberField);
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
                /* We wrapped around.  We're done processing the member fields. */
            	addPaddingToStruct(symbol);
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

void Juicer::addPaddingToStruct(Symbol& symbol)
{
	uint32_t spareCount{0};

	/*Add padding between fields */
	if (symbol.getFields().size()>0 && !symbol.hasBitFields())
	{
		uint32_t fieldsSize= symbol.getFields().size();

		for(uint32_t i= 1;i<fieldsSize;i++)
		{
			/*@note I know the fields container access is ugly this way,
			 * but it is a lot safer than something like std::vector.back() */

			uint32_t previousFieldSize = symbol.getFields().at(i-1)->getType().getByteSize();

			if(symbol.getFields().at(i-1)->isArray()>0)
			{
				previousFieldSize = symbol.getFields().at(i-1)->getArraySize() * previousFieldSize ;
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
						*paddingSymbol, symbol.getElf().isLittleEndian()));

				fieldsSize++;
				i++;
				spareCount++;

				memberLocation += paddingSize;
			}
			memberLocation += memberLocationDelta;

		}

	}

	addPaddingEndToStruct(symbol);
}

/**
 *@brief Adds padding to the end of struct.
 *
 *@note At the moment, if symbol has any bitfields,
 *then this function does not attempt to add padding. Will address this issue ASAP.
 */
void Juicer::addPaddingEndToStruct(Symbol& symbol)
{

	bool hasBitFields = symbol.hasBitFields();
	std::string 		paddingFieldName{"_spare_end"};
	std::string 		paddingType{"_padding"};
	uint32_t 			symbolSize = 0;
	uint32_t 			sizeDelta = 0;

	if(!hasBitFields && symbol.getFields().size()>0)
	{
		symbolSize = symbol.getFields().back()->getByteOffset() + symbol.getFields().back()->getType().getByteSize();

		if(symbol.getFields().back()->getArraySize()>0)
		{
			symbolSize = symbol.getFields().back()->getByteOffset() + (symbol.getFields().back()->getType().getByteSize()
						 * symbol.getFields().back()->getArraySize()) ;
		}

		sizeDelta = symbol.getByteSize() - symbolSize;

		/* The sizeDelta would be the size of the padding chunk, if there is any present. capability */

		if(sizeDelta>0)
		{
			paddingType += std::to_string(sizeDelta*8);


			Symbol* paddingSymbol = symbol.getElf().getSymbol(paddingType);

			if(paddingSymbol == nullptr)
			{
				paddingSymbol = symbol.getElf().addSymbol(paddingType, sizeDelta);
			}

			uint32_t newFieldByteOffset = symbol.getFields().back()->getByteOffset() + symbol.getFields().back()->getType().getByteSize() ;

			symbol.addField(paddingFieldName,newFieldByteOffset, *paddingSymbol, symbol.getElf().isLittleEndian(), 0,0);

		}
	}
}

/**
 *@brief Checks if dataMemberDie has bitfields. And if it does, add them to dataMemberField.
 */
void Juicer::addBitFields(Dwarf_Die dataMemberDie, Field& dataMemberField)
{
    Dwarf_Attribute attr_struct = nullptr;
	int32_t res = 0;
	Dwarf_Unsigned bit_offset = 0;
	Dwarf_Unsigned bit_size = 0;

	res = dwarf_attr(dataMemberDie, DW_AT_data_bit_offset, &attr_struct, &error);


    if(DW_DLV_OK == res)
    {
		res = dwarf_formudata(attr_struct, &bit_offset, &error);
    }

    res = dwarf_attr(dataMemberDie, DW_AT_bit_size, &attr_struct, &error);

    if(DW_DLV_OK == res)
    {
		res = dwarf_formudata(attr_struct, &bit_size, &error);
		if(res != DW_DLV_OK)
		{
			dataMemberField.setBitOffset(0);
			dataMemberField.setBitSize(0);
		}
		else if(DW_DLV_OK == res)
		{


		    res = dwarf_attr(dataMemberDie, DW_AT_bit_offset, &attr_struct, &error);

		    if(DW_DLV_OK == res)
		    {
				res = dwarf_formudata(attr_struct, &bit_offset, &error);
		    }
			dataMemberField.setBitOffset(bit_offset);
			dataMemberField.setBitSize(bit_size);
		}
    }

    return;
}

/**
 *@brief Checks if the CU(Compilation Unit such as a .o or executable file) is supported by Juicer.
 *See the DWARF_VERSION macro.
 */
bool Juicer::isDWARFVersionSupported(Dwarf_Die inDie)
{
	bool isSupported = true;

	Dwarf_Half dwarfVersion = 0;

	Dwarf_Half dwarfOffset = 0;

	int rec =  dwarf_get_version_of_die(inDie, &dwarfVersion, &dwarfOffset);

	if(rec != DW_DLV_OK)
	{
		logger.logWarning("The dwarf version of this die is unknown");
	}
	else
	{
		if(dwarfVersion==DWARF_VERSION)
		{
			isSupported  = true;
		}
	}

	return isSupported;
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

    for(;;)
    {
        Dwarf_Die sib_die = 0;
        Dwarf_Half tag = 0;
        Dwarf_Off  offset = 0;

        res = dwarf_dieoffset(cur_die, &offset, &error);
        if(res != DW_DLV_OK)
        {
            logger.logError("Error in dwarf_dieoffset , level %d.  errno=%u %s", in_level, dwarf_errno(error),
            dwarf_errmsg(error));
            return_value = JUICER_ERROR;
        }

    	DisplayDie(cur_die, in_level);

        res = dwarf_tag(cur_die, &tag, &error);
        if(res != DW_DLV_OK)
        {
            logger.logError("Error in dwarf_tag , level %d.  errno=%u %s", in_level, dwarf_errno(error),
                    dwarf_errmsg(error));
            return_value = JUICER_ERROR;
        }

        if(DW_DLV_OK == res)
        {
        	bool isDwarfSupported = isDWARFVersionSupported(cur_die);

			if(isDwarfSupported == false)
			{
				logger.logWarning("This DWARF version is not supported for this die. At the moment only DWARF Version 4 is supported.");
			}
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

    if(strcmp(name,"CFE_SB_TlmHdr_t") == 0)
    {
    	printf("hello\n");
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

			if(ident_buffer[EI_DATA] == ELFDATA2MSB)
			{
				rc = JUICER_ENDIAN_BIG;
			}
			else if(ident_buffer[EI_DATA] == ELFDATA2LSB)
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

			if(ident_buffer[EI_DATA] == ELFDATA2MSB)
			{
				rc = JUICER_ENDIAN_BIG;
			}
			else if(ident_buffer[EI_DATA] == ELFDATA2LSB)
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

            elf->isLittleEndian(JUICER_ENDIAN_BIG == endianness?
                                false: true);
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
            logger.logInfo("Parsing of elf file '%s' is complete.  Writing to data container.", elfFilePath.c_str());
            return_value  = idc->write(*elf.get());
        }
    }

    return return_value;
}

uint32_t Juicer::calcArraySizeForDimension(Dwarf_Debug dbg, Dwarf_Die dieSubrangeType)
{

    Dwarf_Unsigned  dwfUpperBound = 0;
    Dwarf_Attribute attr_struct;

    int res = DW_DLV_OK;
    uint32_t dimSize = 0;
    /* Now lets get the array size.  Get the array size by getting
     * the first child, which should be the subrange_type. */

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

        /* Set the multiplicity argument. */
        if(res == DW_DLV_OK)
        {
        	dimSize += dwfUpperBound + 1;
        }
    }

    return dimSize;
}

/**
 *
 * @return The number of elements in the die array entry, including all dimensions. It is assumed that die is of type
 * DW_TAG_array_type.
 */
int Juicer::calcArraySizeForAllDims(Dwarf_Debug dbg, Dwarf_Die die)
{
    int arraySize = 0;
    std::vector<Dwarf_Die>  children = getChildrenVector(dbg, die);

    for(auto child: children)
    {
    	if (arraySize == 0)
    			arraySize = 1;

    	arraySize = arraySize * calcArraySizeForDimension(dbg, child);
    }

    return arraySize;

}

/**
 * Assuming that die is a DW_TAG_array_type, iterate through each  DW_TAG_subrange_type and return
 * a std::vector with all them as Dimension objects
 */
std::vector<Dimension> Juicer::getDimList(Dwarf_Debug dbg, Dwarf_Die die)
{
	std::vector<Dimension> dimList{};
    std::vector<Dwarf_Die>  children = getChildrenVector(dbg, die);
    for(auto child: children)
    {
    	dimList.push_back(Dimension{calcArraySizeForDimension(dbg, child) - 1});
    }

    return dimList;
}

/**
 *Very useful for counting sibling sibling with tags such as DW_TAG_subrange_type
 *to figure out the size of multidimensional arrays.
 */
int Juicer::getNumberOfSiblingsForDie(Dwarf_Debug dbg, Dwarf_Die die)
{
    int res = DW_DLV_OK;
    int siblingCount = 0;

    Dwarf_Die   sibling_die;

    res = dwarf_siblingof(dbg, die, &sibling_die, &error);

    if(res != DW_DLV_OK)
    {
        logger.logWarning("Error in dwarf_siblingof.  errno=%u %s", dwarf_errno(error),
                dwarf_errmsg(error));
    }
    else
    {
    	siblingCount = 1;
    	return siblingCount += getNumberOfSiblingsForDie(dbg, sibling_die);
    }

    return siblingCount;
}

std::vector<Dwarf_Die> Juicer::getSiblingsVector(Dwarf_Debug dbg, Dwarf_Die die)
{
    int res = DW_DLV_OK;
    std::vector<Dwarf_Die> siblingList{};

    Dwarf_Die   sibling_die;

    int siblingCount =  getNumberOfSiblingsForDie(dbg, die);

    for(int sibling =0; sibling<siblingCount; sibling++)
    {
    	res = dwarf_siblingof(dbg, die, &sibling_die, &error);
        if(res != DW_DLV_OK)
        {
            logger.logWarning("Error in dwarf_siblingof.  errno=%u %s", dwarf_errno(error),
                    dwarf_errmsg(error));
        }
        else
        {
        	siblingList.push_back(sibling_die);
        	die = sibling_die;
        }
    }

    return siblingList;
}

/**
 *@brief Get all of the children of the die in a nice STL vector.
 */
std::vector<Dwarf_Die> Juicer::getChildrenVector(Dwarf_Debug dbg, Dwarf_Die parentDie)
{
    int res = DW_DLV_OK;
    std::vector<Dwarf_Die> childList{};

    Dwarf_Die   childDie;

    int childCount = 0;

    //Get the first sibling
    res = dwarf_child(parentDie, &childDie, &error);
    if(res != DW_DLV_OK)
    {
        logger.logError("Error in dwarf_child. errno=%u %s", dwarf_errno(error),
                dwarf_errmsg(error));
    }

    if(res == DW_DLV_OK)
    {
		childList.push_back(childDie);
        childCount = getNumberOfSiblingsForDie(dbg, childDie) + 1;
    	//Get all of the siblings, including the very first one.
        Dwarf_Die siblingDie;
		for(int child =0; child<childCount; child++)
		{
			res = dwarf_siblingof(dbg, childDie, &siblingDie, &error);
			if(res != DW_DLV_OK)
			{
				logger.logWarning("Error in dwarf_siblingof.  errno=%u %s", dwarf_errno(error),
						dwarf_errmsg(error));
			}
			else
			{
				childList.push_back(siblingDie);
				childDie = siblingDie;
			}
		}
    }

    return childList;
}

void Juicer::setIDC(IDataContainer *inIdc)
{
    idc = inIdc;
}
