#include <windows.h>
#include <mem.h>
#include <bwcc.h>
#include <stdio.h>
#include <dos.h>				; voor FP_SEG en FP_OFF
#include "spectrum.h"

#define pt_rom 0
#define pt_ram 1
#define pt_loram 2
#define pt_hiram 3
#define pt_special 4

HGLOBAL	hKeyword;			// handle to spectrum keywords, for rs232 output
HGLOBAL  pages[13];
char     pagetype[13];
char     pagemap[4];
char		useVz80d;
char		IniUseVz80d=1;
long		hVz80d;
MEMSTATE memstate;
char     page7locked;
BYTE		FAR *page7fp;

void     pageram0(void);

#define LoadRomPage(pg,of) \
	{BYTE *p=temppage(pg);LoadRom(p,16384,of,pg);tempunpage(pg);}

void LoadRom(BYTE *where, WORD len, long offset, int page)
{
	HFILE handle;
	char *name=gszRomFile;
	char errmsg[100];
	long ofs=offset;
	int i;
	char copy8k=FALSE;
	switch (offset) {
		case rom_48k:
			if (gszRom48k[0]) {
				name=gszRom48k;
				ofs=0;
			}
			break;
		case rom_mface:
			if (gszRomMface[0]) {
				name=gszRomMface;
				ofs=0;
			}
			break;
		case rom_currah:
			if (gszRomCurrah[0]) {
				name=gszRomCurrah;
				ofs=0;
			}
			break;
		case rom_if1:
			if (gszRomIf1[0]) {
				name=gszRomIf1;
				ofs=0;
				if (romif1_8k)
					copy8k=TRUE;		// copy 8k into second part if necessary
				else
					len=16384;
			} else
				copy8k = TRUE;			// default rom: always copy second part
			break;
		case rom_1281:
			if (gszRom1281[0]) {
				name=gszRom1281;
				ofs=0;
			}
			break;
		case rom_1282:
			if (gszRom1282[0]) {
				name=gszRom1282;
				ofs=0;
			}
			break;
	}
	handle=OpenRead(name);
	if (handle==-1) {
		if (name==gszRomFile) {
			fatalerror(FatalNoRomfile);return;
		}
		sprintf(errmsg,"Could not find ROM from file '%s'.",name);
		MyMessageBox(hWndMain,errmsg,"Error:",MB_ICONHAND|MB_OK);
		return;
	}
	_llseek(handle,ofs,0);
	if (_lread(handle,where,len)!=len) {
		_lclose(handle);
		if (name==gszRomFile) {
			fatalerror(FatalBadRomfile);return;
		}
		sprintf(errmsg,"Error reading ROM file '%s' -- probably not enough bytes to read (I need %u bytes starting from offset %lu).",name,(int)len,ofs);
		MyMessageBox(hWndMain,errmsg,"Error:",MB_ICONHAND|MB_OK);
		return;
	}
	_lclose(handle);
	if (copy8k) {
		for (i=0;i<8192;i++)
			where[i+8192]=where[i];
	}
	for (i=0;i<3;i++)                         // signal 'unknown page' in banks
		if (pagemap[i]==page) pagemap[i]=-1;   // holding page 'page'.
}


BYTE *temppage(int i)
{
	BYTE *p;
	if (useVz80d) {
		vz80d_page(hVz80d,0,i);
		return SpecMem;
	}
	p = GlobalLock(pages[i]);
	return p;
}

void tempunpage(int i)
{
	if (useVz80d) {
		if (pagemap[0]>=0) vz80d_page(hVz80d,0,pagemap[0]);
	} else
		GlobalUnlock(pages[i]);
}


void init_memory(void)
{
	int i;
	char *p;
	hKeyword=GlobalAlloc(GPTR,368);
	p=(char*)GlobalLock(hKeyword);
	LoadRom(p,368,0x96,-1);
	GlobalUnlock(hKeyword);
	useVz80d = IniUseVz80d;
	if (useVz80d) {
		init_vz80d_api_call(gszVz80dName);
		if ( (!vz80d_segm) && (!vz80d_addr) ) {
			WriteInfoString("Error loading VZ80D.VXD");
			useVz80d = 0;
		}
	} else {
		WriteInfoString("Not using VZ80D.VXD - disabled in WinZ80.INI");
	}
	if (useVz80d) {
		int version=vz80d_version();
		if (version != 0x102) {
			useVz80d = 0;
			unload_vz80d();
			WriteInfoString("Error: Wrong version of VZ80D.VXD");
		}
	}
	if (useVz80d) {
		if (vz80d_alloc(5,13,&hVz80d)) {
			useVz80d = 0;
			unload_vz80d();
			WriteInfoString("Error: Could not allocate enough memory through VZ80D.VXD");
		}
	}
	if (useVz80d) {
		vz80d_getframe(hVz80d,&(void*)SpecMem);
//		vz80d_getbuf(hVz80d,&(void*)Vz80d_buf);
		vz80d_getpage(hVz80d,10,&(void*)page7fp);
		page7seg = FP_SEG(page7fp);
//		page7ptr = FP_OFF(Vz80d_buf) + 10*0x4000L;
		page7ptr = FP_OFF(page7fp);
//		page7fp = &(Vz80d_buf[10*0x4000L]);
		WriteInfoString("Using VZ80D.VXD for bank switching.");
	} else {
		hSpecMem=GlobalAlloc(GPTR,0x10002L);
		if (!hSpecMem) {fatalerror(FatalMemAlloc);return;}
		SpecMem=(BYTE*)GlobalLock(hSpecMem);
		for (i=0;i<13;i++) {
			pages[i]=GlobalAlloc(GPTR,0x4000);
			if (!pages[i]) {fatalerror(FatalMemAlloc);return;}
		}
	}
	for (i=0;i<13;i++)
		pagetype[i]=pt_rom;
	z80header.loram=0;
	z80header.hiram=0;
	memstate.diskifpaged=FALSE;
	page7locked=FALSE;
	for (i=0;i<4;i++) pagemap[i]=-1;
	mempage(0,0);
	mempage(1,8);
	mempage(2,4);
	mempage(3,5);
//	LoadRomPage(0,rom_48k);
	pagetype[8]=pt_ram;
	pagetype[4]=pt_ram;
	pagetype[5]=pt_ram;
	i = hmode;
	hmode = -1;
	select_hmode(i);
	ResetAY();
	InitAY();
	Reset();
}

void dealloc_memory(void)
{
	int i;
	if (useVz80d) {
		vz80d_free(hVz80d);
		unload_vz80d();
	} else {
		if (page7locked)
			GlobalUnlock(pages[10]);
		for (i=0;i<13;i++) {
			GlobalUnlock(pages[i]);
			GlobalFree(pages[i]);
		}
		GlobalUnlock(hSpecMem);
		GlobalFree(hSpecMem);
	}
}

void mempage(char bank, char page)
// hack: if mode=128k, and page '5'=2 or '5'=8 is paged, it is copied from
//  the fixed bank in SpecMem instead of the bank memory
// if it is paged out, it is copied to the relevant part of SpecMem
// if currah mem is paged, currah_setbusybit is called too
{
	char j;
	BYTE *ptr,*oldptr;
	if ((j=pagemap[bank])==page) return;
	if (useVz80d) {
		if (vz80d_page(hVz80d,bank,page)) {
			fatalerror(FatalVz80dError);
			return;
		}
		if (bank==0) {
			if (vz80d_page(hVz80d,4,page)) {
				fatalerror(FatalVz80dError);
				return;
			}
		}
	} else {
		ptr=GlobalLock(pages[page]);
		if (j!=-1) switch(pagetype[j]) {
		case pt_ram:
			if ((hmode>=hm_128k)&&((page==5)||(page==8))&&(bank==3)) {
//            memcpy(SpecMem+(page==5?0x8000:0x4000),SpecMem+0xC000,0x4000);
			} else {
				oldptr=GlobalLock(pages[j]);
				memcpy(oldptr,SpecMem+0x4000*bank,0x4000);
				GlobalUnlock(pages[j]);
			}
			break;
		case pt_loram:
			oldptr=GlobalLock(pages[j]);
			memcpy(oldptr,SpecMem+0x4000*bank,0x2000);
			GlobalUnlock(pages[j]);
			break;
		case pt_hiram:
			oldptr=GlobalLock(pages[j]);
			memcpy(oldptr+0x2000,SpecMem+0x2000+0x4000*bank,0x2000);
			GlobalUnlock(pages[j]);
			break;
		}
		if ((hmode<hm_128k)||((page!=5)&&(page!=8))||(bank!=3))
			memcpy(SpecMem+0x4000*bank,ptr,0x4000);
		else
			memcpy(SpecMem+0xC000,SpecMem+(page==5?0x8000:0x4000),0x4000);
		GlobalUnlock(pages[page]);
	}
	pagemap[bank]=page;
	if (bank==0) {
		putrombyteshigh(SpecMem);
		switch (pagetype[page]) {
		case pt_rom:
			z80header.loram=FALSE;
			z80header.hiram=FALSE;
			break;
		case pt_ram:
			z80header.loram=TRUE;
			z80header.hiram=TRUE;
			break;
		case pt_loram:
			z80header.loram=TRUE;
			z80header.hiram=FALSE;
			break;
		case pt_hiram:
			z80header.loram=FALSE;
			z80header.hiram=TRUE;
			break;
		case pt_special:
			z80header.loram=2;
			z80header.hiram=2;
			currah_setbusybit();
			break;
		}
	}
}


void pokepage(char page, int addr, BYTE value)
// used to update 'busy' bit in currah memory-mapped io
// also used by assembler, via pokeasmbyte function below
// DOES NOT CHECK WHETHER YOU POKE IN ROM.
{
	BYTE *pg;
	int i;
	addr &= 0x3fff;
	if (useVz80d) {
		vz80d_pokevalue(hVz80d, 0x4000L*page+addr, value);
	} else {
		pg=GlobalLock(pages[page]);
		pg[addr]=value;
		GlobalUnlock(pages[page]);
		for (i=0;i<4;i++) {
			if (pagemap[i]==page) {
				SpecMem[0x4000*i+addr]=value;
			}
		}
	}
}

void pokeasmbyte(unsigned int addr, char bank, unsigned int val)
// gebruikt door assembler
{
	addr &= 0x3fff;
	if (bank>12) return;
	if (pagetype[bank]==pt_rom) return;
	if ((pagetype[bank]==pt_loram)&&(addr>=0x2000)) return;
	if ((pagetype[bank]==pt_hiram)&&(addr<0x2000)) return;
	pokepage(bank, addr , val);
}

char CurrentBank(unsigned int addr)
// gebruikt door assembler
{
	char b=pagemap[addr >> 14];
	// In 128k modes, bank 2 contains ordinary rom.
	if ((hmode >= hm_128k)&&((b&0xFD)==0))
		b^=2;
	return b;
}

unsigned int Addr2Word(unsigned int addr, char bank, unsigned int cur_pc)
// gebruikt door assembler
{
	int i;
	WORD bestmatch;
	char matched=FALSE;
	if ((bank<0)||(bank>12)) {
		WriteInfoString("Internal error: Addr2Word called with bank out of bounds.");
		bank=0;
	}
	addr &= 0x3fff;
	for (i=0;i<4;i++) {
		if (pagemap[i]==bank) {
			bestmatch = 0x4000*(unsigned)i + addr;
			matched=TRUE;
			if (((cur_pc ^ bestmatch) & 0xc000) == 0) return bestmatch;
		}
	}
	if (matched) return bestmatch;
	if (pagetype[bank]!=pt_ram) return addr;   // pt_rom or pt_special
	switch (bank) {
	case 8:
		return addr+0x4000;
	case 4:
		return addr+0x8000;
	}
	return addr+0xC000;
}


void select_secondarymode()
// loads multiface, disciple roms, etc., if necessary, for ch^s not refl^d in hmode
// also handles SpecDRUM
{
	BYTE *p0;
	int i;
	if (state.multifaceemulated) {
		pagetype[11]=pt_hiram;
		p0=temppage(11);
		LoadRom(p0,8192,rom_mface,11);
		for (i=8192;i<16384;i++)
			p0[i]=0;
		tempunpage(11);
	} else
		state.multifacepaged=FALSE;
	if (state.currahemulated) {
		pagetype[12]=pt_special;
		p0=temppage(12);
		LoadRom(p0,2048,rom_currah,12);
		for (i=0;i<2048;i++)
			p0[i+2048]=p0[i];
		for (i=4096;i<8192;i++)
			p0[i]=0xFF;
		for (i=0;i<8192;i++)
			p0[i+8192]=p0[i];
		tempunpage(12);
	} else
		state.currahpaged=FALSE;
	i=AYemul;
	if (hmode >= hm_128k)
		i=TRUE;
	else if (hmode == hm_samram)
		i=FALSE;
	else
		i=state.ayemu48k;
	if (i!=AYemul) {
		AYemul=i;
		ResetAY();
		InitAY();
	}
	if (!state.specdrumemu)
		specdrumport = specdrumval = 0;
	pagerom();
}

void select_hmode(char newhmode)
// also calls select_secondarymode, and sets rom/ram in state acc. to variables
{
	// to do: retain as much as possible of current mem state
	// make sure i/o state and page state are in agreement
	BYTE *p0;
	int i;
	char oldhmode=hmode;

	if (oldhmode==newhmode) return;

	hmode=newhmode;
	for (i=3;i<12;i++) pagetype[i]=pt_ram;
	pagetype[0]=pagetype[1]=pagetype[2]=pagetype[11]=pt_rom;
	if ((oldhmode<hm_128k)&&(hmode>=hm_128k)) ResetAY();
	if ((oldhmode>=hm_128k)&&(hmode<hm_128k)) ResetAY();
	switch (hmode) {
	case hm_samram:
		LoadRomPage(2,rom_sam1);
		LoadRomPage(3,rom_sam2);
		pagetype[3]=pt_rom;
	case hm_48kif1:
	case hm_48kmgt:
		p0=temppage(1);
		LoadRom(p0,8192,rom_if1,1);
		tempunpage(1);
	case hm_48k:
		LoadRomPage(0,rom_48k);
		if (page7locked) {
		  GlobalUnlock(pages[10]);
		  page7locked=FALSE;
		}
		videopage7=0;
		break;
	case hm_128kif1:
	case hm_128kmgt:
		p0=temppage(1);
		LoadRom(p0,8192,rom_if1,1);
		tempunpage(1);
	case hm_128k:
		LoadRomPage(0,rom_1282);
		LoadRomPage(2,rom_1281);
		if (page7locked) GlobalUnlock(pages[10]);
		page7locked=FALSE;
		videopage7=0;
		if ((state.hstate & 0x08)&&(!useVz80d)) {
			void *p = GlobalLock(pages[10]);
			page7seg = FP_SEG(p);
			page7ptr = FP_OFF(p);
			page7fp = p;
			page7locked=TRUE;
			videopage7=1;
		}
		break;
	}
	select_secondarymode();
	pageram0();
	pageram();
	if (hHardwareDialog)
		SendMessage(hHardwareDialog,WM_INITDIALOG,0,0);
}


void reloadroms(void)
// used by RomDialog procedure
{
	BYTE *p0;
	switch (hmode) {
	case hm_samram:
		LoadRomPage(2,rom_sam1);
		LoadRomPage(3,rom_sam2);
	case hm_48kif1:
	case hm_48kmgt:
		p0=temppage(1);
		LoadRom(p0,8192,rom_if1,1);
		tempunpage(1);
	case hm_48k:
		LoadRomPage(0,rom_48k);
		break;
	case hm_128kif1:
	case hm_128kmgt:
		p0=temppage(1);
		LoadRom(p0,8192,rom_if1,1);
		tempunpage(1);
	case hm_128k:
		LoadRomPage(0,rom_1282);
		LoadRomPage(2,rom_1281);
		break;
	}
	pagemap[0]=-1;
	select_secondarymode();
}


void pagerom(void)
{
	switch (hmode) {
	case hm_48k:
		if (state.currahpaged) mempage(0,12); else
		if (state.multifacepaged) mempage(0,11); else
			mempage(0,0);
		break;
	case hm_48kif1:
	case hm_48kmgt:
		if (state.currahpaged) mempage(0,12); else
		if (state.multifacepaged) mempage(0,11); else {
			if (memstate.diskifpaged)
				mempage(0,1);
			else
				mempage(0,0);
		}
		break;
	case hm_128k:
		if (state.multifacepaged) mempage(0,11); else
			if (state.hstate&0x10) mempage(0,2); else
				mempage(0,0);
		break;
	case hm_128kif1:
	case hm_128kmgt:
		if (state.multifacepaged) mempage(0,11); else {
			if (memstate.diskifpaged)
				mempage(0,1);
			else
				if (state.hstate&0x10) mempage(0,2); else
					 mempage(0,0);
		}
		break;
	case hm_samram:
		if (memstate.diskifpaged)
			mempage(0,1);
		else {
			if (state.hstate & 0x02)
				mempage(0,0);
			else {
				if (state.hstate & 0x08)
					mempage(0,3);
				else
					mempage(0,2);
			}
		}
		break;
	}
	return;
}

const char *romstring()
{
	const char *roms[]={"48k  ","128-2","128-1","sam-1","sam-2","mface","curr.","if1  ","???  "};
	switch (pagemap[0]) {
	case 0:
		if (hmode >= hm_128k)
			return roms[1];
		else
			return roms[0];
	case 1:
		return roms[7];
	case 2:
		if (hmode == hm_samram)
			return roms[3];
		else
			return roms[2];
	case 3:
		return roms[4];
	case 11:
		return roms[5];
	case 12:
		return roms[6];
	}
	return roms[8];
}

void pageram0(void)
// select standard ram frame, then continue with 'variable ram' paging
{
	switch (hmode) {
	case hm_48k:
	case hm_48kif1:
	case hm_48kmgt:
		mempage(1,8);
		mempage(2,4);
		mempage(3,5);
		break;
	case hm_128k:
	case hm_128kif1:
	case hm_128kmgt:
		mempage(1,8);
		mempage(2,5);
		break;
	case hm_samram:
		mempage(1,8);
		break;
	}
	pageram();
}

void pageram(void)
{
	switch (hmode) {
	case hm_48k:
	case hm_48kif1:
	case hm_48kmgt:
		break;
	case hm_128k:
	case hm_128kif1:
	case hm_128kmgt:
		mempage(3,3+(state.hstate&7));
		break;
	case hm_samram:
		if (state.hstate & 0x20) {
			mempage(2,6);
			mempage(3,7);
		} else {
			mempage(2,4);
			mempage(3,5);
		}
		break;
	}
}


void rst08(void)
// is also called when executing INC HL at #1708
{
	if (z80header.pc==0x1708) {
		z80header.hl++;										// inc hl
		z80header.pc++;
		rreg-=(0x060000L-1);									// 6 T states
	} else {
		z80header.hl=*((WORD*)&SpecMem[0x5c5d]);        // ld hl,(ch-add)
		z80header.pc+=3;
		rreg-=(0x100000L-1);                            // 16 T states
	}
	if ((hmode==hm_48k)||(hmode==hm_128k)) return;
	if ((hmode==hm_samram)&&(state.hstate&0x10)) return;  // if1 disable
	if (memstate.diskifpaged) return;
	memstate.diskifpaged=TRUE;
	pagerom();
}

void ret0700(void)
{
	z80header.pc=*((WORD*)&SpecMem[z80header.sp]);  // ret
	z80header.sp+=2;
	rreg-=(0xa0000L-1);                             // 10 T states
	if (memstate.diskifpaged) {
		memstate.diskifpaged=FALSE;
		pagerom();
	}
}

void out31samram(BYTE b)
{
	if (state.hstate & 0x04) return;     // locked
	b&=0x0F;
	if (b&1)
		state.hstate|=(1<<(b>>1));
	else
		state.hstate&=-(1+(1<<(b>>1)));
	if (((b>>1)==1)||((b>>1)==3))        // samram enable or rom select?
		pagerom();
	if ((b>>1)==5)                       // 32k ram bank select?
		pageram();
}

void out7ffd128(BYTE b)
{
	 BYTE c=state.hstate^b;
	 if (hmode < hm_128k) return;
	 if (state.hstate & 0x20) return;    // locked
	 state.hstate=b;
	 if (c & 0x07) pageram();
	 if (c & 0x10) pagerom();
	 if (c & 0x08) {
		  videopage7=!!(b&0x08);
		  if ((videopage7!=page7locked)&&(!useVz80d)) {
				if (videopage7) {
					 void *p = GlobalLock(pages[10]);
					 page7seg = FP_SEG(p);
					 page7ptr = FP_OFF(p);
					 page7fp = p;
					 page7locked = TRUE;
				} else {
					 GlobalUnlock(pages[10]);
					 page7locked = FALSE;
				}
		  }
	 }
}

void outfffd128(BYTE b)
{
	 fffdstate=b;
}

//// Next routine is done in INOUT.ASM by AYout(BYTE)
//
//void outbffd128(BYTE b)
//{
//	 soundregs[fffdstate&0x0F]=b;
//}

BYTE infffd128(void)
{
	 return(soundregs[fffdstate&0x0F]);
}


void ldobhla(void)
// ed fb
// emulates a LD (HL),A but with reference to SamRam's shadow 32K ram bank
{
	int hpag;
	rreg-=(0x40000L-1);                 // 4 T states
	z80header.pc+=2;
	if (hmode!=hm_samram) return;
	if (z80header.hl<0x4000) return;
	if (z80header.hl<0x8000) {
		SpecMem[z80header.hl]=z80header.a[0];
	} else {
		if (z80header.hl<0xc000) {
			if (state.hstate&0x20)
				hpag=4;
			else
				hpag=6;
		} else {
			if (state.hstate&0x20)
				hpag=5;
			else
				hpag=7;
		}
		if (useVz80d)
			vz80d_pokevalue(hVz80d,hpag*0x4000L + (z80header.hl & 0x3fff),z80header.a[0]);
		else {
			((BYTE*)GlobalLock(pages[hpag]))[z80header.hl & 0x3fff]=z80header.a[0];
			GlobalUnlock(pages[hpag]);
		}
	}
}

void ldobahl(void)
// ed fa
// emulates a LD A,(HL) but with reference to SamRam's shadow 32K ram bank
{
	int hpag;
	rreg-=(0x40000L-1);                 // 4 T states
	z80header.pc+=2;
	if (hmode!=hm_samram) return;
	if (z80header.hl<0x8000) {
		z80header.a[0]=SpecMem[z80header.hl];
	} else {
		if (z80header.hl<0xc000) {
			if (state.hstate&0x20)
				hpag=4;
			else
				hpag=6;
		} else {
			if (state.hstate&0x20)
				hpag=5;
			else
				hpag=7;
		}
		if (useVz80d)
			z80header.a[0]=vz80d_peekvalue(hVz80d,hpag * 0x4000L + (z80header.hl & 0x3fff));
		else {
			z80header.a[0]=((BYTE*)GlobalLock(pages[hpag]))[z80header.hl & 0x3fff];
			GlobalUnlock(pages[hpag]);
		}
	}
}

void edf9(void)
// shortcut for quick printing in SamRom
{
	BYTE *pag;
	rreg-=(0x40000L-1);                 // 4 T states
	z80header.pc+=2;
	if (hmode!=hm_samram) return;
	if (z80header.de>0x4000) {
		z80header.a[0]=SpecMem[z80header.de];        // ld a,(de)
	} else {
		if (useVz80d)
			z80header.a[0]=vz80d_peekvalue(hVz80d,0x8000L+z80header.de);
		else {
			pag=GlobalLock(pages[2]);           // samram rom (basic)
			z80header.a[0]=pag[z80header.de];   // ld a,(de)
			GlobalUnlock(pages[2]);
		}
	}
	if (z80header.hl>=0x4000)
		SpecMem[z80header.hl]=z80header.a[0];     // ld (hl),a
	(*((BYTE*)&(z80header.de)))++;               // inc e
	z80header.hl+=0x100;                         // inc h (but no flags)
}

void ldhla(void)
// ED FE is used in the SamRom
{
	if (z80header.hl<0x4000) return;
	SpecMem[z80header.hl]=z80header.a[0];
}

BOOL CALLBACK HardwareProc(HWND hDlg, WORD wMess, WORD wPar, LONG lPar)
{
	static HGLOBAL hrd;
	int computer,diskif,hm;
	switch (wMess) {
	case WM_INITDIALOG:
		hrd=0;
		computer=CH_48K;
		diskif=CH_NONE;
		switch (hmode) {
		case hm_48k:                                       break;
		case hm_128k:    computer=CH_128K;                 break;
		case hm_48kif1:  diskif=CH_IF1;                    break;
		case hm_128kif1: diskif=CH_IF1; computer=CH_128K;  break;
		case hm_48kmgt:  diskif=CH_IF1;                    break;
		case hm_128kmgt: diskif=CH_IF1; computer=CH_128K;  break;
		case hm_samram:  diskif=CH_IF1; computer=CH_SAMRAM; break;
		}
		CheckRadioButton(hDlg,CH_48K,CH_SAMRAM,computer);
		CheckRadioButton(hDlg,CH_NONE,CH_IF1,diskif);
		CheckDlgButton(hDlg,CH_MULTIFACE,state.multifaceemulated);
		CheckDlgButton(hDlg,CH_CURRAH,state.currahemulated);
		CheckDlgButton(hDlg,CH_SPECDRUM,state.specdrumemu);
		if (computer==CH_SAMRAM) {
			CheckDlgButton(hDlg,CH_MULTIFACE,0);
			CheckDlgButton(hDlg,CH_AY,0);
		}
		if (computer!=CH_48K) {
			CheckDlgButton(hDlg,CH_CURRAH,0);
		}
		if (computer==CH_128K) {
			CheckDlgButton(hDlg,CH_AY,1);
		}
		if (computer==CH_48K) {
			CheckDlgButton(hDlg,CH_AY,state.ayemu48k);
		}
		CheckDlgButton(hDlg,CH_RESET,state.resetathchange);
		break;
	case WM_CLOSE:
		hrd=RepaintData(hDlg);
		DestroyWindow(hDlg);
		return 0;
	case WM_DESTROY:
		hHardwareDialog=0;
		PostMessage(hWndMain,IK_FREELPFN,hrd,(LONG)lpfnHardwareProc);
		break;
	case WM_COMMAND:
		switch (wPar) {
		case CH_RESET:
			state.resetathchange=!state.resetathchange;
			SendMessage(hDlg,WM_INITDIALOG,0,0);
			break;
		case CH_MULTIFACE:
			if (hmode!=hm_samram) {
				state.multifaceemulated=!state.multifaceemulated;
				select_secondarymode();
				if (state.resetathchange) Reset();
			} else {
				MessageBeep(-1);
			}
			SendMessage(hDlg,WM_INITDIALOG,0,0);
			break;
		case CH_CURRAH:
			if ((hmode==hm_48k)||(hmode==hm_48kif1)) {
				state.currahemulated = !state.currahemulated;
				select_secondarymode();
				if (state.resetathchange) Reset();
			} else {
				MessageBeep(-1);
			}
			SendMessage(hDlg,WM_INITDIALOG,0,0);
			break;
		case CH_AY:
			if ((hmode==hm_48k)||(hmode==hm_48kif1)) {
				state.ayemu48k = !state.ayemu48k;
				select_secondarymode();
			} else {
				MessageBeep(-1);
			}
			SendMessage(hDlg,WM_INITDIALOG,0,0);
			break;
		case CH_SPECDRUM:
			state.specdrumemu = !state.specdrumemu;
			select_secondarymode();
			SendMessage(hDlg,WM_INITDIALOG,0,0);
			break;
		case CH_48K:
			if ((hmode==hm_128kif1)||(hmode==hm_48kif1))
				hm=hm_48kif1;
			else if ((hmode==hm_128kmgt)||(hmode==hm_48kmgt))
				hm=hm_48kmgt;
			else
				hm=hm_48k;
			select_hmode(hm);
			SendMessage(hDlg,WM_INITDIALOG,0,0);
			if (state.resetathchange) Reset();
			SetActiveWindow(hWndMain);
			break;
		case CH_128K:
			switchoff_currah();
			if ((hmode==hm_48kif1)||(hmode==hm_128kif1))
				hm=hm_128kif1;
			else if ((hmode==hm_48kmgt)||(hmode==hm_128kmgt))
				hm=hm_128kmgt;
			else
				hm=hm_128k;
			select_hmode(hm);
			SendMessage(hDlg,WM_INITDIALOG,0,0);
			if (state.resetathchange) Reset();
			SetActiveWindow(hWndMain);
			break;
		case CH_SAMRAM:
			switchoff_multiface();
			switchoff_currah();
			state.multifaceemulated=FALSE;
			select_hmode(hm_samram);
			if (state.resetathchange) Reset();
			SendMessage(hDlg,WM_INITDIALOG,0,0);
			SetActiveWindow(hWndMain);
			break;
		case CH_NONE:
		case CH_IF1:
			switch (hmode) {
			case hm_48k:
			case hm_48kif1:
			case hm_48kmgt:
				if (wPar==CH_NONE)
					hm=hm_48k;
				else
					hm=hm_48kif1;
				break;
			case hm_128k:
			case hm_128kif1:
			case hm_128kmgt:
				if (wPar==CH_NONE)
					hm=hm_128k;
				else
					hm=hm_128kif1;
				break;
			case hm_samram:
				if (wPar==CH_NONE)
					MessageBeep(-1);
				return TRUE;
			}
			select_hmode(hm);
			if (state.resetathchange) Reset();
			SendMessage(hDlg,WM_INITDIALOG,0,0);
			SetActiveWindow(hWndMain);
			break;
		}
		return TRUE;
	}
	return MyDlgProc(hDlg,wMess,wPar,lPar);
}

void switchoff_multiface(void)
{
	state.multifacepaged=FALSE;
	pagerom();
}

void switchon_multiface(void)
{
	if (state.multifaceemulated) {
		state.multifacepaged=TRUE;
		pagerom();
	}
}

void switchoff_currah(void)
{
	state.currahpaged=FALSE;
	pagerom();
}

void toggle_currah(void)
{
	if ((state.currahemulated)&&
		 ((hmode==hm_48k)||(hmode==hm_48kif1))) {
		 state.currahpaged=!state.currahpaged;
		 pagerom();
	}
}
