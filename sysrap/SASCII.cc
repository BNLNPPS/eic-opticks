/*
 * Copyright (c) 2019 Opticks Team. All Rights Reserved.
 *
 * This file is part of Opticks
 * (see https://bitbucket.org/simoncblyth/opticks).
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); 
 * you may not use this file except in compliance with the License.  
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software 
 * distributed under the License is distributed on an "AS IS" BASIS, 
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  
 * See the License for the specific language governing permissions and 
 * limitations under the License.
 */


#include <iostream>
#include <iomanip>
#include <cstring>
#include <cassert>
#include <sstream>

#include "SDice.hh"

#include "SASCII.hh"


const char SASCII::UPPER[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ" ;   
const char SASCII::LOWER[] = "abcdefghijklmnopqrstuvwxyz" ;  
const char SASCII::NUMBER[] = "0123456789" ;  
const char SASCII::OTHER[] = "_" ; 
const char SASCII::EXTRA[] = "-" ; 
const char SASCII::ALLOWED[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
"abcdefghijklmnopqrstuvwxyz"
"0123456789"
"_"
"-"
 ; 



int SASCII::Index(const char* arr, int num,  char s )
{
    int idx = -1 ; 
    for(int i=0 ; i < num ; i++ ) if(arr[i] == s) idx = i ; 
    return idx ; 
}

int SASCII::LowerIndex(char s){ return Index(LOWER, sizeof(LOWER)/sizeof(char), s ) ; }
int SASCII::UpperIndex(char s){ return Index(UPPER, sizeof(UPPER)/sizeof(char), s ) ; }


char SASCII::ToUpper(char s )
{
    int idx = LowerIndex(s) ; 
    return idx == -1 ? s : UPPER[idx] ; 
}
char SASCII::ToLower(char s )
{
    int idx = UpperIndex(s) ; 
    return idx == -1 ? s : LOWER[idx] ; 
}
const char* SASCII::ToUpper( const char* str )
{
    char* s = strdup(str); 
    for(unsigned i=0 ; i < strlen(s) ; i++ ) s[i] = ToUpper(s[i] ); 
    return s ; 
}
const char* SASCII::ToLower( const char* str )
{
    char* s = strdup(str); 
    for(unsigned i=0 ; i < strlen(s) ; i++ ) s[i] = ToLower(s[i] ); 
    return s ; 
}




void SASCII::dump() const 
{
    unsigned num_upper = sizeof(UPPER)/sizeof(char) ; 
    unsigned num_lower = sizeof(LOWER)/sizeof(char) ; 
    unsigned num_number = sizeof(NUMBER)/sizeof(char) ; 
    unsigned num_other = sizeof(OTHER)/sizeof(char) ; 

    std::cout << " num_upper " << num_upper  << std::endl ; 
    std::cout << " num_lower " << num_lower  << std::endl ; 
    std::cout << " num_number " << num_number  << std::endl ; 
    std::cout << " num_other " << num_other  << std::endl ; 

    assert( num_upper == 26+1 );  // +1 from the "invisible" string terminator  
    assert( num_lower == 26+1 ); 
    assert( num_number == 10+1 ); 
    assert( num_other == 1+1 );
 
    SDice<26> rng ;  // inclusive range   
    
    for(unsigned i=0 ; i < 100 ; i++ ) 
    {
        unsigned u = rng(); 
        char c = UPPER[u] ; 
        std::cout 
            << " i " << std::setw(4) << i 
            << " u " << std::setw(4) << u 
            << " c [" << std::setw(1) << c << "]" 
            << std::endl
            ; 
    }


}


unsigned SASCII::Count( char c, const char* list )
{
    int count(0) ;  
    for(unsigned i=0 ; i < strlen(list) ; i++ )
    {
        if(list[i] == c) count++ ;   
    }
    return count ; 
}
 
bool SASCII::IsUpper( char c ){ return 1 == Count(c, UPPER) ; }
bool SASCII::IsLower( char c ){ return 1 == Count(c, LOWER) ; }
bool SASCII::IsNumber( char c ){ return 1 == Count(c, NUMBER) ; }
bool SASCII::IsOther( char c ){ return 1 == Count(c, OTHER) ; }
bool SASCII::IsExtra( char c ){ return 1 == Count(c, EXTRA) ; }
bool SASCII::IsAllowed( char c ){ return 1 == Count(c, ALLOWED) ; }


char SASCII::Classify( char c )
{
   if(IsUpper(c)) return 'U' ; 
   else if(IsLower(c)) return 'L' ;  
   else if(IsNumber(c)) return 'N' ;  
   else if(IsOther(c)) return 'O' ;  
   else if(IsExtra(c)) return 'X' ;  
   else return '?' ; 
}



void SASCII::DumpAllowed()
{
   Dump(ALLOWED) ; 
}

void SASCII::Dump(const char* s)
{
    char* p = strdup(s) ; 
    while(*p)
    {
       int i = *p ; 
       std::cout 
           << std::setw(5) << *p 
           << std::setw(5) << i 
           << std::setw(5) << Classify(*p) 
           << std::endl ; 
       p++;  
    }
}


SASCII::SASCII(const char* s_)
    :
    s(strdup(s_)),
    len(strlen(s)),
    upper(0),
    lower(0),
    number(0),
    other(0),
    extra(0),
    allowed(0),
    first_upper_index(-1),
    first_other_index(-1),
    first_extra_index(-1),
    first_number_index(-1)
{
    init(); 
}

void SASCII::init()
{
    for(unsigned i=0 ; i < len ; i++)
    {
        char c = s[i]; 
        if(IsUpper(c)) upper++ ; 
        if(IsLower(c)) lower++ ; 
        if(IsNumber(c)) number++ ; 
        if(IsOther(c)) other++ ; 
        if(IsExtra(c)) extra++ ; 
        if(IsAllowed(c)) allowed++ ; 

        if(IsUpper(c) && upper == 1) first_upper_index = int(i) ;  
        if(IsNumber(c) && number == 1) first_number_index = int(i) ;  
        if(IsOther(c)  && other == 1)  first_other_index = int(i) ;  
        if(IsExtra(c)  && extra == 1)  first_extra_index = int(i) ;  
    }     
    assert( len == allowed ); 
}


/**
SASCII::getFirst
-----------------

Return string of first n characters.

**/

std::string SASCII::getFirst(unsigned n) const 
{
    std::stringstream ss ; 
    unsigned num(0); 
    for(unsigned i=0 ; i < len ; i++)
    {
        char c = s[i]; 
        if(num < n)
        {
            ss << c ;
            num++ ;   
        } 
    }
    return ss.str();  
}


/**
SASCII::getFirstUpper
----------------------

Return string of the first n Upper characters

**/


std::string SASCII::getFirstUpper(unsigned n) const 
{
    std::stringstream ss ; 
    unsigned num(0); 
    for(unsigned i=0 ; i < len ; i++)
    {
        char c = s[i]; 
        if(IsUpper(c) && num < n)
        {
            ss << c ;
            num++ ;   
        } 
    }
    return ss.str();  
}


/**
SASCII::getFirstLast
----------------------

Return string with first and last char
**/

std::string SASCII::getFirstLast() const 
{
    std::stringstream ss ; 
    ss << s[0] ; 
    ss << s[len-1] ; 
    return ss.str();  
}

/**
SASCII::getTwoChar
---------------------

Return string with two chars picked by their indices.

**/


std::string SASCII::getTwoChar(unsigned first, unsigned second) const 
{
    std::stringstream ss ; 
    for(unsigned i=0 ; i < len ; i++)
    {
        char c = s[i]; 
        if( i == first ) ss << c ;
        if( i == second ) ss << c ; 
    }
    return ss.str();  
}


std::string SASCII::getTwoRandom(SDice<26>& rng ) const 
{
    std::stringstream ss ; 
    unsigned u0 = rng(); 
    unsigned u1 = rng(); 
    ss << UPPER[u0] << UPPER[u1] ; 
    return ss.str() ; 
}



