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

#if (HOST_OS == OMR_LINUX)

#include "codegen/ELFGenerator.hpp"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <elf.h>
#include "env/CompilerEnv.hpp"
#include "control/Options.hpp"
#include "control/Options_inlines.hpp"


TR::ELFGenerator::ELFGenerator(TR::RawAllocator rawAllocator,
                    uint8_t const * codeStart, size_t codeSize,
                    bool isRelocatable = false):
                    _rawAllocator(rawAllocator),
                    _codeStart(codeStart),
                    _codeSize(codeSize),
                    _totalELFSymbolNamesLength(0),
                    _isRelocatable(isRelocatable),
                    {
                        _trailerStructSize = isRelocatable ? sizeof(ELFRelocatableTrailer) : sizeof(ELFTrailer);
                        initializeHeader();
                    }


TR::ELFGenerator::~ELFGenerator() throw {}

void TR::ELFGenerator::initializeELFHeader(){
   if (_isRelocatable){
        ELFRelocatableHeader *hdr = 
            static_cast<ELFRelocatableHeader *>(_rawAllocator.allocate(sizeof(ELFRelocatableHeader),
            std::nothrow));
        hdr->hdr.e.type = ET_REL;           
        hdr->hdr.e_entry = 0; //relocatable          
        hdr->hdr.e_phoff = 0; //relocatable
        hdr->hdr.e_shoff = sizeof(ELFRelocatableHeader) + _codeSize;
        hdr->hdr.e_phentsize = 0;
        hdr->hdr.e_phnum = 0;
        hdr->hdr.e_shnum = 6;
        hdr->hdr.e_shstrndx = 4;


   }
   else{
       ELFExecutableHeader *hdr =
            static_cast<ELFExecutableHeader *>(_rawAllocator.allocate(sizeof(ELFExecutableHeader),
            std::nothrow));
        hdr->hdr.e.type = ET_EXEC;
        /* extra needed for this */hdr->hdr.e_entry = (ELFAddress) _codeStart;
        hdr->hdr.e_phoff = offsetof(ELFCodeCacheHeader, phdr); //exec only
        hdr->hdr.e_shoff = sizeof(ELFExecutableHeader) + _codeSize;
        hdr->hdr.e_phentsize = sizeof(ELFProgramHeader); //0 for reloc
        hdr->hdr.e_phnum = 1; //0 for reloc
        hdr->hdr.e_shnum = 5; // number of sections in trailer (6 for reloc)
        hdr->hdr.e_shstrndx = 3; // index to shared string table section in trailer (4 for reloc)
        initializePHeader(hdr);
   }

   // main elf header
   hdr->hdr.e_ident[EI_MAG0] = ELFMAG0;
   hdr->hdr.e_ident[EI_MAG1] = ELFMAG1;
   hdr->hdr.e_ident[EI_MAG2] = ELFMAG2;
   hdr->hdr.e_ident[EI_MAG3] = ELFMAG3;
   hdr->hdr.e_ident[EI_CLASS] = ELFClass;
   hdr->hdr.e_ident[EI_VERSION] = EV_CURRENT;
   for (auto b = EI_PAD;b < EI_NIDENT;b++)
      hdr->hdr.e_ident[b] = 0;

    //platform specific inititializations
   #if (HOST_ARCH == ARCH_X86)
      hdr->hdr.e_machine = EM_386;
   #elif (HOST_ARCH == ARCH_POWER)
      #if defined(TR_TARGET_64BIT)
         hdr->hdr.e_machine = EM_PPC64;
      #else
         hdr->hdr.e_machine = EM_PPC;
      #endif
   #elif (HOST_ARCH == ARCH_ZARCH)
      hdr->hdr.e_machine = EM_S390;
   #else
      TR_ASSERT(0, "unrecognized architecture: cannot initialize code cache elf header");
   #endif

   #if (HOST_OS == OMR_LINUX)
      hdr->hdr.e_ident[EI_OSABI] = ELFOSABI_LINUX;
   #elif defined(AIXPPC)
      hdr->hdr.e_ident[EI_OSABI] = ELFOSABI_AIX;
   #else
      TR_ASSERT(0, "unrecognized operating system: cannot initialize code cache elf header");
   #endif
   
   hdr->hdr.e_ident[EI_ABIVERSION] = 0;
   hdr->hdr.e_ident[EI_DATA] = TR::Compiler->host.cpu.isLittleEndian() ? ELFDATA2LSB : ELFDATA2MSB;
   hdr->hdr.e_version = EV_CURRENT;
   hdr->hdr.e_flags = 0;
   hdr->hdr.e_ehsize = sizeof(ELFHeader);
   hdr->hdr.e_shentsize = sizeof(ELFSectionHeader);

   if(_isRelocatable){
       _elfRelocatableHeader = hdr;
   }
   else{
       _elfHeader = hdr;
   }
}

void TR::ELFGenerator::initializePHeader(ELFExecutableHeader *hdr){
    // program header initialization (executable only)
   hdr->phdr.p_type = PT_LOAD;
   hdr->phdr.p_offset = sizeof(ELFCodeCacheHeader);
   hdr->phdr.p_vaddr = (ELFAddress) _codeStart;
   hdr->phdr.p_paddr = (ELFAddress) _codeStart;
   hdr->phdr.p_filesz = _codeSize;
   hdr->phdr.p_memsz = _codeSize;
   hdr->phdr.p_flags = PF_X | PF_R; // should add PF_W if we get around to loading patchable code
   hdr->phdr.p_align = 0x1000;
}


void TR::ELFGenerator::initializeELFTrailer(){
    _elfTrailerSize = _trailerStructSize +
                     _numELFSymbols * sizeof(ELFSymbol) + // NOTE: ELFCodeCacheTrailer includes 1 ELFSymbol: UNDEF
                     _totalELFSymbolNamesLength;
    uint32_t trailerStartOffset, symbolStartOffset, symbolNamesStartOffset
    uint32_t relaStartOffset = 0;
    if (_isRelocatable){
        ELFRelocatableTrailer *trlr = static_cast<ELFRelocatableTrailer *>(_rawAllocator.allocate(_elfTrailerSize));
        size_t trailerStartOffset = sizeof(ELFHeader) + _codeSize;
        size_t symbolsStartOffset = trailerStartOffset + offsetof(ELFTrailer, symbols);
        size_t symbolNamesStartOffset = symbolsStartOffset + (numELFSymbols()+1) * sizeof(ELFSymbol);
        size_t relaStartOffset = symbolNamesStartOffset + _totalELFSymbolNamesLength;
        
    }
    else{
        trailerStructSize = sizeof(ELFExecutableHeader);
        uint32_t trailerStartOffset = sizeof(ELFCodeCacheHeader) + _elfHeader->phdr.p_filesz;
        uint32_t symbolsStartOffset = trailerStartOffset + offsetof(ELFCodeCacheTrailer, symbols);
        uint32_t symbolNamesStartOffset = symbolsStartOffset + (_numELFSymbols+1) * sizeof(ELFSymbol);
    }

    trlr->zeroSection.sh_name = 0;
    trlr->zeroSection.sh_type = 0;
    trlr->zeroSection.sh_flags = 0;
    trlr->zeroSection.sh_addr = 0;
    trlr->zeroSection.sh_offset = 0;
    trlr->zeroSection.sh_size = 0;
    trlr->zeroSection.sh_link = 0;
    trlr->zeroSection.sh_info = 0;
    trlr->zeroSection.sh_addralign = 0;
    trlr->zeroSection.sh_entsize = 0;

    trlr->textSection.sh_name = trlr->textSectionName - trlr->zeroSectionName;
    trlr->textSection.sh_type = SHT_PROGBITS;
    trlr->textSection.sh_flags = SHF_ALLOC | SHF_EXECINSTR;
    trlr->textSection.sh_addr = _isRelocatable ? 0 : ((ELFAddress) _codeStart);
    trlr->textSection.sh_offset = _isRelocatable ? sizeof(ELFRelocatableHeader) : sizeof(ELFExecutableHeader);
    trlr->textSection.sh_size = _codeSize;
    trlr->textSection.sh_link = 0;
    trlr->textSection.sh_info = 0;
    trlr->textSection.sh_addralign = 32;
    trlr->textSection.sh_entsize = 0;
    
    if (_isRelocatable){
        trlr->relaSection.sh_name = trlr->relaSectionName - trlr->zeroSectionName;
        trlr->relaSection.sh_type = SHT_RELA;
        trlr->relaSection.sh_flags = 0;
        trlr->relaSection.sh_addr = 0;
        trlr->relaSection.sh_offset = relaStartOffset;
        trlr->relaSection.sh_size = numELFRelocations() * sizeof(ELFRela);
        trlr->relaSection.sh_link = 3; // dynsymSection
        trlr->relaSection.sh_info = 1;
        trlr->relaSection.sh_addralign = 8;
        trlr->relaSection.sh_entsize = sizeof(ELFRela);
    }

    trlr->dynsymSection.sh_name = trlr->dynsymSectionName - trlr->zeroSectionName;
    trlr->dynsymSection.sh_type = SHT_SYMTAB; // SHT_DYNSYM
    trlr->dynsymSection.sh_flags = 0; //SHF_ALLOC;
    trlr->dynsymSection.sh_addr = 0; //(ELFAddress) &((uint8_t *)_elfHeader + symbolStartOffset); // fake address because not continuous
    trlr->dynsymSection.sh_offset = symbolsStartOffset;
    trlr->dynsymSection.sh_size = (_numSymbols + 1)*sizeof(ELFSymbol);
    trlr->dynsymSection.sh_link = isRelocatable ? 5 : 4; // dynamic string table index
    trlr->dynsymSection.sh_info = 1; // index of first non-local symbol: for now all symbols are global
    trlr->dynsymSection.sh_addralign = 8;
    trlr->dynsymSection.sh_entsize = sizeof(ELFSymbol);

    trlr->shstrtabSection.sh_name = trlr->shstrtabSectionName - trlr->zeroSectionName;
    trlr->shstrtabSection.sh_type = SHT_STRTAB;
    trlr->shstrtabSection.sh_flags = 0;
    trlr->shstrtabSection.sh_addr = 0;
    trlr->shstrtabSection.sh_offset = trailerStartOffset + offsetof(ELFTrailer, zeroSectionName);
    trlr->shstrtabSection.sh_size = sizeof(trlr->zeroSectionName) +
                                    sizeof(trlr->shstrtabSectionName) +
                                    sizeof(trlr->textSectionName) +
                                    sizeof(trlr->dynsymSectionName) +
                                    sizeof(trlr->dynstrSectionName);
    if (_isRelocatable) trlr->shstrtabSection.sh_size += sizeof(trlr->relaSectionName);
    trlr->shstrtabSection.sh_link = 0;
    trlr->shstrtabSection.sh_info = 0;
    trlr->shstrtabSection.sh_addralign = 1;
    trlr->shstrtabSection.sh_entsize = 0;

    trlr->dynstrSection.sh_name = trlr->dynstrSectionName - trlr->zeroSectionName;
    trlr->dynstrSection.sh_type = SHT_STRTAB;
    trlr->dynstrSection.sh_flags = 0;
    trlr->dynstrSection.sh_addr = 0;
    trlr->dynstrSection.sh_offset = symbolNamesStartOffset;
    trlr->dynstrSection.sh_size = _totalELFSymbolNamesLength;
    trlr->dynstrSection.sh_link = 0;
    trlr->dynstrSection.sh_info = 0;
    trlr->dynstrSection.sh_addralign = 1;
    trlr->dynstrSection.sh_entsize = 0;

    trlr->zeroSectionName[0] = 0;
    strcpy(trlr->shstrtabSectionName, ".shstrtab");
    strcpy(trlr->textSectionName, ".text");
    if (_isRelocatable) strcpy(trlr->relaSectionName, ".rela.text");
    strcpy(trlr->dynsymSectionName, ".symtab");
    strcpy(trlr->dynstrSectionName, ".strtab");

   // now walk list of compiled code symbols building up the symbol names and filling in array of ELFSymbol structures
   ELFSymbol *elfSymbols = trlr->symbols + 0;
   char *elfSymbolNames = (char *) (elfSymbols + (_numELFSymbols+1));

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
      if(_isRelocatable){
          elfSym->st_other = ELF64_ST_VISIBILITY(STV_DEFAULT);
          elfSym->st_shndx = sym->_start ? 1 : SHN_UNDEF; // text section
          elfSym->st_value = sym->_start ? static_cast<ELFAddress>(it->_start - _codeStart) : 0;
      }
      else{
          elfSym->st_other = 0;
          elfSym->st_shndx = 1;
          elfSym->st_value = (ELFAddress) sym->_start;
      }
      elfSym->st_size = sym->_size;
      names += sym->_nameLength;
      elfSym++;

      sym = sym->_next;
    }

    if(_isRelocatable){
        ELFObjectFileRelocation * reloc = _relocations;
        ELFRela *elfRela = pointer_cast<ELFRela *>(elfSymbolNames + _totalELFSymbolNamesLength);
        while (reloc){
            elfRela->r_offset = static_cast<ELFAddress>(reloc->_location - _codeStart);
            elfRela->r_info = ELF64_R_INFO(reloc->_symbol + 1, reloc->_type);
            elfRela->r_addend = 0;
            elfRela++;
            reloc = reloc->_next;
        }
    }
    
    if (_isRelocatable){
        _elfRelocatableTrailer = trlr;
    }
    else{
        _elfTrailer = trlr;
    }
   
}

void TR::ELFGenerator::emitELF(const char * filename,
                CodeCacheSymbol *symbols, uint32_t numSymbols,
                uint32_t totalELFSymbolNamesLength,
                CodeCacheRelocationInfo *relocations = NULL,
                uint32_t numRelocations = 0){
    //init trailer and write to file
    _symbols = symbols;
    _relocations = relocations;
    _numSymbols = numSymbols;
    _numRelocations = numRelocations;
    _totalELFSymbolNamesLength = totalELFSymbolNamesLength;
    initializeELFTrailer();

    FILE *elfFile = fopen(filename, "wb");
    if (_isRelocatable){
        fwrite(_elfRelocatableHeader, sizeof(uint8_t), sizeof(ELFRelocatableHeader), elfFile);
        fwrite(_codeStart, sizeof(uint8_t), _codeSize, elfFile);
        fwrite(_elfRelocatableTrailer, sizeof(uint8_t), _elfTrailerSize, elfFile);
    }
    else{
        fwrite(_elfHeader, sizeof(uint8_t), sizeof(ELFExecutableHeader), elfFile);
        fwrite(_codeStart, sizeof(uint8_t), _codeSize, elfFile);
        fwrite(_elfTrailer, sizeof(uint8_t), _elfTrailerSize, elfFile);
    }
    fclose(elfFile);
}

#endif /* HOST_OS == OMR_LINUX */
