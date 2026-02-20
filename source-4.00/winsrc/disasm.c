#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "spectrum.h"
#include "label.h"

// Disassembler for Z80 code

// Some global variables

char	ixflg;		//	0=normal, 1=DD prefixed, 2=FD prefixed
char	usedix;		// if ixflg, 0=opcode didn't change, 1=opcode changed
char	usedixofs;	// if offset was used, this becomes 1
char	ixoffset;	// d in (IX+d)
char	undoc;		// 1=undocumented opcode
char	undocnop;	// 1=undocumented NOP

char	col1[20],col2[20],col3[20],col4[20];

const char regpair1[]="BC\0DE\0HL\0SP\0IX\0IY";
const char regpair2[]="BC\0DE\0HL\0AF\0IX\0IY";
const char flagstr[] ="NZ\0Z\0xNC\0C\0xPO\0PE\0P\0xM";
const char operstr[] ="ADD\0ADC\0SUB\0SBC\0AND\0XOR\0OR\0xCP";

#define flg(f) (flagstr+3*(((f)>>3)&7))

char *regp(char r, char spaf)
// spaf == 1  means  return SP instead of AF
{
	r = (r>>4)&3;
	if (ixflg && (r==2)) {
		usedix = 1;
		r = 3+ixflg;
	}
	return (char*)(spaf?regpair1:regpair2) + 3*r;
}

void reg(char r, char useix, char *str)
// if !useix, H and L are not changed into IXh and IXl resp.
{
	r&=7;
	if ((!ixflg)||(r<4)||(r==7)) {
		strcpy(str,"B\0xxxC\0xxxD\0xxxE\0xxxH\0xxxL\0xxx(HL)\0A\0"+5*r);
		return;
	}
	if (r==6) {
		sprintf(str,"(IX+%.02x)",abs(ixoffset));
		if (ixflg==2) str[2]='Y';
		if (ixoffset<0) str[3]='-';
		usedix=1;
		usedixofs=1;
	} else {
		if (useix) {
			if (r==4)
				strcpy(str,"IXh");
			else
				strcpy(str,"IXl");
			if (ixflg==2) str[1]='Y';
			usedix=1;
			undoc=1;
		} else {
			if (r==4)
				strcpy(str,"H");
			else
				strcpy(str,"L");
		}
	}
}

char *sreg(char r, char useix)
{
	static char temp[10];
	reg(r,useix,temp);
	return temp;
}

BYTE specpeek(WORD addr)
{
	return SpecMem[addr];
}

void specpoke(WORD addr, BYTE b)
{
	pokebyte(SpecMem, addr, b);			// see core.asm and macro.asm
}

int disasm(WORD addr)
{
	BYTE opc = specpeek(addr);
	int length = 1;
	char stre[16], strn[16], strnn[16];
	unsigned int vale,valnn;
	char *s1,*s2;
	BYTE b1,b2;
	char c1;
	char *tijdel;

	// First prepare numeric operand string, for easy reference later
	sprintf(stre,"%.04x",vale=addr+2+(char)specpeek(addr+1));
	sprintf(strn,"%.02x",specpeek(addr+1));
	sprintf(strnn,"%.04x",valnn=specpeek(addr+1)+(((WORD)specpeek(addr+2))<<8));

	#define dis(s1,s2,s3) {strcpy(col1,s1);strcpy(col2,s2);strcpy(col3,s3);}
	#define lble (tijdel=FindAddrLabel(vale),tijdel?tijdel:"")
	#define lblnn (tijdel=FindAddrLabel(valnn),tijdel?tijdel:"")

	switch (opc & 0xC0) {

	case 0:
	////////////////////////////////////////////////////////////////////////////
	// First block.
	////////////////////////////////////////////////////////////////////////////
	switch (opc & 7) {
	case 0:
		if (opc>=16)
			length = 2;
	case 7:
		switch (opc) {
		case 0:	dis("NOP","","");break;
		case 7:	dis("RLCA","","");break;
		case 8:	dis("EX","AF","AF'");break;
		case 15: dis("RRCA","","");break;
		case 16: dis("DJNZ",stre,lble); break;
		case 23: dis("RLA","","");break;
		case 24: dis("JR",stre,lble); break;
		case 31:	dis("RRA","","");break;
		case 32:	dis("JR","NZ",stre); strcpy(col4,lble); break;
		case 39:	dis("DAA","","");break;
		case 40:	dis("JR","Z",stre); strcpy(col4,lble); break;
		case 47:	dis("CPL","","");break;
		case 48:	dis("JR","NC",stre); strcpy(col4,lble); break;
		case 55: dis("SCF","","");break;
		case 56: dis("JR","C",stre); strcpy(col4,lble); break;
		case 63: dis("CCF","","");break;
		}
		return length;
	case 4:
		dis("INC",sreg(opc>>3,1),"");
		return 1;
	case 5:
		dis("DEC",sreg(opc>>3,1),"");
		return 1;
	case 6:
		dis("LD",sreg(opc>>3,1),strn);
		return 2;
	default:
		switch(opc & 0x0f) {
		case 1:
			dis("LD",regp(opc,1),strnn);
			return 3;
		case 9:
			dis("ADD",regp(32,1),regp(opc,1));
			return 1;
		case 2:
		case 10:
			strcpy(col1,"LD");
			if (opc&8) {
				s1=col3; s2=col2;
			} else {
				s1=col2; s2=col3;
			}
			switch (opc&0xF0) {
			case 0:
				strcpy(s1,"(BC)");
				strcpy(s2,"A");
				return 1;
			case 16:
				strcpy(s1,"(DE)");
				strcpy(s2,"A");
				return 1;
			case 32:
				sprintf(s1,"(%s)",strnn);
				strcpy(s2,regp(opc,1));
				return 3;
			case 48:
				sprintf(s1,"(%s)",strnn);
				strcpy(s2,"A");
				return 3;
			}
		case 3:
			dis("INC",regp(opc,1),"");
			return 1;
		case 11:
			dis("DEC",regp(opc,1),"");
			return 1;
		}
	}


	case 0x40:
	////////////////////////////////////////////////////////////////////////////
	// LD r,r block
	////////////////////////////////////////////////////////////////////////////
	if (opc==118) {
		dis("HALT","","");
		return 1;
	}
	b1 = (opc>>3)&7;
	b2 = opc&7;
	c1 = !((b1==6)||(b2==6));		// if (HL) appears, H should not become IXh
	reg(b1,c1,strn);
	reg(b2,c1,strnn);
	dis("LD",strn,strnn);
	return 1;



	case 0x80:
	////////////////////////////////////////////////////////////////////////////
	// 8-bit arithmetic block
	////////////////////////////////////////////////////////////////////////////
	if (((opc&0xF8)==0x90) || (opc>=0xA0)) {
		s1 = col2;
	} else {
		s1 = col3;
		strcpy(col2,"A");
	}
	strcpy(col1,operstr+4*((opc-0x80)>>3));
	reg(opc,1,s1);
	return 1;



	case 0xC0:
	////////////////////////////////////////////////////////////////////////////
	// Miscellaneous, shift opcodes &c
	////////////////////////////////////////////////////////////////////////////
	switch(opc&7) {
	case 0:
		dis("RET",flg(opc),"");
		return 1;
	case 2:
		dis("JP",flg(opc),strnn);
		strcpy(col4,lblnn);
		return 3;
	case 4:
		dis("CALL",flg(opc),strnn);
		strcpy(col4,lblnn);
		return 3;
	case 6:
		dis(operstr+4*((opc&56)>>3), "A", strn);
		if ((opc==0xD6)||(opc>=0xF6)) {
			strcpy(col2,col3);
			col3[0]=0;
		}
		return 2;
	case 7:
		strcpy(col1,"RST");
		sprintf(col2,"#%02X",opc&56);
		return 1;
	}
	switch(opc&15) {
	case 1:
		dis("POP",regp(opc,0),"");
		return 1;
	case 5:
		dis("PUSH",regp(opc,0),"");
		return 1;
	}
	switch(opc) {
	case 0xC3:	dis("JP",strnn,lblnn); return 3;
	case 0xC9:	dis("RET","",""); return 1;
	case 0xCD:	dis("CALL",strnn,lblnn); return 3;
	case 0xD3:
	case 0xDB:
		if (opc==0xD3) {
			strcpy(col1,"OUT");
			strcpy(col3,"A");
			s1=col2;
		} else {
			strcpy(col1,"IN");
			strcpy(col2,"A");
			s1=col3;
		}
		sprintf(s1,"(%s)",strn);
		return 2;
	case 0xD9:	dis("EXX","",""); return 1;
	case 0xE3:	dis("EX","(SP)",regp(32,0)); return 1;
	case 0xE9:	dis("JP",regp(32,0),""); return 1;			// deliberately without ()'s
	case 0xEB:	dis("EX","DE","HL"); return 1;
	case 0xF3:	dis("DI","",""); return 1;
	case 0xF9:	dis("LD","SP",regp(32,0)); return 1;
	case 0xFB:	dis("EI","",""); return 1;
	// now the interesting ones
	case 0xDD:
		ixflg = 1;
		return 1;
	case 0xFD:
		ixflg = 2;
		return 1;
	case 0xCB:
		opc = specpeek(addr+(ixflg?2:1));
		if (opc>=0x40) {
			strcpy(col1,"BIT\0RES\0SET"+4*((opc-64)>>6));
			sprintf(col2,"%u",(opc&56)>>3);
			if (ixflg && ((opc&7)!=6)) {
				// undocumented opcodes like BIT 6,(IX+0),D
				reg(6,0,col4);
				reg(opc,0,col3);
				undoc = 1;
			} else {
				reg(opc,0,col3);
			}
		} else {
			strcpy(col1,"RLC\0RRC\0RL\0.RR\0.SLA\0SRA\0SLL\0SRL\0"+4*(opc>>3));
			if ((opc&56)==48)
				undoc = 1;				// SLL
			if (ixflg && ((opc&7)!=6)) {
				// undocumented opcodes like RL (IX+0),D
				reg(6,0,col3);
				reg(opc,0,col2);
				undoc = 1;
			} else {
				reg(opc,0,col2);
			}
		}
		return 2;
	case 0xED:
		if (ixflg) return 2;
		opc = specpeek(addr+1);
		if ((opc<0x40)||(opc>0x80)) {
			if ((opc & 0xE4) == 0xA0) {
				strcpy(col1,
					"LDI\0.CPI\0.INI\0.OUTI\0LDD\0.CPD\0.IND\0.OUTD\0LDIR\0CPIR\0INIR\0OTIR\0LDDR\0CPDR\0INDR\0OTDR"
					+5*((opc&3) + ((opc>>1)&12) ));
			} else {
				strcpy(col1,"NOP");
				undoc = 1;
			}
			return 2;
		}
		switch(opc&7) {
		case 0:
			dis("IN",sreg(opc>>3,0),"(C)");
			if (opc==0x70) {
				dis("IN","(C)","");
				undoc = 1;
			}
			return 2;
		case 1:
			dis("OUT","(C)",sreg(opc>>3,0));
			if (opc==0x71) {
				strcpy(col3,"0");
				undoc = 1;
			}
			return 2;
		case 2:
			if (opc & 8)
				strcpy(col1,"ADC");
			else
				strcpy(col1,"SBC");
			strcpy(col2,"HL");
			strcpy(col3,regp(opc,1));
			return 2;
		case 3:
			if (opc&8) {
				s1 = col2;
				s2 = col3;
			} else {
				s1 = col3;
				s2 = col2;
			}
			strcpy(col1,"LD");
			sprintf(s2,"(%.04x)",specpeek(addr+2)+(((WORD)specpeek(addr+3))<<8));
			strcpy(s1,regp(opc,1));
			return 4;
		case 4:
			dis("NEG","","");
			if (opc != 0x44)
				undoc = 1;
			return 2;
		case 5:
			dis("RETN","","");
			if (opc & 8)
				col1[3]='I';
			if (opc>0x4D)
				undoc = 1;
			return 2;
		case 6:
			strcpy(col1,"IM");
			strcpy(col2,"0");
			if (opc&16)
				col2[0]++;
			if ((opc&24) == 24)
				col2[0]++;
			if (opc == 0x4E)
				undoc = 1;
			if (opc>0x5E)
				undoc = 1;
			return 2;
		}
		switch (opc) {
		case 0x47:	dis("LD","I","A"); return 2;
		case 0x4F:	dis("LD","R","A"); return 2;
		case 0x57:	dis("LD","A","I"); return 2;
		case 0x5F:	dis("LD","A","R"); return 2;
		case 0x67:	dis("RRD","",""); return 2;
		case 0x6F:	dis("RLD","",""); return 2;
		default:
			undocnop = 1;
			return 2;
		}
	}	// switch(2nd opcode after ED)
	}  // switch(opcode)
	return 0;
}



int Disassemble(WORD addr, BOOL text, char *str)
// str should point to buffer of 80 chars; is filled with zero terminated string.
// if text==TRUE, produces hex & ascii line
// Returns length of opcode (1..4)
// If str!=NULL & !text, first a label is returned if one is defined at current addr
{
	int length;
	int i;
	static WORD labeladdr;
	static BOOL labelreturned=FALSE;
	char *labelstr=NULL;

	col1[0]=col2[0]=col3[0]=col4[0]=0;
	ixflg = 0;
	usedix = 0;
	usedixofs = 0;
	ixoffset = specpeek(addr+2);
	undoc = 0;
	undocnop = 0;

	if (text) {
		length = 8;
		for (i=0;i<length;i++) {
			BYTE b=specpeek(addr+i);
			b &= 0x7F;
			if (b<0x20) b='.';
			sprintf(col1+i,"%c",b);
		}
	} else {
		if ((str!=NULL) && ((!labelreturned) || (labeladdr != addr))) {
			labelstr = FindAddrLabel(addr);
			if (labelstr != NULL) {
				labelreturned=TRUE;
				labeladdr=addr;
				length = 0;
			}
		}
		if (!labelstr) {
			labelreturned = FALSE;
			length = disasm(addr);
			if (ixflg) {
				// DD or FD prefix, so try again
				col1[0]=col2[0]=col3[0]=col4[0]=0;
				length = 1+disasm(addr+1);
				if (!usedix) {
					// Prefix was effectively a NOP
					undocnop = 1;
				} else {
					// Add one to length if offset byte was referred to
					length += usedixofs;
				}
			}
			if (undocnop) {
				length = 1;
				undoc = 1;
				col2[0]=col3[0]=col4[0]=0;
				strcpy(col1,"NOP");
			}
		}
	}

	if (str==NULL)
		return length;
	for (i=0;i<80;i++)
		str[i]=' ';
	if (!labelstr)
		sprintf(str,"%.04x",addr);
	for (i=0;i<length;i++)
		sprintf(str+(text?3:2)*i+5,"%.02x ",specpeek(addr+i));
	if (undoc)
		str[13]='*';
	if (text)
		sprintf(str+30,"%s",col1);
	else if (labelstr)
		sprintf(str+2,"%s:",labelstr);
	else
		sprintf(str+14,"%-5s%s,%s,%s",col1,col2,col3,col4);
	for (i=0;i<80;i++)
		if (!str[i])
			str[i]=' ';
	for (i=79;(str[i]==' ')||(str[i]==',');i--)
		str[i]=' ';
	str[40]=0;
	return length;
}


