/****************************************************************************
 *
 *   Copyright (c) 2021 Windhover Labs, L.L.C. All rights reserved.
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

/*
 * DimensionList.h
 *
 *  Created on: Sep 23, 2021
 *      Author: lgomez
 *      Email:  lgomez@windhoverlabs.com
 *
 */

#ifndef DIMENSION_H_
#define DIMENSION_H_
#include <stdint.h>

#include <string>
#include <vector>

/**
 *The dimensions of an array. Modeled after the DW_TAG_array_type tag in DWARF4 standard
 *and DimensionList in XTCE. The DimensionList class keeps all
 *dimensions inside of an internal std::vector<Dimension> that is internally managed.
 *
 *Users of this class can only add Dimension objects to this class via the addDimension method;
 *they cannot remove dimensions, at least not directly. This is to guarantee the order of each Dimension object.
 *Users that need to extract data from each dimension by calling getDimensions() method
 *can assume the order has not been changed since the first time each dimension was added with addDimension.
 *Notice the vector returned from getDimensions is const; this is to prevent users of this class
 *from tampering with order of the dimensions.
 */
class DimensionList
{
    struct Dimension
    {
        Dimension(uint32_t newUpperBound) : upperBound{newUpperBound} {}

       private:
        // Inclusive. Meaning that in an array such as " flatArray int[3]" the upperBound will be 2.
        uint32_t upperBound;

       public:
        uint32_t    getUpperBound() const { return upperBound; }
        std::string toString()
        {
            std::string str{};

            str += "{";
            str += "upperBound:";
            str += std::to_string(upperBound);
            str += "}";

            return str;
        }
    };

   public:
    DimensionList();
    ~DimensionList();

    std::string                                  toString();
    void                                         addDimension(uint32_t inUpperBound);
    uint32_t                                     getId() const;
    void                                         setId(uint32_t id);
    const std::vector<DimensionList::Dimension>& getDimensions() const;

   private:
    uint32_t                              id{};
    std::vector<DimensionList::Dimension> dimensions{};
};

#endif /* DIMENSION_H_ */
