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

#ifndef OMR_COMPILER_OPTIONS_HPP
#define OMR_COMPILER_OPTIONS_HPP

#include "env/TRMemory.hpp"

namespace TR
{


class CompilerOptions
   {

public:

    TR_ALLOC(TR_Memory::CompilerOptions)

    CompilerOptions() :
    #include "control/OptionInitializerList.inc"
        unknownBooleanOption(false),
        optionsProcessed(false)
        {}


    #include "control/Options.inc"
    bool unknownBooleanOption;
    bool optionsProcessed;


}; /* Class CompilerOptions */

} /* namespace TR */


/* unavailable options

,{
    "name":"TraceMulDecomposition",
    "category":"M",
    "desc":"todo",
    "option-member": "TR_TraceMulDecomposition",
    "type":"bool",
    "default":"false",
    "processing-fn":"setTrue",
    "subsettable":"no"
},{
    "name":"EnableUnneededNarrowIntConversion",
    "category":"M",
    "desc":"todo",
    "option-member": "TR_EnableUnneededNarrowIntConversion",
    "type":"bool",
    "default":"false",
    "processing-fn":"setTrue",
    "subsettable":"no"
},{
    "name":"TraceCGPreInstructionSelection",
    "category":"M",
    "desc":"todo",
    "option-member": "TR_TraceCGPreInstructionSelection",
    "type":"bool",
    "default":"false",
    "processing-fn":"setTrue",
    "subsettable":"no"
},{
    "name":"TraceCGPostInstructionSelection",
    "category":"M",
    "desc":"todo",
    "option-member": "TR_TraceCGPostInstructionSelection",
    "type":"bool",
    "default":"false",
    "processing-fn":"setTrue",
    "subsettable":"no"
},{
    "name":"TraceCGPostRegisterAssignment",
    "category":"M",
    "desc":"todo",
    "option-member": "TR_TraceCGPostRegisterAssignment",
    "type":"bool",
    "default":"false",
    "processing-fn":"setTrue",
    "subsettable":"no"
},{
    "name":"TraceCGPostBinaryEncoding",
    "category":"M",
    "desc":"todo",
    "option-member": "TR_TraceCGPostBinaryEncoding",
    "type":"bool",
    "default":"false",
    "processing-fn":"setTrue",
    "subsettable":"no"
},{
    "name":"TraceCGMixedModeDisassembly",
    "category":"M",
    "desc":"todo",
    "option-member": "TR_TraceCGMixedModeDisassembly",
    "type":"bool",
    "default":"false",
    "processing-fn":"setTrue",
    "subsettable":"no"
},{
    "name":"TraceCGBinaryCodedDecimal",
    "category":"M",
    "desc":"todo",
    "option-member": "TR_TraceCGBinaryCodedDecimal",
    "type":"bool",
    "default":"false",
    "processing-fn":"setTrue",
    "subsettable":"no"
},{
    "name":"TraceCGEvaluation",
    "category":"M",
    "desc":"todo",
    "option-member": "TR_TraceCGEvaluation",
    "type":"bool",
    "default":"false",
    "processing-fn":"setTrue",
    "subsettable":"no"
},{
    "name":"TraceEarlyStackMap",
    "category":"M",
    "desc":"todo",
    "option-member": "TR_TraceEarlyStackMap",
    "type":"bool",
    "default":"false",
    "processing-fn":"setTrue",
    "subsettable":"no"
},{
    "name":"EnumerateBlock",
    "category":"M",
    "desc":"todo",
    "option-member": "TR_EnumerateBlock",
    "type":"bool",
    "default":"false",
    "processing-fn":"setTrue",
    "subsettable":"no"
},{
    "name":"EnumerateInstruction",
    "category":"M",
    "desc":"todo",
    "option-member": "TR_EnumerateInstruction",
    "type":"bool",
    "default":"false",
    "processing-fn":"setTrue",
    "subsettable":"no"
},{
    "name":"EnumerateNode",
    "category":"M",
    "desc":"todo",
    "option-member": "TR_EnumerateNode",
    "type":"bool",
    "default":"false",
    "processing-fn":"setTrue",
    "subsettable":"no"
},{
    "name":"EnumerateRegister",
    "category":"M",
    "desc":"todo",
    "option-member": "TR_EnumerateRegister",
    "type":"bool",
    "default":"false",
    "processing-fn":"setTrue",
    "subsettable":"no"
},{
    "name":"EnumerateSymbol",
    "category":"M",
    "desc":"todo",
    "option-member": "TR_EnumerateSymbol",
    "type":"bool",
    "default":"false",
    "processing-fn":"setTrue",
    "subsettable":"no"
},{
    "name":"EnumerateStructure",
    "category":"M",
    "desc":"todo",
    "option-member": "TR_EnumerateStructure",
    "type":"bool",
    "default":"false",
    "processing-fn":"setTrue",
    "subsettable":"no"
},{
    "name":"TraceRABasic",
    "category":"M",
    "desc":"todo",
    "option-member": "TR_TraceRABasic",
    "type":"bool",
    "default":"false",
    "processing-fn":"setTrue",
    "subsettable":"no"
},{
    "name":"TraceRADependencies",
    "category":"M",
    "desc":"todo",
    "option-member": "TR_TraceRADependencies",
    "type":"bool",
    "default":"false",
    "processing-fn":"setTrue",
    "subsettable":"no"
},{
    "name":"TraceRADetails",
    "category":"M",
    "desc":"todo",
    "option-member": "TR_TraceRADetails",
    "type":"bool",
    "default":"false",
    "processing-fn":"setTrue",
    "subsettable":"no"
},{
    "name":"TraceRAPreAssignmentInstruction",
    "category":"M",
    "desc":"todo",
    "option-member": "TR_TraceRAPreAssignmentInstruction",
    "type":"bool",
    "default":"false",
    "processing-fn":"setTrue",
    "subsettable":"no"
},{
    "name":"TraceRARegisterStates",
    "category":"M",
    "desc":"todo",
    "option-member": "TR_TraceRARegisterStates",
    "type":"bool",
    "default":"false",
    "processing-fn":"setTrue",
    "subsettable":"no"
},{
    "name":"TraceRASpillTemps",
    "category":"M",
    "desc":"todo",
    "option-member": "TR_TraceRASpillTemps",
    "type":"bool",
    "default":"false",
    "processing-fn":"setTrue",
    "subsettable":"no"
},{
    "name":"TraceRAListing",
    "category":"M",
    "desc":"todo",
    "option-member": "TR_TraceRAListing",
    "type":"bool",
    "default":"false",
    "processing-fn":"setTrue",
    "subsettable":"no"
},{
    "name":"TraceGRABasic",
    "category":"M",
    "desc":"todo",
    "option-member": "TR_TraceGRABasic",
    "type":"bool",
    "default":"false",
    "processing-fn":"setTrue",
    "subsettable":"no"
},{
    "name":"TraceGRAListing",
    "category":"M",
    "desc":"todo",
    "option-member": "TR_TraceGRAListing",
    "type":"bool",
    "default":"false",
    "processing-fn":"setTrue",
    "subsettable":"no"
},{
    "name":"TraceLRAResults",
    "category":"M",
    "desc":"todo",
    "option-member": "TR_TraceLRAResults",
    "type":"bool",
    "default":"false",
    "processing-fn":"setTrue",
    "subsettable":"no"
},{
    "name":"TraceRegisterITFBasic",
    "category":"M",
    "desc":"todo",
    "option-member": "TR_TraceRegisterITFBasic",
    "type":"bool",
    "default":"false",
    "processing-fn":"setTrue",
    "subsettable":"no"
},{
    "name":"TraceRegisterITFBuild",
    "category":"M",
    "desc":"todo",
    "option-member": "TR_TraceRegisterITFBuild",
    "type":"bool",
    "default":"false",
    "processing-fn":"setTrue",
    "subsettable":"no"
},{
    "name":"TraceRegisterITFColour",
    "category":"M",
    "desc":"todo",
    "option-member": "TR_TraceRegisterITFColour",
    "type":"bool",
    "default":"false",
    "processing-fn":"setTrue",
    "subsettable":"no"
},{
    "name":"TraceSpillCostsBasic",
    "category":"M",
    "desc":"todo",
    "option-member": "TR_TraceSpillCostsBasic",
    "type":"bool",
    "default":"false",
    "processing-fn":"setTrue",
    "subsettable":"no"
},{
    "name":"TraceILDeadCodeBasic",
    "category":"M",
    "desc":"todo",
    "option-member": "TR_TraceILDeadCodeBasic",
    "type":"bool",
    "default":"false",
    "processing-fn":"setTrue",
    "subsettable":"no"
},{
    "name":"EnableGPU",
    "category":"M",
    "desc":"todo",
    "option-member": "TR_EnableGPU",
    "type":"bool",
    "default":"false",
    "processing-fn":"setTrue",
    "subsettable":"no"
},{
    "name":"EnableGPUForce",
    "category":"M",
    "desc":"todo",
    "option-member": "TR_EnableGPUForce",
    "type":"bool",
    "default":"false",
    "processing-fn":"setTrue",
    "subsettable":"no"
},{
    "name":"EnableGPUVerbose",
    "category":"M",
    "desc":"todo",
    "option-member": "TR_EnableGPUVerbose",
    "type":"bool",
    "default":"false",
    "processing-fn":"setTrue",
    "subsettable":"no"
},{
    "name":"EnableGPUDetails",
    "category":"M",
    "desc":"todo",
    "option-member": "TR_EnableGPUDetails",
    "type":"bool",
    "default":"false",
    "processing-fn":"setTrue",
    "subsettable":"no"
},{
    "name":"EnableSafeMT",
    "category":"M",
    "desc":"todo",
    "option-member": "TR_EnableSafeMT",
    "type":"bool",
    "default":"false",
    "processing-fn":"setTrue",
    "subsettable":"no"
},{
    "name":"EnableGPUEnableMath",
    "category":"M",
    "desc":"todo",
    "option-member": "TR_EnableGPUEnableMath",
    "type":"bool",
    "default":"false",
    "processing-fn":"setTrue",
    "subsettable":"no"
},{
    "name":"EnableGPUDisableTransferHoist",
    "category":"M",
    "desc":"todo",
    "option-member": "TR_EnableGPUDisableTransferHoist",
    "type":"bool",
    "default":"false",
    "processing-fn":"setTrue",
    "subsettable":"no"
},{
    "name":"TraceILRematBasic",
    "category":"M",
    "desc":"todo",
    "option-member": "TR_TraceILRematBasic",
    "type":"bool",
    "default":"false",
    "processing-fn":"setTrue",
    "subsettable":"no"
}

*/

#endif  /* OMR_COMPILER_OPTIONS_HPP */