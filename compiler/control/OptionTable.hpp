/*******************************************************************************
 * Copyright (c) 2019, 2019 IBM Corp. and others
 *
 * This program and the accompanying materials are made available under
 * the terms of the Eclipse Public License 2.0 which accompanies this
 * distribution and is available at http://eclipse.org/legal/epl-2.0
 * or the Apache License, Version 2.0 which accompanies this distribution
 * and is available at https://www.apache.org/licenses/LICENSE-2.0.
 *
 * This Source Code may also be made available under the following Secondary
 * Licenses when the conditions for such availability set forth in the
 * Eclipse Public License, v. 2.0 are satisfied: GNU General Public License,
 * version 2 with the GNU Classpath Exception [1] and GNU General Public
 * License, version 2 with the OpenJDK Assembly Exception [2].
 *
 * [1] https://www.gnu.org/software/classpath/license.html
 * [2] http://openjdk.java.net/legal/assembly-exception.html
 *
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0 OR GPL-2.0 WITH Classpath-exception-2.0 OR LicenseRef-GPL-2.0 WITH Assembly-exception
 *******************************************************************************/

#ifndef OMR_OPTION_TABLE_HPP
#define OMR_OPTION_TABLE_HPP

#include <unordered_map>

#include "env/jittypes.h"
#include "control/CompilerOptions.hpp"
#include <stdio.h>

/**
 * OPTION_MEMBER_TO_SET(x) macro used for specifying the member that is
 * used as lvalue when an option is being set
 * @param[in] x the option member name
 */
#define OPTION_MEMBER_TO_SET(x) {&TR::CompilerOptions::x}


namespace TR{


/**
 * struct OptionMapHasher. Used for hashing the option key for the option table
 * hash map, ignoring case and '=' suffix. Currently meant to handle ASCII only.
 */
typedef struct OptionMapHasher{
    size_t ascii_tolower(size_t c) const {
        if ((c >= 'A') && (c <= 'Z')) return (c - ('Z' - 'z'));

        return c;
    }

    size_t operator() (const char * key) const {
        size_t hash = 9000; size_t c;

        while (((c = ascii_tolower(*key++)) && (c != '='))){
            hash += (hash << 2) + c;
        }

        return hash;
    }

} OptionMapHasher;


/**
 * struct OptionKeyEquals used for key equality operation by the option hash map ignoring
 * case and '=' suffix.
 * Meant to handle ascii only.
 */
typedef struct OptionKeyEquals {

    int ascii_tolower(int c) const {
        if ((c >= 'A') && (c <= 'Z')) return (c - ('Z' - 'z'));
        
        return c;
    }

    int key_stricmp(const char * s1,const char * s2) const {
        char c1,c2;
        int diff;
        while (1){
            c1 = *s1++;
            c2 = *s2++;
            // prevent substring matches and handle `=` suffix in option names
            if (c1 == '\0' || c2 == '\0'){
                if ((c1 == c2) || (c1 == '=' || c2 == '=')){
                    return 0;
                }
                else {
                    return -1;
                }
            }
            
            diff = ascii_tolower(c1) - ascii_tolower(c2);

            if (diff){
                return diff;
            }
        }
        return 0;
    }

    bool operator()(const char * s1,const char* s2) const {

        if (key_stricmp(s1, s2) == 0) return true;
        
        return false;
    }

} OptionKeyEquals;

struct OptionTableItem;

/**
 * Option processing function pointer format, used as one of the members in struct OptionTableItem
 */
typedef void (* OptionProcessingFnPtr)(char * optionStr, TR::CompilerOptions * options, OptionTableItem * entry);

/**
 * Option hash table using const char* as key, struct OptionTableItem as mapped value, and custom hasher
 * and key equal objects meant to enable case-insensitive option lookup and handling of trailing '='
 */
typedef std::unordered_map<const char *, struct OptionTableItem, OptionMapHasher, OptionKeyEquals> OptionHashTable;

union OptionMemberToSet{
    bool TR::CompilerOptions::* booleanMember;
    size_t TR::CompilerOptions::* numericMember;
    //other member types...
};

/**
 * struct OptionTableItem contains information on how to handle options that may appear in the command line and/or env
 * vars. This information is mainly used by the option processing function to process an option.
 */
struct OptionTableItem {

    /**
     * Option category, a single char mapping to the following:
     * todo: could easily use an enum here
     * 
     *  ' ' ==> uncategorized
     *  'C' ==> code generation options
     *  'O' ==> optimization options
     *  'L' ==> log file options
     *  'D' ==> debugging options
     *  'M' ==> miscellaneous options
     *  'R' ==> recompilation and profiling options
     *  'I' ==> internal options (not reported by -Xjit:help) usually for experimentation or testing
     *  '0' - '9' ==> reserved for VM-specific categories
     */
    char * optionCategory;

    /**
     * help text to output for an option if requested
     */
    char * helpText;

    /**
     * Function used for processsing an option
     */
    OptionProcessingFnPtr fcn;

    /**
     * Arg 1 used by the processing function, usually the option member to set
     */
    OptionMemberToSet memberToSet;

    /**
     * Arg 2 used by the processing function, usually the value (numeric, etc) to
     * set the option member to
     */
    uintptrj_t param2;

    /**
     * boolean member determining if the option can be used as in a option set. Currently
     * all entries set this as false until option subset handling support is added.
     */
    bool isSubsettable;

};


} /* namespace TR */

#endif /* OMR_OPTION_TABLE_HPP */