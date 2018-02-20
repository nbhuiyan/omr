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
#include "codegen/ELFGenerator.hpp"

#if defined(LINUX)

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <elf.h>
#include "env/CompilerEnv.hpp"
#include "control/Options.hpp"
#include "control/Options_inlines.hpp"
#include "runtime/CodeCacheManager.hpp"


TR::ELFExecutableGenerator::ELFExecutableGenerator(TR::RawAllocator rawAllocator,
                            uint8_t const * codeStart, size_t codeSize):
                            ELFGenerator(rawAllocator, codeStart, codeSize)
                            {
                                initialize();
                            }

TR::ELFRelocatableGenerator::ELFRelocatableGenerator(TR::RawAllocator rawAllocator,
                            uint8_t const * codeStart, size_t codeSize):
                            ELFGenerator(rawAllocator, codeStart, codeSize)
                            {
                                initialize();
                            }

void
TR::ELFGenerator::initializeELFHeaderForPlatform(void){
    _header->e_ident[EI_MAG0] = ELFMAG0;
    _header->e_ident[EI_MAG1] = ELFMAG1;
    _header->e_ident[EI_MAG2] = ELFMAG2;
    _header->e_ident[EI_MAG3] = ELFMAG3;
    _header->e_ident[EI_CLASS] = ELFClass;
    _header->e_ident[EI_VERSION] = EV_CURRENT;
    _header->e_ident[EI_ABIVERSION] = 0;
    _header->e_ident[EI_DATA] = TR::Compiler->target.cpu.isLittleEndian() ? ELFDATA2LSB : ELFDATA2MSB;
    
    for (auto b = EI_PAD;b < EI_NIDENT;b++)
        _header->e_ident[b] = 0;

    #if defined(LINUX)
        _header->e_ident[EI_OSABI] = ELFOSABI_LINUX;
    #elif defined(AIXPPC)
        _header->e_ident[EI_OSABI] = ELFOSABI_AIX;
    #else
        TR_ASSERT(0, "unrecognized operating system: ELF header cannot be initialized");
    #endif
    
    #if defined(TR_TARGET_X86)
        _header->e_machine = EM_386;
    #elif defined(TR_TARGET_POWER)
        #if defined(TR_TARGET_64BIT)
            _header->e_machine = EM_PPC64;
        #else
            _header->e_machine = EM_PPC;
        #endif
    #elif (TR_TARGET_S390)
        _header->e_machine = EM_S390;
    #else
        TR_ASSERT(0, "unrecognized architecture: ELF header cannot be initialized");
    #endif

    _header->e_version = EV_CURRENT;
    _header->e_flags = 0; //processor-specific flags associatiated with the file
    _header->e_ehsize = sizeof(ELFEHeader);
    _header->e_shentsize = sizeof(ELFSectionHeader);
}

void 
TR::ELFGenerator::initializeZeroSection(){
    ELFSectionHeader * shdr = static_cast<ELFSectionHeader *>(_rawAllocator.allocate(sizeof(ELFSectionHeader)));
    
    shdr->sh_name = 0;
    shdr->sh_type = 0;
    shdr->sh_flags = 0;
    shdr->sh_addr = 0;
    shdr->sh_offset = 0;
    shdr->sh_size = 0;
    shdr->sh_link = 0;
    shdr->sh_info = 0;
    shdr->sh_addralign = 0;
    shdr->sh_entsize = 0;

    _zeroSection = shdr;
    _zeroSectionName[0] = 0;
}
    
void 
TR::ELFGenerator::initializeTextSection(uint32_t shName, ELFAddress shAddress,
                                                 ELFOffset shOffset, uint32_t shSize){

    ELFSectionHeader * shdr = static_cast<ELFSectionHeader *>(_rawAllocator.allocate(sizeof(ELFSectionHeader)));
    
    shdr->sh_name = shName;
    shdr->sh_type = SHT_PROGBITS;
    shdr->sh_flags = SHF_ALLOC | SHF_EXECINSTR;
    shdr->sh_addr = shAddress;
    shdr->sh_offset = shOffset;
    shdr->sh_size = shSize;
    shdr->sh_link = 0;
    shdr->sh_info = 0;
    shdr->sh_addralign = 32;
    shdr->sh_entsize = 0;

    _textSection = shdr;
    strcpy(_textSectionName, ".text");
}
    
void
TR::ELFGenerator::initializeDynSymSection(uint32_t shName, ELFOffset shOffset, uint32_t shSize, uint32_t shLink){
    
    ELFSectionHeader * shdr = static_cast<ELFSectionHeader *>(_rawAllocator.allocate(sizeof(ELFSectionHeader)));

    shdr->sh_name = shName;
    shdr->sh_type = SHT_SYMTAB; // SHT_DYNSYM
    shdr->sh_flags = 0; //SHF_ALLOC;
    shdr->sh_addr = 0; //(ELFAddress) &((uint8_t *)_elfHeader + symbolStartOffset); // fake address because not continuous
    shdr->sh_offset = shOffset;
    shdr->sh_size = shSize;
    shdr->sh_link = shLink; // dynamic string table index
    shdr->sh_info = 1; // index of first non-local symbol: for now all symbols are global
    shdr->sh_addralign = 8;
    shdr->sh_entsize = sizeof(ELFSymbol);

    _dynSymSection = shdr;
    strcpy(_dynSymSectionName, ".symtab");
}

void
TR::ELFGenerator::initializeStrTabSection(uint32_t shName, ELFOffset shOffset, uint32_t shSize){
    
    ELFSectionHeader * shdr = static_cast<ELFSectionHeader *>(_rawAllocator.allocate(sizeof(ELFSectionHeader)));
    
    shdr->sh_name = shName;
    shdr->sh_type = SHT_STRTAB;
    shdr->sh_flags = 0;
    shdr->sh_addr = 0;
    shdr->sh_offset = shOffset;
    shdr->sh_size = shSize;
    shdr->sh_link = 0;
    shdr->sh_info = 0;
    shdr->sh_addralign = 1;
    shdr->sh_entsize = 0;

    _shStrTabSection = shdr;
    strcpy(_shStrTabSectionName, ".shstrtab");
}

void
TR::ELFGenerator::initializeDynStrSection(uint32_t shName, ELFOffset shOffset, uint32_t shSize){

    ELFSectionHeader * shdr = static_cast<ELFSectionHeader *>(_rawAllocator.allocate(sizeof(ELFSectionHeader)));

    shdr->sh_name = shName;
    shdr->sh_type = SHT_STRTAB;
    shdr->sh_flags = 0;
    shdr->sh_addr = 0;
    shdr->sh_offset = shOffset;
    shdr->sh_size = shSize;
    shdr->sh_link = 0;
    shdr->sh_info = 0;
    shdr->sh_addralign = 1;
    shdr->sh_entsize = 0;

    _dynStrSection = shdr;
    strcpy(_dynStrSectionName, ".symtab");
}

void
TR::ELFGenerator::initializeRelaSection(uint32_t shName, ELFOffset shOffset, uint32_t shSize){

    ELFSectionHeader * shdr = static_cast<ELFSectionHeader *>(_rawAllocator.allocate(sizeof(ELFSectionHeader)));

    shdr->sh_name = shName;
    shdr->sh_type = SHT_RELA;
    shdr->sh_flags = 0;
    shdr->sh_addr = 0;
    shdr->sh_offset = shOffset;
    shdr->sh_size = shSize;
    shdr->sh_link = 3; // dynsymSection index in the elf file
    shdr->sh_info = 1;
    shdr->sh_addralign = 8;
    shdr->sh_entsize = sizeof(ELFRela);

    _relaSection = shdr;
    strcpy(_relaSectionName, ".rela.text");

}

void
TR::ELFGenerator::buildELFFile(::FILE *elfFile){
    
    writeHeaderToFile(elfFile);
    if(_programHeader){
        writeProgramHeaderToFile(elfFile);
    }
    writeCodeSegmentToFile(elfFile);

    writeSectionHeaderToFile(elfFile, _zeroSection);
    writeSectionHeaderToFile(elfFile, _textSection);
    if(_relocations){
        writeSectionHeaderToFile(elfFile, _relaSection);
    }
    writeSectionHeaderToFile(elfFile, _dynSymSection);
    writeSectionHeaderToFile(elfFile, _shStrTabSection);
    writeSectionHeaderToFile(elfFile, _dynStrSection);
    
    writeSectionNameToFile(elfFile, _zeroSectionName, sizeof(_zeroSectionName));
    writeSectionNameToFile(elfFile, _textSectionName, sizeof(_textSectionName));
    if(_relocations){
        writeSectionNameToFile(elfFile, _relaSectionName, sizeof(_relaSectionName));
    }
    writeSectionNameToFile(elfFile, _dynSymSectionName, sizeof(_dynSymSectionName));
    writeSectionNameToFile(elfFile, _shStrTabSectionName, sizeof(_shStrTabSectionName));
    writeSectionNameToFile(elfFile, _dynStrSectionName, sizeof(_dynStrSectionName));
    
    writeELFSymbolsToFile(elfFile);
    if(_relocations){
        writeRelaEntriesToFile(elfFile);
    }
}


void
TR::ELFGenerator::writeHeaderToFile(::FILE *fp){
    fwrite(_header, sizeof(uint8_t), sizeof(ELFEHeader), fp);
}

void 
TR::ELFGenerator::writeProgramHeaderToFile(::FILE *fp){
    fwrite(_programHeader, sizeof(uint8_t), sizeof(ELFProgramHeader), fp);
}

void 
TR::ELFGenerator::writeSectionHeaderToFile(::FILE *fp, ELFSectionHeader *shdr){
    fwrite(shdr, sizeof(uint8_t), sizeof(ELFSectionHeader), fp);
}

void 
TR::ELFGenerator::writeSectionNameToFile(::FILE *fp, char * name, uint32_t size){
    fwrite(name, sizeof(uint8_t), size, fp);
}

void 
TR::ELFGenerator::writeCodeSegmentToFile(::FILE *fp){
    fwrite(_codeStart, sizeof(uint8_t), _codeSize, fp);
}

void 
TR::ELFGenerator::writeELFSymbolsToFile(::FILE *fp){
    fwrite(_ELFSymbols, sizeof(uint8_t), sizeof(ELFSymbol) * _numSymbols, fp);
}

void 
TR::ELFGenerator::writeRelaEntriesToFile(::FILE *fp){
    fwrite(_ELFRela, sizeof(uint8_t), sizeof(ELFRela) * _numRelocations, fp);
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void
TR::ELFExecutableGenerator::initialize(void){
    ELFEHeader *hdr =
        static_cast<ELFEHeader *>(_rawAllocator.allocate(sizeof(ELFEHeader),
        std::nothrow));
    ELFProgramHeader *phdr =
        static_cast<ELFProgramHeader *>(_rawAllocator.allocate(sizeof(ELFProgramHeader),
        std::nothrow));
    _header = hdr;
    _programHeader = phdr;

    initializeELFHeader();
    initializeELFHeaderForPlatform();
    initializePHdr();
}

void
TR::ELFExecutableGenerator::initializeELFHeader(void){
    _header->e_type = ET_EXEC;
    _header->e_entry = (ELFAddress) _codeStart; //virtual address to which the system first transfers control
    _header->e_phoff = sizeof(ELFEHeader); //program header offset
    _header->e_shoff = sizeof(ELFEHeader) + sizeof(ELFProgramHeader) + _codeSize; //section header offset
    _header->e_phentsize = sizeof(ELFProgramHeader);
    _header->e_phnum = 1; // number of ELFProgramHeaders
    _header->e_shnum = 5; // number of sections in trailer
    _header->e_shstrndx = 3; // index of section header string table section in trailer
}

void
TR::ELFExecutableGenerator::initializePHdr(void){
    
    _programHeader->p_type = PT_LOAD; //should be loaded in memory
    _programHeader->p_offset = sizeof(ELFEHeader) + sizeof(ELFProgramHeader); //offset from the first byte of file to be loaded
    _programHeader->p_vaddr = (ELFAddress) _codeStart; //virtual address to load into
    _programHeader->p_paddr = (ELFAddress) _codeStart; //physical address to load into
    _programHeader->p_filesz = _codeSize; //in-file size
    _programHeader->p_memsz = _codeSize; //in-memory size
    _programHeader->p_flags = PF_X | PF_R; // should add PF_W if we get around to loading patchable code
    _programHeader->p_align = 0x1000;
}

void
TR::ELFExecutableGenerator::buildELFTrailer(void){
    uint32_t shStrTabNameLength = sizeof(_zeroSectionName) +
                            sizeof(_shStrTabSectionName) +
                            sizeof(_textSectionName) +
                            sizeof(_dynSymSectionName) +
                            sizeof(_dynStrSectionName);
    _elfTrailerSize = sizeof(ELFSectionHeader) * /* number of section headers */ 5 +
                      shStrTabNameLength +
                     _numSymbols * sizeof(ELFSymbol) + // NOTE: ELFCodeCacheTrailer includes 1 ELFSymbol: UNDEF
                     _totalELFSymbolNamesLength;
    
    /* offset calculations */
    uint32_t trailerStartOffset = sizeof(ELFEHeader) + sizeof(ELFProgramHeader) + _codeSize;
    uint32_t symbolsStartOffset = trailerStartOffset + sizeof(ELFSectionHeader) * /* Number of section headers */ 5;
    uint32_t symbolNamesStartOffset = symbolsStartOffset + (_numSymbols+1) * sizeof(ELFSymbol);
    uint32_t shNameOffset = 0;

    initializeZeroSection();
    shNameOffset += sizeof(_zeroSectionName);

    initializeTextSection(shNameOffset,
                                   (ELFAddress) _codeStart, sizeof(ELFEHeader) + sizeof(ELFProgramHeader), 
                                   _codeSize);
    shNameOffset += sizeof(_textSectionName);

    initializeDynSymSection(shNameOffset,
                            symbolsStartOffset, 
                            symbolNamesStartOffset - symbolsStartOffset,
                            /* Index of dynStrTab */ 4);
    shNameOffset += sizeof(_dynSymSectionName);
    
    initializeStrTabSection(shNameOffset, symbolsStartOffset , shStrTabNameLength);
    shNameOffset += sizeof(_shStrTabSectionName);

    initializeDynStrSection(shNameOffset, symbolNamesStartOffset, _totalELFSymbolNamesLength);
    shNameOffset += sizeof(_dynStrSectionName);

    ELFSymbol *elfSymbols = _ELFSymbols + 0;
    char *elfSymbolNames = (char *) (elfSymbols + (_numSymbols+1)); //+1 for the UNDEF symbol

    // first symbol is UNDEF symbol: all zeros, even name is zero-terminated empty string
    elfSymbolNames[0] = 0;
    elfSymbols[0].st_name = 0;
    elfSymbols[0].st_info = ELF_ST_INFO(0,0);
    elfSymbols[0].st_other = 0;
    elfSymbols[0].st_shndx = 0;
    elfSymbols[0].st_value = 0;
    elfSymbols[0].st_size = 0;

    TR::CodeCacheSymbol *sym = _symbols;
    ELFSymbol *elfSym = elfSymbols + 1;
    char *names = elfSymbolNames + 1;

    while (sym)
      {
        memcpy(names, sym->_name, sym->_nameLength);

        elfSym->st_name = names - elfSymbolNames;
        elfSym->st_info = ELF_ST_INFO(STB_GLOBAL,STT_FUNC);
        elfSym->st_other = ELF64_ST_VISIBILITY(STV_DEFAULT);
        elfSym->st_shndx = sym->_start ? 1 : SHN_UNDEF;
        elfSym->st_value = sym->_start ? static_cast<ELFAddress>(sym->_start - _codeStart) : 0;
        elfSym->st_size = sym->_size;
        names += sym->_nameLength;
        elfSym++;

        sym = sym->_next;
    }
}

void
TR::ELFExecutableGenerator::emitELF(const char * filename,
                CodeCacheSymbol *symbols, uint32_t numSymbols,
                uint32_t totalELFSymbolNamesLength){
    //init trailer and write to file
    _symbols = symbols;
    _numSymbols = numSymbols;
    _totalELFSymbolNamesLength = totalELFSymbolNamesLength;
    
    buildELFTrailer();

    ::FILE *elfFile = fopen(filename, "wb");
    
    buildELFFile(elfFile);

    fclose(elfFile);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void
TR::ELFRelocatableGenerator::initialize(void){
    ELFEHeader *hdr =
        static_cast<ELFEHeader *>(_rawAllocator.allocate(sizeof(ELFEHeader),
        std::nothrow));
    _header = hdr;
    initializeELFHeader();
    initializeELFHeaderForPlatform();
}

void
TR::ELFRelocatableGenerator::initializeELFHeader(void){
    _header->e_type = ET_REL;           
    _header->e_entry = 0; //no associated entry point for relocatable ELF files
    _header->e_phoff = 0; //no program header for relocatable files
    _header->e_shoff = sizeof(ELFEHeader) + _codeSize; //start of the section header table in bytes from the first byte of the ELF file
    _header->e_phentsize = 0; //no program headers in relocatable elf
    _header->e_phnum = 0;
    _header->e_shnum = 6;
    _header->e_shstrndx = 4; //index of section header string table
}

void
TR::ELFRelocatableGenerator::buildELFTrailer(void){
    uint32_t shStrTabNameLength = sizeof(_zeroSectionName) +
                            sizeof(_shStrTabSectionName) +
                            sizeof(_textSectionName) +
                            sizeof(_relaSectionName) +
                            sizeof(_dynSymSectionName) +
                            sizeof(_dynStrSectionName);
    _elfTrailerSize = sizeof(ELFSectionHeader) * 6 + //6 section headers in relocatable elf
                    shStrTabNameLength +
                     _numSymbols * sizeof(ELFSymbol) + // NOTE: ELFCodeCacheTrailer includes 1 ELFSymbol: UNDEF
                     _totalELFSymbolNamesLength +
                     _numRelocations * sizeof(ELFRela);
    

    /* offset calculations */
    uint32_t trailerStartOffset = sizeof(ELFEHeader) + _codeSize;
    uint32_t symbolsStartOffset = trailerStartOffset + + sizeof(ELFSectionHeader) * /* Number of section headers */ 6;
    uint32_t symbolNamesStartOffset = symbolsStartOffset + (_numSymbols+1) * sizeof(ELFSymbol);
    uint32_t relaStartOffset = symbolNamesStartOffset + _totalELFSymbolNamesLength;
    uint32_t shNameOffset = 0;

    initializeZeroSection();
    shNameOffset += sizeof(_zeroSectionName);

    initializeTextSection(          shNameOffset,
                                    /*sh_addr*/ 0,
                                    sizeof(ELFEHeader),
                                    _codeSize);
    shNameOffset += sizeof(_textSectionName);

    initializeRelaSection(shNameOffset,
                          relaStartOffset, 
                          _numRelocations * sizeof(ELFRela));
    shNameOffset += sizeof(_relaSectionName);

    initializeDynSymSection(shNameOffset, 
                            symbolsStartOffset,
                            symbolNamesStartOffset - symbolsStartOffset,
                            /*Index of dynStrTab*/ 5);
    shNameOffset += sizeof(_dynSymSectionName);

    initializeStrTabSection(shNameOffset, symbolsStartOffset, shStrTabNameLength);
    shNameOffset += sizeof(_shStrTabSectionName);

    initializeDynStrSection(shNameOffset,
                            symbolNamesStartOffset, 
                            _totalELFSymbolNamesLength);
    shNameOffset += sizeof(_dynStrSectionName);

    ELFSymbol *elfSymbols = _ELFSymbols + 0;
    char *elfSymbolNames = (char *) (elfSymbols + (_numSymbols+1));

    // first symbol is UNDEF symbol: all zeros, even name is zero-terminated empty string
    elfSymbolNames[0] = 0;
    elfSymbols[0].st_name = 0;
    elfSymbols[0].st_info = ELF_ST_INFO(0,0);
    elfSymbols[0].st_other = 0;
    elfSymbols[0].st_shndx = 0;
    elfSymbols[0].st_value = 0;
    elfSymbols[0].st_size = 0;

    TR::CodeCacheSymbol *sym = _symbols;
    ELFSymbol *elfSym = elfSymbols + 1;
    char *names = elfSymbolNames + 1;

    while (sym)
      {
        //fprintf(stderr, "Writing elf symbol %d, name(%d) = %s\n", (elfSym - elfSymbols), sym->_nameLength, sym->_name);
        memcpy(names, sym->_name, sym->_nameLength);

        elfSym->st_name = names - elfSymbolNames;
        elfSym->st_info = ELF_ST_INFO(STB_GLOBAL,STT_FUNC);
        elfSym->st_other = ELF64_ST_VISIBILITY(STV_DEFAULT);
        elfSym->st_shndx = sym->_start ? 1 : SHN_UNDEF; // text section
        elfSym->st_value = sym->_start ? static_cast<ELFAddress>(sym->_start - _codeStart) : 0;
        elfSym->st_size = sym->_size;
        names += sym->_nameLength;
        elfSym++;

        sym = sym->_next;
    }
    
    //build elf rela entries
    ELFRela *elfRela = _ELFRela + 0;
    TR::CodeCacheRelocationInfo *reloc = _relocations;
    while (reloc){
        elfRela->r_offset = static_cast<ELFAddress>(reloc->_location - _codeStart);
        elfRela->r_info = ELF64_R_INFO(reloc->_symbol + 1, reloc->_type);
        elfRela->r_addend = 0;
        elfRela++;
        reloc = reloc->_next;
    }
}

void 
TR::ELFRelocatableGenerator::emitELF(const char * filename,
                CodeCacheSymbol *symbols, uint32_t numSymbols,
                uint32_t totalELFSymbolNamesLength,
                CodeCacheRelocationInfo *relocations,
                uint32_t numRelocations){
    _symbols = symbols;
    _relocations = relocations;
    _numSymbols = numSymbols;
    _numRelocations = numRelocations;
    _totalELFSymbolNamesLength = totalELFSymbolNamesLength;
    
    buildELFTrailer();

    ::FILE *elfFile = fopen(filename, "wb");
    
    buildELFFile(elfFile);

    fclose(elfFile);
}

#endif //LINUX