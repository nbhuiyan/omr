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

#include <cstdlib>
#include <cstring>
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include "control/OMRCompilerOptionsManager.hpp"
#include "control/OptionsBuilder.hpp"
#include "codegen/FrontEnd.hpp"
#include "infra/Assert.hpp"
#include "control/Options.hpp"

TR::CompilerOptionsManager * OMR::CompilerOptionsManager::_optionsManager = 0;
TR::CompilerOptions * OMR::CompilerOptionsManager::_options = 0;

TR::OptionTableItem OMR::CompilerOptionsManager::_optionTable[][OPTION_TABLE_MAX_BUCKET_SIZE] = {

#include "control/OptionTableEntries.inc"

};

const unsigned char OMR::CompilerOptionsManager::_hashingValues[] = {

#include "control/OptionCharMap.inc"

};

TR::CompilerOptionsManager*
OMR::CompilerOptionsManager::self(){
   return static_cast<TR::CompilerOptionsManager*>(this);
}

/*const TR::CompilerOptionsManager*
OMR::CompilerOptionsManager::self() const {
   return static_cast<const TR::CompilerOptionsManager*>(this);
}*/


void
OMR::CompilerOptionsManager::initialize(char * cmdLineOptions){

    _optionsManager = new (PERSISTENT_NEW) TR::CompilerOptionsManager();
    _options = new (PERSISTENT_NEW) TR::CompilerOptions();

    //_optionsManager->setDefaults();

    TR::OptionsBuilder::processCmdLineOptions(_options,cmdLineOptions);

    TR::OptionsBuilder::processEnvOptions(_options);

    //_optionsManager->postProcess();

    _options->optionsProcessed = true;

    return;
}

TR::OptionTableItem *
OMR::CompilerOptionsManager::getOptionTableEntry(char * optionName, int length){
   
   char c;
   char * optionNamePtr = optionName;
   int hash = 5381;
   while ((c = *optionNamePtr++)){
      hash = ((hash * 7) + _hashingValues[c]) % OPTION_TABLE_SIZE;
   }

   if (0 == _optionTable[hash][0].optionName){
      return NULL;
   } else {
      char * entryNamePtr;
      for (int i = 0; i < OPTION_TABLE_MAX_BUCKET_SIZE; i++){
         entryNamePtr = _optionTable[hash][i].optionName;
         if (0 == entryNamePtr){
            break;
         }
         optionNamePtr = optionName;
         if (length == strlen(entryNamePtr)){
            char c1, c2;
            int diff = 0;
            while ((c1 = *entryNamePtr++) && ((c2 = *optionNamePtr++))){
               diff = _hashingValues[c1] - _hashingValues[c2];
               if (diff != 0) break;
            }
            if (diff == 0) return &_optionTable[hash][i];
         }
      }
      return NULL;
   }
}

bool TR::CompilerOptions::*
OMR::CompilerOptionsManager::getMemberPtrFromOldEnum(uint32_t option){
   switch (option){
      #include "control/OptionTranslatingSwitch.inc"
      default: return &TR::CompilerOptions::unknownBooleanOption;

   return &TR::CompilerOptions::unknownBooleanOption;
   };
}

char * 
OMR::CompilerOptionsManager::getOptionNameFromOldEnum(uint32_t option){
   switch (option){
      #include "control/OptionEnumToStringSwitch.inc"
      default: return "Uknown option";
   }
}

