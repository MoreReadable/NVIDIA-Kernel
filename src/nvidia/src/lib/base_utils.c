/*
 * SPDX-FileCopyrightText: Copyright (c) 1993-2020 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
 * SPDX-License-Identifier: MIT
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/*!
 * @file
 * @brief Common utility code that has no natural home
 */


#include "lib/base_utils.h"
#include "os/os.h"

//
// Log2 approximation that assumes a power of 2 number passed in.
//

NvU32 nvLogBase2(NvU64 value){
    
    NvU32 i;

    NV_ASSERT((value & (value - 1)) == 0);

    for(i = 0;i < 64;i++)
       if((1ull << i) == value)
           break;

    NV_ASSERT(i < 64);

    return i;
}


/**
 * @brief Finds the lowest unset bit of a given bitfield.
 *
 * Returns the lowest value of X such that the expression
 * pBitField[X/32] & (1<<(X%32)) is zero.
 *
 * If all bits are set, returns numElements*32.
 *
 * @param[in] pBitField
 * @param[in] numElements size of array pBitField
 *
 * @return the lowest zero bit, numElements*32 otherwise.
 */
 
NvU32 nvBitFieldLSZero(NvU32 * pBitField32,NvU32 count){
    
    NvU32 i;

    for (i = 0;i < count;++i){
        
        NvU32 temp = ~ pBitField32[i];
        
        if(temp){
            LOWESTBITIDX_32(temp);
            return temp + i * sizeof(NvU32) * 8;
        }
    }

    return count*32;
}


/**
 * @brief Finds the highest unset bit of a given bitfield.
 *
 * Returns the highest value of X such that the expression
 * pBitField[X/32] & (1<<(X%32)) is zero.
 *
 * If all bits are set, returns numElements*32.
 *
 * @param[in] pBitField
 * @param[in] numBits  must be a multiple of 32.
 *
 * @return The highest zero bit, numElements*32 otherwise.
 */

NvU32 nvBitFieldMSZero(NvU32 * pBitField32,NvU32 count){
    
    NvU32 
        j = count - 1,
        i = 0;

    while (i++ < count){
        
        NvU32 temp = ~ pBitField32[j];
        
        if(temp){
            HIGHESTBITIDX_32(temp);
            return temp + j * sizeof(NvU32) * 8;
        }
        
        j--;
    }

    return count * 32;
}


NvBool nvBitFieldTest(NvU32 * pBitField,NvU32 count,NvU32 bit){
    return (bit < count * 32)
        ? (NvBool) !!(pBitField[bit / 32] & NVBIT(bit % 32)) 
        : NV_FALSE;
}


void nvBitFieldSet(NvU32 * pBitField,NvU32 count,NvU32 bit,NvBool value){
    
    NV_ASSERT(bit < count * 32);
    
    pBitField[bit / 32] = 
        (pBitField[bit / 32] & ~NVBIT(bit % 32)) | 
        (value ? NVBIT(bit % 32) : 0);
}

//
// Sort an array of n elements/structures.
// Example:
//    NvBool integerLess(void * a, void * b)
//    {
//      return *(NvU32 *)a < *(NvU32 *)b;
//    }
//    NvU32 array[1000];
//    ...
//    NvU32 temp[1000];
//    nvMergeSort(array, arrsize(array), temp, sizeof(NvU32), integerLess);
//

#define EL(n) ((char *) array + (n) * size)

void nvMergeSort(
    void * array,
    NvU32 n,
    void * tempBuffer,
    NvU32 size,
    NvBool (*less)(void *,void *)
){
    char * mergeArray = (char *) tempBuffer;
    NvU32 m,i;

    //
    //  Bottom-up merge sort divides the sort into a sequence of passes.
    //  In each pass, the array is divided into blocks of size 'm'.
    //  Every pair of two adjacent blocks are merged (in place).
    //  The next pass is started with twice the block size
    //
    
    for(m = 1;m <= n;m *= 2){
        for (i = 0;i < (n - m);i += 2 * m){
            
            NvU32 loMin = i;
            NvU32 lo    = loMin;
            NvU32 loMax = i + m;
            NvU32 hi    = i + m;
            NvU32 hiMax = NV_MIN(n,i + 2 * m);

            char * dest = mergeArray;

            //
            // Standard merge of [lo, loMax) and [hi, hiMax)
            //
            
            while(1){
                if(less(EL(lo),EL(hi))){
                
                    portMemCopy(dest,size,EL(lo),size);
                    
                    lo++;
                    dest += size;
                    
                    if(lo >= loMax)
                        break;
                        
                } else {
                    
                    portMemCopy(dest,size,EL(hi),size);
                    
                    hi++;
                    dest += size;
                    
                    if(hi >= hiMax)
                        break;
                }
            }

            //
            // Copy remaining items (only one of these loops can run)
            //
            
            while (lo < loMax){
                
                portMemCopy(dest,size,EL(lo),size);
                
                dest += size;
                lo++;
            }

            while (hi < hiMax){
                
                portMemCopy(dest,size,EL(hi),size);
                
                dest += size;
                hi++;
            }

            //
            // Copy merged data back over array
            //
            
            portMemCopy(
                EL(loMin),
                (NvU32)(dest - mergeArray),
                mergeArray,
                (NvU32)(dest - mergeArray)
            );
        }
    }
}


#define RANGE(value,low,high) \
    (((value) >= (low)) && ((value) <= (high)))


// Do not conflict with libc naming

NvS32 nvStrToL(
    NvU8 * string,
    NvU8 ** pEndStr,
    NvS32 base,
    NvU8 stopChar,
    NvU32 * found
){
    * found = 0;

    // scan for start of number
    
    for(;* string;string++){
    
        if(RANGE(* string,'0','9')){
            * found = 1;
            break;
        }

        if((BASE16 == base) && (RANGE(* string,'a','f'))){
            * found = 1;
            break;
        }

        if((BASE16 == base) && (RANGE(* string,'A','F'))){
            * found = 1;
            break;
        }
        
        if(* string == stopChar)
            break;
    }

    // convert number
    
    NvU32 newnum;
    NvU32 num = 0;
    
    for(;* string;string++){
        
        if(RANGE(* string,'0','9')){
            newnum = * string - '0';
        } else
        if((BASE16 == base) && (RANGE(* string,'a','f'))){
            newnum = * string - 'a' + 10;
        } else
        if((BASE16 == base) && (RANGE(* string,'A','F'))){
            newnum = * string - 'A' + 10;
        } else {
            break;
        }

        num *= base;
        num += newnum;
    }

    * pEndStr = string;

    return num;
}


/**
 * @brief Returns MSB of input as a bit mask
 *
 * @param x
 * @return MSB of x
 */

NvU64 nvMsb64(NvU64 x){
    
    x |= (x >> 1);
    x |= (x >> 2);
    x |= (x >> 4);
    x |= (x >> 8);
    x |= (x >> 16);
    x |= (x >> 32);
    
    //
    // At this point, x has same MSB as input, but with all 1's below it, clear
    // everything but MSB
    //
    
    return(x & ~(x >> 1));
}


/**
 * @brief Convert unsigned long int to char*
 *
 * @param value to be converted to string
 * @param *string is the char array to be have the converted data
 * @param radix denoted the base of the operation : hex(16),octal(8)..etc
 * @return the converted string
 */
 
char * nvU32ToStr(NvU32 value,char * string,NvU32 base){
  
    char tmp[33];
    char * tp = tmp;
    NvS32 i;
    NvU32 v = value;
    char * sp;

    if(base > 36)
        return 0;
    
    if(base <= 1)
        return 0;

    while(v || tp == tmp){
        
        i = v % base;
        v = v / base;

        const char extra = (i < 10) 
            ? '0' 
            : ('a' - 10) ;

        * tp = (char)(i + extra);
            
        tp++;
    }

    sp = string;

    while(tp > tmp){
        tp--;
        * sp = * tp;
        sp++;
    }
    
    * sp = 0;

    return string;
}


/**
 * @brief Get the string length
 *
 * @param string for which length has to be calculated
 * @return the string length
 */
 
NvU32 nvStringLen(const char * str){
    
    NvU32 i = 0;
    
    while(true){
        
        if(str[i]) == '\0')
            break;
            
        i++;
    }
    
    return i;
}

