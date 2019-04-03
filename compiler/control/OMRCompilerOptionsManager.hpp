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


#ifndef OMR_COMPILER_OPTIONS_MANAGER_HPP
#define OMR_COMPILER_OPTIONS_MANAGER_HPP

#ifndef OMR_COMPILER_OPTIONS_MANAGER_CONNECTOR
#define OMR_COMPILER_OPTIONS_MANAGER_CONNECTOR
namespace OMR { class CompilerOptionsManager; }
namespace OMR { typedef OMR::CompilerOptionsManager CompilerOptionsManagerConnector; }
#endif

#include "control/CompilerOptions.hpp"
#include "control/OptionTable.hpp"
#include "control/OptionProcessors.hpp"
#include "env/TRMemory.hpp"
#include "infra/Annotations.hpp"
#include "control/Options.hpp"

#include "control/OptionTableProperties.inc"

namespace OMR {

class OMR_EXTENSIBLE CompilerOptionsManager{

public:
    TR_ALLOC(TR_Memory::CompilerOptionsManager)

    static TR::OptionTableItem _optionTable[][OPTION_TABLE_MAX_BUCKET_SIZE];
    static const unsigned char _hashingValues[];
    static TR::CompilerOptions         *_options;
    static OMR::CompilerOptionsManager  *_optionsManager;

    /**
     * temp
     */
    static void initialize(char * cmdLineOptions);

    static TR::CompilerOptions * getOptions(){
        return _options;
    }

    static TR::OptionTableItem * getOptionTableEntry(char * optionName, int length);

    static bool TR::CompilerOptions::* getMemberPtrFromOldEnum(TR_CompilationOptions option);

    static char * getOptionNameFromOldEnum(TR_CompilationOptions option);

private:
    void setDefaults(); //only for jitbuilder

    void postProcess(); //

}; /* class CompilerOptionsManager */

} /* namespace OMR */


#endif /* OMR_COMPILER_OPTIONS_MANAGER_HPP */
