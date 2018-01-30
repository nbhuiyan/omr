/*******************************************************************************
 * Copyright (c) 2018, 2018 IBM Corp. and others
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

#ifndef ELFGENERATOR_HPP
#define ELFGENERATOR_HPP

#if (HOST_OS == OMR_LINUX)

#include <elf.h>
#include <string>
#include "env/TypedAllocator.hpp"
#include "env/RawAllocator.hpp"
#include "codegen/StaticRelocation.hpp"
#include "runtime/CodeCacheManager.hpp"
class TR_Memory;

namespace TR{

class ELFGenerator{

public:
    ELFGenerator(TR::RawAllocator rawAllocator,
                    uint8_t const * codeStart, size_t codeSize,
                    bool isRelocatable = false);

    ~ELFGenerator() throw();

protected:

#if defined(TR_HOST_64BIT)
    typedef Elf64_Ehdr ELFHeader;
    typedef Elf64_Shdr ELFSectionHeader;
    typedef Elf64_Phdr ELFProgramHeader;
    typedef Elf64_Addr ELFAddress;
    typedef Elf64_Sym ELFSymbol;
    typedef Elf64_Rela ELFRela;
    typedef Elf64_Off ELFOffset;
#define ELF_ST_INFO(bind, type) ELF64_ST_INFO(bind, type)
#define ELFClass ELFCLASS64;
#else
    typedef Elf32_Ehdr ELFHeader;
    typedef Elf32_Shdr ELFSectionHeader;
    typedef Elf32_Phdr ELFProgramHeader;
    typedef Elf32_Addr ELFAddress;
    typedef Elf32_Sym ELFSymbol;
    typedef Elf32_Rela ELFRela;
    typedef Elf32_Off ElfOffset;
#define ELF_ST_INFO(bind, type) ELF32_ST_INFO(bind, type)
#define ELFClass ELFCLASS32;
#endif

    /**
     * 
     * 2 variants of ELF header and trailer structs
     * 
     */
    struct ELFRelocatableHeader{
        ELFHeader hdr;
    };

    struct ELFExecutableHeader{
        ELFHeader hdr;
        ELFProgramHeader phdr;
    };

    struct ELFTrailer{
        ELFSectionHeader zeroSection;
        ELFSectionHeader textSection;
        ELFSectionHeader dynsymSection;
        ELFSectionHeader shstrtabSection;
        ELFSectionHeader dynstrSection;
        
        char zeroSectionName[1];
        char shstrtabSectionName[10];
        char textSectionName[6];
        char dynsymSectionName[8];
        char dynstrSectionName[8];

        // start of a variable sized region: an ELFSymbol structure per symbol + total size of elf symbol names
        ELFSymbol symbols[1];
    };

    struct ELFRelocatableTrailer{
        ELFSectionHeader zeroSection;
        ELFSectionHeader textSection;
        ELFSectionHeader relasection;
        ELFSectionHeader dynsymSection;
        ELFSectionHeader shstrtabSection;
        ELFSectionHeader dynstrSection;

        char zeroSectionName[1];
        char shstrtabSectionName[10];
        char textSectionName[6];
        char relaSectionName[11];
        char dynsymSectionName[8];
        char dynstrSectionName[8];

        // start of a variable sized region: an ELFSymbol structure per symbol + total size of elf symbol names
        ELFSymbol symbols[1];

        // followed by variable sized symbol names located only by computed offset

        // followed by rela entries located only by computed offset
    };

    uint8_t const * const _codeStart;
    size_t const _codeSize;
    
    struct ELFRelocatableHeader *_elfRelocatableHeader;
    struct ELFRelocatableTrailer *_elfRelocatableTrailer;

    struct ELFExecutableHeader *_elfHeader;
    struct ELFTrailer * _elfTrailer;
    
    struct CodeCacheSymbol *_symbols;
    uint32_t _numSymbols;
    struct CodeCacheRelocationInfo *_relocations;
    uint32_t _numRelocations;

    uint32_t _elfTrailerSize;
    uint32_t _elfHeaderSize;
    uint32_t _totalELFSymbolNamesLength;
    uint32_t _trailerStructSize;

    bool _isRelocatable; //bool variable to indicate if the elf file to be generated is relocatable

    void initializePHeader();
    void initializeELFTrailer();
    void initializeELFHeader();
public:

    void initialize();

    void emitELF(const char * filename,
                CodeCacheSymbol *symbols, uint32_t numSymbols,
                uint32_t totalELFSymbolNamesLength,
                CodeCacheRelocationInfo *relocations = NULL,
                uint32_t numRelocations = 0);

}; /* class ELFGenerator */

} /* namespace TR */

#endif /* HOST_OS == OMR_LINUX */

#endif /* ELFGENERATOR_HPP */
