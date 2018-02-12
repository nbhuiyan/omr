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

#if defined(LINUX)

#include <elf.h>
#include <string>
#include "env/TypedAllocator.hpp"
#include "env/RawAllocator.hpp"
#include "codegen/StaticRelocation.hpp"
#include "runtime/CodeCacheManager.hpp"

class TR_Memory;

namespace TR{

/**
 * ELFGenerator Abstract Base Class. This abstract base class provides
 * a way to share the commonalities between the building of different
 * kinds of ELF objet files
*/
class ELFGenerator{
public:
    /**
     * ELFGenerator constructor
     * @param[in] rawAllocator the TR::RawAllocator
    */
    ELFGenerator(TR::RawAllocator rawAllocator):
                        _rawAllocator(rawAllocator)
                        {
                        }
    
    /**
     * ELFGenerator destructor
    */
    ~ELFGenerator() throw(){}

protected:

#if defined(TR_TARGET_64BIT)
    typedef Elf64_Ehdr ELFEHeader;
    typedef Elf64_Shdr ELFSectionHeader;
    typedef Elf64_Phdr ELFProgramHeader;
    typedef Elf64_Addr ELFAddress;
    typedef Elf64_Sym  ELFSymbol;
    typedef Elf64_Rela ELFRela;
    typedef Elf64_Off  ELFOffset;
#define ELF_ST_INFO(bind, type) ELF64_ST_INFO(bind, type)
#define ELFClass ELFCLASS64;
#else
    typedef Elf32_Ehdr ELFEHeader;
    typedef Elf32_Shdr ELFSectionHeader;
    typedef Elf32_Phdr ELFProgramHeader;
    typedef Elf32_Addr ELFAddress;
    typedef Elf32_Sym  ELFSymbol;
    typedef Elf32_Rela ELFRela;
    typedef Elf32_Off  ELFOffset;
#define ELF_ST_INFO(bind, type) ELF32_ST_INFO(bind, type)
#define ELFClass ELFCLASS32;
#endif

    /**
     * Pure virtual function that should be implemented to handle how the header is set up
     * for different types of ELF objects
    */
    virtual void initialize(void) = 0;

    /**
     * Pure virtual function that should be implemented to initialize header members appropriately
    */
    virtual void initializeELFHeader(void) = 0;

    /**
     * Sets up arch and OS specific information, along with ELFEHeader's e_ident member
     * @param[in] hdr the ptr to ELFEHeader (aka 'ELF64_Ehdr' or 'ELF32_Ehdr') to be initialized for platform
    */
    void initializeELFHeaderForPlatform(ELFEHeader *hdr);
    
    /**
     * This pure virtual method should be implemented by the derived classes to
     * correctly set up the trailer section and fill the CodeCacheSymbols to write
     * (plus other optional info such as CodeCacheRelocationInfo).
    */
    virtual void initializeELFTrailer(void) = 0;

    /**
     * Set up the trailer zero section
     * @param[in] shdr the ptr to ELFSectionHeader
    */
    void initializeELFTrailerZeroSection(ELFSectionHeader *shdr);
    
    /**
     * Set up the trailer text section
     * @param[in] shdr the ptr to ELFSectionHeader
     * @param[in] shName the section header name
     * @param[in] shAddress the section header address
     * @param[in] shOffset the section header offset
     * @param[in] shSize the section header size
    */
    void initializeELFTrailerTextSection(
                                            ELFSectionHeader *shdr,
                                            uint32_t shName, 
                                            ELFAddress shAddress,
                                            ELFOffset shOffset, 
                                            uint32_t shSize
                                        );
    
    /**
     * Set up the trailer text section
     * @param[in] shdr the ptr to ELFSectionHeader
     * @param[in] shName the section header name
     * @param[in] shOffset the section header offset
     * @param[in] shSize the section header size
     * @param[in] shLink the section header link
    */
    void initializeELfTrailerDynSymSection(
                                            ELFSectionHeader *shdr,
                                            uint32_t shName, 
                                            ELFOffset shOffset, 
                                            uint32_t shSize, 
                                            uint32_t shLink
                                        );

    /**
     * Set up the trailer text section
     * @param[in] shdr the ptr to ELFSectionHeader
     * @param[in] shName the section header name
     * @param[in] shOffset the section header offset
     * @param[in] shSize the section header size
    */
    void initializeELFTrailerStrTabSection(
                                            ELFSectionHeader *shdr, 
                                            uint32_t shName, 
                                            ELFOffset shOffset, 
                                            uint32_t shSize
                                        );

    /**
     * Set up the trailer text section
     * @param[in] shdr the ptr to ELFSectionHeader
     * @param[in] shName the section header name
     * @param[in] shOffset the section header offset
     * @param[in] shSize the section header size
    */
    void initializeELFTrailerDynStrSection(
                                            ELFSectionHeader *shdr, 
                                            uint32_t shName, 
                                            ELFOffset shOffset, 
                                            uint32_t shSize
                                        );

    TR::RawAllocator _rawAllocator; /**< the RawAllocator passed to the constructor */

}; //class ELFGenerator


/**
 * ELFExecutableGenerator class. Used for generating executable ELF objects
*/
class ELFExecutableGenerator : public ELFGenerator{
public:

    /**
     * ELFExecutableGenerator constructor
     * @param[in] rawAllocator 
     * @param[in] codeStart the codeStart is the base of the code segment
     * @param[in] codeSize the size of the region of the code segment
    */
    ELFExecutableGenerator(TR::RawAllocator rawAllocator,
                            uint8_t const * codeStart, size_t codeSize);

    /**
     * ELFExecutableGenerator destructor
    */
    ~ELFExecutableGenerator() throw(){}

private:
    struct ELFHeader{
        ELFEHeader hdr;
        ELFProgramHeader phdr;
    };

    /**
     * This structure lays out how the sections in the trailer will be written.
     * This will soon be changed such that the sections are set up independently.
    */
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
    
    
    uint32_t _elfTrailerSize;               /**< Size of the region of the ELF file that would be occupied by the trailer segment */
    uint32_t _elfHeaderSize;                /**< Size of the region of the ELF file that would be occupied by the header segment */
    uint32_t _totalELFSymbolNamesLength;    /**< Total length of the symbol names that would be written to file  */
    uint32_t _trailerStructSize;            /**< size of the trailer struct used for containing the ELF tailer layout in memory */
    struct TR::CodeCacheSymbol *_symbols;   /**< Chain of CodeCacheSymbol structures to be written */
    uint32_t _numSymbols;                   /**< Number of symbols to be written */

    uint8_t const * const _codeStart;       /**< base of code segment */
    uint32_t _codeSize;                     /**< Size of the code segment */
    struct ELFHeader *_elfHeader;           /**< Struct containing the ELF Header layout to be written in file */
    struct ELFTrailer * _elfTrailer;        /**< Struct containing the ELF Trailer layout to be written in file */

protected:
    /**
     * Initializes header for executable ELF and calls helper methods
    */
    virtual void initialize(void);
    
    /**
     * Initializes ELF header struct members
    */
    virtual void initializeELFHeader(void);

    /**
     * Initializes ELF Program Header, required for executable ELF
    */
    virtual void initializePHdr(void);
    
    virtual void initializeELFTrailer(void);

public:

    /**
     * This function is called when it is time to write symbols to file. This function
     * initializes the ELF Trailer and then writes to file.
    */
    void emitELF(const char * filename,
                CodeCacheSymbol *symbols, uint32_t numSymbols,
                uint32_t totalELFSymbolNamesLength);

}; //class ELFExecutableGenerator


class ELFRelocatableGenerator : public ELFGenerator{
public:
    ELFRelocatableGenerator(TR::RawAllocator rawAllocator,
                            uint8_t const * codeStart, size_t codeSize);

    ~ELFRelocatableGenerator() throw(){}
private:
    struct ELFHeader{
        ELFEHeader hdr;
    };
    
    /**
     * This structure lays out how the sections in the trailer will be written.
     * This will soon be changed such that the sections are set up independently
     * to allow better code sharing
    */
    struct ELFTrailer{
        ELFSectionHeader zeroSection;
        ELFSectionHeader textSection;
        ELFSectionHeader relaSection;
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

    uint32_t _elfTrailerSize;               /**< Size of the region of the ELF file that would be occupied by the trailer segment */
    uint32_t _elfHeaderSize;                /**< Size of the region of the ELF file that would be occupied by the header segment */
    uint32_t _totalELFSymbolNamesLength;    /**< Total length of the symbol names that would be written to file  */
    uint32_t _trailerStructSize;            /**< size of the trailer struct used for containing the ELF tailer layout in memory */
    struct TR::CodeCacheSymbol *_symbols;   /**< Chain of CodeCacheSymbol structures to be written */
    uint32_t _numSymbols;                   /**< Number of symbols to be written */
    uint8_t const * const _codeStart;       /**< base of code segment */
    uint32_t _codeSize;                     /**< Size of the code segment */

    struct ELFHeader *_elfHeader;           /**< Struct containing the ELF Header layout to be written in file */
    struct ELFTrailer * _elfTrailer;        /**< Struct containing the ELF Trailer layout to be written in file */

    struct TR::CodeCacheRelocationInfo *_relocations; /**< Struct containing relocation info to be written */
    uint32_t _numRelocations;                         /**< Number of relocations to write to file */

protected:

    /**
     * Initializes header for relocatable ELF file
    */
    virtual void initialize(void);
    
    /**
     * Initializes header struct members for relocatable ELF
    */
    virtual void initializeELFHeader(void);

    /**
     * Initializes ELF Trailer struct members, with calls to helper methods
     * implemented by the parent class and then lays out the
     * symbols to be written in memory
    */
    virtual void initializeELFTrailer(void);

public:

    /**
     * This function is called when it is time to write symbols to file. This function
     * initializes the ELF Trailer and then writes to file.
    */
    void emitELF(const char * filename,
                TR::CodeCacheSymbol *symbols, uint32_t numSymbols,
                uint32_t totalELFSymbolNamesLength,
                TR::CodeCacheRelocationInfo *relocations,
                uint32_t numRelocations);



}; //class ELFRelocatableGenerator

} //namespace TR

#endif //LINUX

#endif //ifndef ELFGENERATOR_HPP