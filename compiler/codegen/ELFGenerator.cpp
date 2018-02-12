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
                            ELFGenerator(rawAllocator),
                            _codeStart(codeStart),
                            _codeSize(codeSize),
                            _trailerStructSize(sizeof(ELFTrailer))
                            {
                                initialize();
                            }

TR::ELFRelocatableGenerator::ELFRelocatableGenerator(TR::RawAllocator rawAllocator,
                            uint8_t const * codeStart, size_t codeSize):
                            ELFGenerator(rawAllocator),
                            _codeStart(codeStart),
                            _codeSize(codeSize),
                            _trailerStructSize(sizeof(ELFTrailer))
                            {
                                initialize();
                            }

void
TR::ELFGenerator::initializeELFHeaderForPlatform(ELFEHeader *hdr){
    hdr->e_ident[EI_MAG0] = ELFMAG0;
    hdr->e_ident[EI_MAG1] = ELFMAG1;
    hdr->e_ident[EI_MAG2] = ELFMAG2;
    hdr->e_ident[EI_MAG3] = ELFMAG3;
    hdr->e_ident[EI_CLASS] = ELFClass;
    hdr->e_ident[EI_VERSION] = EV_CURRENT;
    for (auto b = EI_PAD;b < EI_NIDENT;b++)
        hdr->e_ident[b] = 0;

    //platform specific inititializations
    #if defined(TR_TARGET_X86)
        hdr->e_machine = EM_386;
    #elif defined(TR_TARGET_POWER)
        #if defined(TR_TARGET_64BIT)
            hdr->e_machine = EM_PPC64;
        #else
            hdr->e_machine = EM_PPC;
        #endif
    #elif (TR_TARGET_S390)
        hdr->e_machine = EM_S390;
    #else
        TR_ASSERT(0, "unrecognized architecture: ELF header cannot be initialized");
    #endif

    #if defined(LINUX)
        hdr->e_ident[EI_OSABI] = ELFOSABI_LINUX;
    #elif defined(AIXPPC)
        hdr->e_ident[EI_OSABI] = ELFOSABI_AIX;
    #else
        TR_ASSERT(0, "unrecognized operating system: ELF header cannot be initialized");
    #endif
   
    hdr->e_ident[EI_ABIVERSION] = 0;
    hdr->e_ident[EI_DATA] = TR::Compiler->target.cpu.isLittleEndian() ? ELFDATA2LSB : ELFDATA2MSB;
    hdr->e_version = EV_CURRENT;
    hdr->e_flags = 0;
    hdr->e_ehsize = sizeof(ELFEHeader);
    hdr->e_shentsize = sizeof(ELFSectionHeader);
}

void 
TR::ELFGenerator::initializeELFTrailerZeroSection(ELFSectionHeader *shdr){
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
}
    
void 
TR::ELFGenerator::initializeELFTrailerTextSection(ELFSectionHeader *shdr, uint32_t shName, ELFAddress shAddress,
                                                 ELFOffset shOffset, uint32_t shSize){
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

}
    
void
TR::ELFGenerator::initializeELfTrailerDynSymSection(ELFSectionHeader *shdr, uint32_t shName, ELFOffset shOffset, uint32_t shSize, uint32_t shLink){
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
}

void
TR::ELFGenerator::initializeELFTrailerStrTabSection(ELFSectionHeader *shdr, uint32_t shName, ELFOffset shOffset, uint32_t shSize){
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
}

void
TR::ELFGenerator::initializeELFTrailerDynStrSection(ELFSectionHeader *shdr, uint32_t shName, ELFOffset shOffset, uint32_t shSize){
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
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void
TR::ELFExecutableGenerator::initialize(void){
    ELFHeader *hdr =
        static_cast<ELFHeader *>(_rawAllocator.allocate(sizeof(ELFHeader),
        std::nothrow));
    _elfHeader = hdr;
    initializeELFHeader();
    initializeELFHeaderForPlatform(&(_elfHeader->hdr));
    initializePHdr();
}

void
TR::ELFExecutableGenerator::initializeELFHeader(void){
    _elfHeader->hdr.e_type = ET_EXEC;
    _elfHeader->hdr.e_entry = (ELFAddress) _codeStart;
    _elfHeader->hdr.e_phoff = offsetof(ELFHeader, phdr); //program header offset
    _elfHeader->hdr.e_shoff = sizeof(ELFHeader) + _codeSize;
    _elfHeader->hdr.e_phentsize = sizeof(ELFProgramHeader); //0 for reloc
    _elfHeader->hdr.e_phnum = 1; //0 for reloc
    _elfHeader->hdr.e_shnum = 5; // number of sections in trailer (6 for reloc)
    _elfHeader->hdr.e_shstrndx = 3; // index to shared string table section in trailer (4 for reloc)
}

void
TR::ELFExecutableGenerator::initializePHdr(void){
    // program header initialization (executable only)
    _elfHeader->phdr.p_type = PT_LOAD;
    _elfHeader->phdr.p_offset = sizeof(ELFEHeader) + sizeof(ELFProgramHeader);
    _elfHeader->phdr.p_vaddr = (ELFAddress) _codeStart;
    _elfHeader->phdr.p_paddr = (ELFAddress) _codeStart;
    _elfHeader->phdr.p_filesz = _codeSize;
    _elfHeader->phdr.p_memsz = _codeSize;
    _elfHeader->phdr.p_flags = PF_X | PF_R; // should add PF_W if we get around to loading patchable code
    _elfHeader->phdr.p_align = 0x1000;
}

void
TR::ELFExecutableGenerator::initializeELFTrailer(void){
    _elfTrailerSize = _trailerStructSize +
                     _numSymbols * sizeof(ELFSymbol) + // NOTE: ELFCodeCacheTrailer includes 1 ELFSymbol: UNDEF
                     _totalELFSymbolNamesLength;
    /* offset calculations */
    ELFTrailer *trlr = static_cast<ELFTrailer *>(_rawAllocator.allocate(_elfTrailerSize));
    uint32_t trailerStartOffset = sizeof(ELFHeader) + _codeSize;
    uint32_t symbolsStartOffset = trailerStartOffset + offsetof(ELFTrailer, symbols);
    uint32_t symbolNamesStartOffset = symbolsStartOffset + (_numSymbols+1) * sizeof(ELFSymbol);

    initializeELFTrailerZeroSection(&(trlr->zeroSection));

    initializeELFTrailerTextSection(&(trlr->textSection), trlr->textSectionName - trlr->zeroSectionName,
                                    (ELFAddress) _codeStart, sizeof(ELFHeader), _codeSize);

    initializeELfTrailerDynSymSection(&(trlr->dynsymSection), trlr->dynsymSectionName - trlr->zeroSectionName,
                                        symbolsStartOffset, symbolNamesStartOffset - symbolsStartOffset, 4);
    uint32_t strTabSize = sizeof(trlr->zeroSectionName) +
                            sizeof(trlr->shstrtabSectionName) +
                            sizeof(trlr->textSectionName) +
                            sizeof(trlr->dynsymSectionName) +
                            sizeof(trlr->dynstrSectionName);
    uint32_t strTabOffset = trailerStartOffset + offsetof(ELFTrailer, zeroSectionName);
    initializeELFTrailerStrTabSection(&(trlr->shstrtabSection),  trlr->shstrtabSectionName - trlr->zeroSectionName,
                                        strTabOffset, strTabSize);

    initializeELFTrailerDynStrSection(&(trlr->dynstrSection), trlr->dynstrSectionName - trlr->zeroSectionName,
                                        symbolNamesStartOffset, _totalELFSymbolNamesLength);

    trlr->zeroSectionName[0] = 0;
    strcpy(trlr->shstrtabSectionName, ".shstrtab");
    strcpy(trlr->textSectionName, ".text");
    strcpy(trlr->dynsymSectionName, ".symtab");
    strcpy(trlr->dynstrSectionName, ".strtab");

    ELFSymbol *elfSymbols = trlr->symbols + 0;
    char *elfSymbolNames = (char *) (elfSymbols + (_numSymbols+1));

    // first symbol is UNDEF symbol: all zeros, even name is zero-terminated empty string
    elfSymbolNames[0] = 0;
    elfSymbols[0].st_name = 0;
    elfSymbols[0].st_info = ELF_ST_INFO(0,0);
    elfSymbols[0].st_other = 0;
    elfSymbols[0].st_shndx = 0;
    elfSymbols[0].st_value = 0;
    elfSymbols[0].st_size = 0;

    CodeCacheSymbol *sym = _symbols;
    ELFSymbol *elfSym = elfSymbols + 1;
    char *names = elfSymbolNames + 1;

    while (sym)
      {
      //fprintf(stderr, "Writing elf symbol %d, name(%d) = %s\n", (elfSym - elfSymbols), sym->_nameLength, sym->_name);
        memcpy(names, sym->_name, sym->_nameLength);

        elfSym->st_name = names - elfSymbolNames;
        elfSym->st_info = ELF_ST_INFO(STB_GLOBAL,STT_FUNC);
        elfSym->st_other = ELF64_ST_VISIBILITY(STV_DEFAULT);;
        elfSym->st_shndx = sym->_start ? 1 : SHN_UNDEF;
        elfSym->st_value = sym->_start ? static_cast<ELFAddress>(sym->_start - _codeStart) : 0;
        elfSym->st_size = sym->_size;
        names += sym->_nameLength;
        elfSym++;

        sym = sym->_next;
    }
    _elfTrailer = trlr;
}

void
TR::ELFExecutableGenerator::emitELF(const char * filename,
                CodeCacheSymbol *symbols, uint32_t numSymbols,
                uint32_t totalELFSymbolNamesLength){
    //init trailer and write to file
    _symbols = symbols;
    _numSymbols = numSymbols;
    _totalELFSymbolNamesLength = totalELFSymbolNamesLength;
    
    initializeELFTrailer();

    ::FILE *elfFile = fopen(filename, "wb");
    fwrite(_elfHeader, sizeof(uint8_t), sizeof(ELFHeader), elfFile);
    fwrite(_codeStart, sizeof(uint8_t), _codeSize, elfFile);
    fwrite(_elfTrailer, sizeof(uint8_t), _elfTrailerSize, elfFile);
    fclose(elfFile);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void
TR::ELFRelocatableGenerator::initialize(void){
    ELFHeader *hdr =
        static_cast<ELFHeader *>(_rawAllocator.allocate(sizeof(ELFHeader),
        std::nothrow));
    _elfHeader = hdr;
    initializeELFHeader();
    initializeELFHeaderForPlatform(&(_elfHeader->hdr));
}

void
TR::ELFRelocatableGenerator::initializeELFHeader(void){
    _elfHeader->hdr.e_type = ET_REL;           
    _elfHeader->hdr.e_entry = 0;
    _elfHeader->hdr.e_phoff = 0;
    _elfHeader->hdr.e_shoff = sizeof(ELFHeader) + _codeSize;
    _elfHeader->hdr.e_phentsize = 0;
    _elfHeader->hdr.e_phnum = 0;
    _elfHeader->hdr.e_shnum = 6;
    _elfHeader->hdr.e_shstrndx = 4;
}

void
TR::ELFRelocatableGenerator::initializeELFTrailer(void){
    _elfTrailerSize = _trailerStructSize +
                     _numSymbols * sizeof(ELFSymbol) + // NOTE: ELFCodeCacheTrailer includes 1 ELFSymbol: UNDEF
                     _totalELFSymbolNamesLength;
    ELFTrailer *trlr = static_cast<ELFTrailer *>(_rawAllocator.allocate(_elfTrailerSize));

    /* offset calculations */
    uint32_t trailerStartOffset = sizeof(ELFHeader) + _codeSize;
    uint32_t symbolsStartOffset = trailerStartOffset + offsetof(ELFTrailer, symbols);
    uint32_t symbolNamesStartOffset = symbolsStartOffset + (_numSymbols+1) * sizeof(ELFSymbol);
    uint32_t relaStartOffset = symbolNamesStartOffset + _totalELFSymbolNamesLength;

    initializeELFTrailerZeroSection(&(trlr->zeroSection));


    initializeELFTrailerTextSection(&(trlr->textSection), trlr->textSectionName - trlr->zeroSectionName,
                                        0, sizeof(ELFHeader), _codeSize);

    /* Initialize Rela Section */
    trlr->relaSection.sh_name = trlr->relaSectionName - trlr->zeroSectionName;
    trlr->relaSection.sh_type = SHT_RELA;
    trlr->relaSection.sh_flags = 0;
    trlr->relaSection.sh_addr = 0;
    trlr->relaSection.sh_offset = relaStartOffset;
    trlr->relaSection.sh_size = _numRelocations * sizeof(ELFRela);
    trlr->relaSection.sh_link = 3; // dynsymSection
    trlr->relaSection.sh_info = 1;
    trlr->relaSection.sh_addralign = 8;
     trlr->relaSection.sh_entsize = sizeof(ELFRela);

    initializeELfTrailerDynSymSection(&(trlr->dynsymSection), trlr->dynsymSectionName - trlr->zeroSectionName,
                                        symbolsStartOffset, symbolNamesStartOffset - symbolsStartOffset, 5);

    uint32_t strTabSize = sizeof(trlr->zeroSectionName) +
                            sizeof(trlr->shstrtabSectionName) +
                            sizeof(trlr->textSectionName) +
                            sizeof(trlr->dynsymSectionName) +
                            sizeof(trlr->dynstrSectionName) +
                            sizeof(trlr->relaSectionName);
    uint32_t strTabOffset = trailerStartOffset + offsetof(ELFTrailer, zeroSectionName);
    initializeELFTrailerStrTabSection(&(trlr->shstrtabSection), trlr->shstrtabSectionName - trlr->zeroSectionName,
                                        strTabOffset, strTabSize);

    initializeELFTrailerDynStrSection(&(trlr->dynstrSection), trlr->dynstrSectionName - trlr->zeroSectionName,
                                        symbolNamesStartOffset, _totalELFSymbolNamesLength);

    trlr->zeroSectionName[0] = 0;
    strcpy(trlr->shstrtabSectionName, ".shstrtab");
    strcpy(trlr->textSectionName, ".text");
    strcpy(trlr->relaSectionName, ".rela.text");
    strcpy(trlr->dynsymSectionName, ".symtab");
    strcpy(trlr->dynstrSectionName, ".strtab");

    ELFSymbol *elfSymbols = trlr->symbols + 0;
    char *elfSymbolNames = (char *) (elfSymbols + (_numSymbols+1));

    // first symbol is UNDEF symbol: all zeros, even name is zero-terminated empty string
    elfSymbolNames[0] = 0;
    elfSymbols[0].st_name = 0;
    elfSymbols[0].st_info = ELF_ST_INFO(0,0);
    elfSymbols[0].st_other = 0;
    elfSymbols[0].st_shndx = 0;
    elfSymbols[0].st_value = 0;
    elfSymbols[0].st_size = 0;

    CodeCacheSymbol *sym = _symbols;
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

    CodeCacheRelocationInfo * reloc = _relocations;
    ELFRela *elfRela = pointer_cast<ELFRela *>(elfSymbolNames + _totalELFSymbolNamesLength);
    while (reloc){
        elfRela->r_offset = static_cast<ELFAddress>(reloc->_location - _codeStart);
        elfRela->r_info = ELF64_R_INFO(reloc->_symbol + 1, reloc->_type);
        elfRela->r_addend = 0;
        elfRela++;
        reloc = reloc->_next;
    }
    _elfTrailer = trlr;
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
    
    initializeELFTrailer();

    ::FILE *elfFile = fopen(filename, "wb");
    fwrite(_elfHeader, sizeof(uint8_t), sizeof(ELFHeader), elfFile);
    fwrite(_codeStart, sizeof(uint8_t), _codeSize, elfFile);
    fwrite(_elfTrailer, sizeof(uint8_t), _elfTrailerSize, elfFile);
    fclose(elfFile);
}

#endif //LINUX