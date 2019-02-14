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

#include "control/OMRCompilerOptionsManager.hpp"
#include "control/OptionsBuilder.hpp"
#include "codegen/FrontEnd.hpp"
#include <cstdlib>
#include <cstring>
#include <stdio.h>
#include <stddef.h>
#include "control/Options.hpp"

OMR::CompilerOptionsManager * OMR::CompilerOptionsManager::_optionsManager = 0;
TR::CompilerOptions * OMR::CompilerOptionsManager::_options = 0;

TR::OptionHashTable OMR::CompilerOptionsManager::_optionTable = {

#include "control/OptionTableEntries.inc"

};


void
OMR::CompilerOptionsManager::initialize(char * cmdLineOptions){

    _optionsManager = new (PERSISTENT_NEW) OMR::CompilerOptionsManager();
    _options = new (PERSISTENT_NEW) TR::CompilerOptions();

    //OMR::OptionsBuilder::assignDefaultOptions(_options); //todo
    _optionsManager->setDefaults();

    TR::OptionsBuilder::processCmdLineOptions(_options,cmdLineOptions);

    TR::OptionsBuilder::processEnvOptions(_options);

    _optionsManager->postProcess();

    return;
}

//todo: remove or make it debug-specific
void 
OMR::CompilerOptionsManager::printOptionTable(){
    TR::OptionHashTable table = OMR::CompilerOptionsManager::_optionTable;

    printf("Table size: %lu\n", table.size());

    for (auto iter = table.begin(); iter != table.end(); iter++){
        printf("Map key: %s\n", iter->first);
    }
}

void
OMR::CompilerOptionsManager::setDefaults(){
    
    _options->TR_RestrictStaticFieldFolding = true;

#if defined(TR_HOST_ARM)
   // alignment problem for float/double
   _options->TR_DisableIntrinsics = true;
#endif

#if defined(RUBY_PROJECT_SPECIFIC)
   // Ruby has been known to spawn other Ruby VMs.  Log filenames must be unique
   // or corruption will occur.
   //
   bool forceSuffixLogs = true;
#else
#if defined(DEBUG) || defined(PROD_WITH_ASSUMES)
   bool forceSuffixLogs = false;
#else
   bool forceSuffixLogs = true;
#endif
#endif

   if (forceSuffixLogs)
      _options->TR_EnablePIDExtension = true;


   // The signature-hashing seed algorithm it the best default.
   // Unless the user specifies randomSeed=nosignature, we want to override the
   // default seed we just set above in order to improve reproducibility.
   //
   _options->TR_RandomSeedSignatureHash = true;
   _options->TR_DisableRefinedAliases = true;

   _options->TR_DisableTreePatternMatching = true;
   _options->TR_DisableHalfSlotSpills = true;

#if defined(TR_TARGET_64BIT)
   _options->TR_EnableCodeCacheConsolidation = true;
#endif

    // disable the fanin heuristics & virtual scratch memory for non-java frontends!
    _options->TR_DisableInlinerFanIn = true;
    _options->TR_DisableInlineEXTarget = true;
}

void OMR::CompilerOptionsManager::postProcess(){

#if defined(TR_HOST_ARM)
   // OSR is not available for ARM yet
   _options->TR_DisableOSR = true;
   _options->TR_EnableOSR = false;
   _options->TR_EnableOSROnGuardFailure = false;
#endif

#ifndef J9_PROJECT_SPECIFIC
   _options->TR_DisableNextGenHCR = true;
#endif

   static const char *ccr = feGetEnv("TR_DisableCCR");
   if (ccr)
      {
      _options->TR_DisableCodeCacheReclamation = true;
      }
   static const char *disableCCCF = feGetEnv("TR_DisableClearCodeCacheFullFlag");
   if (disableCCCF)
      {
      _options->TR_DisableClearCodeCacheFullFlag = true;
      }

   if (_options->TR_FullSpeedDebug)
      {
      if (!_options->TR_DisableOSR)
         _options->TR_EnableOSR = true; // Make OSR the default for FSD

      _options->TR_DisableMethodHandleThunks = true; // Can't yet transition a MH thunk frame into equivalent interpreter frames
      }

   if (_options->TR_EnableOSROnGuardFailure && !_options->TR_DisableOSR)
      _options->TR_EnableOSR = true;

   if (TR::Compiler->om.mayRequireSpineChecks())
      {
      _options->TR_DisableInternalPointers = true;
      if (TR::Options::getCmdLineOptions()->getFixedOptLevel() == -1 && _options->TR_InhibitRecompilation)
         {
         _options->TR_DisableUpgradingColdCompilations = true;
         _options->TR_DisableGuardedCountingRecompilations = true;
         _options->TR_DisableDynamicLoopTransfer = true;
         _options->TR_DisableEDO = true;
         _options->TR_DisableAggressiveRecompilations = true;
         _options->TR_EnableHardwareProfileRecompilation = false;
         }
      // If the intent was to start with warm, disable downgrades

      //if (TR::Options::getCmdLineOptions()->_initialOptLevel == warm)
      //   _options->TR_DontDowngradeToCold);

      // enable by default rampup improvements
      if (!TR::Options::getCmdLineOptions()->getOption(TR_DisableRampupImprovements))
         {
         _options->TR_EnableDowngradeOnHugeQSZ = true;
         _options->TR_EnableMultipleGCRPeriods = true;
         // enable GCR filtering


#ifdef LINUX // On Linux compilation threads can be starved
         _options->TR_EnableAppThreadYield = true;
#endif
#if defined(TR_TARGET_X86)
         // Currently GCR patching only works correctly on x86
         _options->TR_EnableGCRPatching = true;
#endif
         }
      if (_options->TR_DisableRampupImprovements)
         {
         _options->TR_DisableConservativeHotRecompilationForServerMode = true;
         }

      if (TR::Options::getCmdLineOptions()->getOption(TR_ConservativeCompilation))
         _options->TR_ConservativeCompilation = true;

      if (TR::Options::isQuickstartDetected()
#if defined(J9ZOS390)
         || sharedClassCache()  // Disable GCR for zOS if SharedClasses/AOT is used
#endif
         )
         {
         // Disable GCR in quickstart mode, but provide the option to enable it
         static char *gcr = feGetEnv("TR_EnableGuardedCountingRecompilations");
         if (!gcr)
            _options->TR_DisableGuardedCountingRecompilations = true;
         }

         // To minimize risk, don't use this feature for non-AOT cases
         // Note that information about AOT is only available in late processing stages
         _options->TR_ActivateCompThreadWhenHighPriReqIsBlocked = false;

      // If Iprofiler is disabled we will not have block frequencies so we should
      // disable the logic that makes inlining more conservative based on block frequencies
      if (_options->TR_DisableInterpreterProfiling)
         {
         _options->TR_DisableConservativeInlining = true;
         _options->TR_DisableConservativeColdInlining = true;
         }

      if (_options->TR_DisableCompilationThread)
         _options->TR_DisableNoVMAccess = true;

      // YieldVMAccess and NoVMAccess are incompatible. If the user enables YieldVMAccess
      // make sure NoVMAccess is disabled
      //
      if (_options->TR_EnableYieldVMAccess && !_options->TR_DisableNoVMAccess)
         _options->TR_DisableNoVMAccess = true;
      }

   if (_options->TR_ImmediateCountingRecompilation)
      _options->TR_EnableGCRPatching = false;

   if (_options->TR_DisableLockResevation)
      {
         _options->TR_ReservingLocks = false;
      }
#if defined(TR_HOST_S390)
   // Lock reservation without OOL has not implemented on Z
   if (_options->TR_DisableOOL)
      _options->TR_ReservingLocks = false;
#endif
}