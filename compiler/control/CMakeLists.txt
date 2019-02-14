################################################################################
# Copyright (c) 2017, 2018 IBM Corp. and others
#
# This program and the accompanying materials are made available under
# the terms of the Eclipse Public License 2.0 which accompanies this
# distribution and is available at http://eclipse.org/legal/epl-2.0
# or the Apache License, Version 2.0 which accompanies this distribution
# and is available at https://www.apache.org/licenses/LICENSE-2.0.
#
# This Source Code may also be made available under the following Secondary
# Licenses when the conditions for such availability set forth in the
# Eclipse Public License, v. 2.0 are satisfied: GNU General Public License,
# version 2 with the GNU Classpath Exception [1] and GNU General Public
# License, version 2 with the OpenJDK Assembly Exception [2].
#
# [1] https://www.gnu.org/software/classpath/license.html
# [2] http://openjdk.java.net/legal/assembly-exception.html
#
# SPDX-License-Identifier: EPL-2.0 OR Apache-2.0 OR GPL-2.0 WITH Classpath-exception-2.0 OR LicenseRef-GPL-2.0 WITH Assembly-exception
#############################################################################

compiler_library(control
	${CMAKE_CURRENT_LIST_DIR}/OMROptions.cpp
	${CMAKE_CURRENT_LIST_DIR}/OptimizationPlan.cpp
	${CMAKE_CURRENT_LIST_DIR}/OMRRecompilation.cpp
	${CMAKE_CURRENT_LIST_DIR}/CompilationController.cpp
	${CMAKE_CURRENT_LIST_DIR}/CompileMethod.cpp
	${CMAKE_CURRENT_LIST_DIR}/OMROptionProcessors.cpp
	${CMAKE_CURRENT_LIST_DIR}/OMRCompilerOptionsManager.cpp
	${CMAKE_CURRENT_LIST_DIR}/OptionsBuilder.cpp
)

# Temporary WIP solution to run the Options Generator prior to the generated
# files being needed

set(OPTIONS_GENERATOR ${CMAKE_CURRENT_LIST_DIR}/options-gen.py)

set(OPTIONS_GENERATOR_OUTPUT
	${CMAKE_CURRENT_LIST_DIR}/Options.inc
	${CMAKE_CURRENT_LIST_DIR}/OptionTableProperties.inc
	${CMAKE_CURRENT_LIST_DIR}/OptionTableEntries.inc
	${CMAKE_CURRENT_LIST_DIR}/OptionCharMap.inc
)

add_custom_command(
	OUTPUT ${OPTIONS_GENERATOR_OUTPUT}
	COMMAND echo "running python options generator"
	COMMAND ${PYTHON_EXECUTABLE} ${OPTIONS_GENERATOR} -omroptionsdir ${CMAKE_CURRENT_LIST_DIR}
	DEPENDS ${OPTIONS_GENERATOR}
)

add_custom_target(
	generate-options ALL
	DEPENDS ${OPTIONS_GENERATOR_OUTPUT}
)
