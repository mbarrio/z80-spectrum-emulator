typedef unsigned int WORD;
typedef unsigned char BYTE;
typedef unsigned long DWORD;

#define ywmax 16
// copied from xtra.c

extern WORD far vochandle;
extern WORD far vocbuflen;
extern WORD far vocdatalen;
extern BYTE far *vocbuffer;
extern WORD far srate;
extern BYTE far vocplay;
extern WORD far vocdatapoint;
extern WORD far voctoreadhi;
extern WORD far voctoread;
extern WORD far lastthi, lasttlo, tquarter;

extern int far          read_file(int handle, char *pointer, int bytes);
extern int far          lseek(int,unsigned int*,unsigned int*,char);
extern unsigned int far open_file(char *name, char access_code);
extern int far          close_file(int handle);
extern int far          getkey(void);
extern void far         pauze(void);
extern int far          chdir(char*);

extern void             prwind(void);
extern void             clearwindow(void);
extern void             prat(int,int,char*);
extern void             prgetal(unsigned long,int);
extern void             prstr(char *);
extern void             prw(char ch);
extern void             browsetapefile(char);

extern char rommod;
extern char xwmax2,ywmax2;
extern char xcurs;
extern char screenbagger;
extern char updvid;
extern char defdir[],rtapdir[];
extern char tape_pause;

BYTE    tzx_curblock;       // current block type; 0=none
long    TapeInFilePos;      // offset of current block in tzx file; -1 if unknown
long    TapeInBlockPos;     // # of current block in tzx file
char    gszTzxFile[129];    // name of current tzx file
struct {
	WORD pilot;		// pilot pulse len
	WORD sync1;		// sync1 pulse len
	WORD sync2;		// sync2 pulse len
	WORD zero;		// zero bit pulse len
	WORD one;		// one bit pulse len
	WORD pilotlen;	// # pilot pulses
	BYTE blb;		// # bits in last byte
	WORD pause;		// pause after data in ms
	DWORD datlen;	// (3-byte) length of data
}	tzx_datahdr;
char	tzx_out;			// current output level
BYTE	tzx_outtype;	// 0=pure tone 1=pilot 2,3=sync1,2 4=data 5=pause 6=pulse 7=dirrec
WORD	tzx_numpulses;	// pulses in pure tone or pulse block
long	tzx_bytesleft;	// bytes left in data block or direct recording block

WORD TzxForCounter;
WORD TzxForPos;
WORD TzxForBlk;
WORD TzxForLooping;
WORD TzxCallPos;
WORD TzxCallBlk;
WORD TzxReturnAddress;
int  hInFile=0;

#define min(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)>(b)?(a):(b))
#define abs(a) ((a)>0?(a):(-a))

long _llseek(int handle, long pos, char type)
{
    unsigned int pos0,pos1;
    pos0 = pos;
    pos1 = (pos>>16);
    lseek(handle, &pos0, &pos1, type);
    return (long)( (WORD)pos0 + (((DWORD)((WORD)pos1))<<16) );
}

long BlockLen(int tapefile)
// Returns length of block without length word for .TAP files
// Returns length of block including type byte, for .TZX files
// Returns -2 in error
{
	long len=0;
	long lenofs=0,lenlen=0,lenadd=0;
    BYTE blocktype;
	long pos=_llseek(tapefile,0,1);		// move 0 bytes from HERE, i.e. get pos

    if (read_file(tapefile,&blocktype,1)!=1) {
        len=-2;                             // signal 'error' or EOF
        _llseek(tapefile,pos,0);
        return(len);
    }
    switch (blocktype) {
    case 0x10:  lenofs=2;   lenlen=2;   lenadd=4;   break;
    case 0x11:  lenofs=0xf; lenlen=3;   lenadd=0x12;break;
    case 0x12:                                  lenadd=4;   break;
    case 0x13:  lenofs=0;   lenlen=1;   lenadd=1;   break;  // times 2
    case 0x14:  lenofs=7;   lenlen=3;   lenadd=0xa; break;
    case 0x15:  lenofs=5;   lenlen=3;   lenadd=8;   break;
    case 0x20:                                  lenadd=2;   break;
    case 0x21:  lenofs=0;   lenlen=1;   lenadd=1;   break;
    case 0x22:                                  lenadd=0;   break;
    case 0x23:                                  lenadd=2;   break;
    case 0x24:                          lenadd=2;   break;
    case 0x25:                                  lenadd=0;   break;
    case 0x26:  lenofs=0;   lenlen=2;   lenadd=2;   break;  // times 2
    case 0x27:                                  lenadd=0;   break;
    case 0x28:  lenofs=0;   lenlen=2;   lenadd=2;   break;
    case 0x30:  lenofs=0;   lenlen=1;   lenadd=1;   break;
    case 0x31:  lenofs=1;   lenlen=1;   lenadd=2;   break;
    case 0x32:  lenofs=0;   lenlen=2;   lenadd=2;   break;
    case 0x33:  lenofs=0;   lenlen=1;   lenadd=1;   break;  // times 3
    case 0x34:                                  lenadd=8;   break;
    case 0x35:  lenofs=0x10;   lenlen=4;   lenadd=0x14;break;
    case 0x40:  lenofs=1;   lenlen=3;   lenadd=4;   break;
    case 0x5a:                                  lenadd=9;   break;
    default:        lenofs=0;   lenlen=4;   lenadd=4;   break;
    }
    _llseek(tapefile,lenofs,1);
    if (lenlen) {
        if (read_file(tapefile,(void*)&len,lenlen)!=lenlen) {
            len=-2;
            _llseek(tapefile,pos,0);
            return(len);
        }
    }
    if ((blocktype==0x13)||(blocktype==0x26)) len*=2;
    if (blocktype==0x33) len*=3;
    len += lenadd + 1;          // +1 to account for type byte


    _llseek(tapefile,pos,0);
	return(len);
}

void SkipBlock(int tapefile)
{
	long len;
	len=BlockLen(tapefile);
    _llseek(tapefile,len,1);        // tzx
}

void WindTape(int tapefile)
{
	int i;
	if (TapeInFilePos!=-1) {
		_llseek(tapefile,TapeInFilePos,0);
		return;
	}
    i=TapeInFilePos=0;
    _llseek(tapefile,TapeInFilePos,0);
    while (i<TapeInBlockPos) {
        TapeInFilePos += BlockLen(tapefile);
        _llseek(tapefile,TapeInFilePos,0);
		i++;
	}
}

int ReadBuffer(BYTE *buf, int handle)
// Support function for browsing.  Reads in relevant data from tzx file
{
    long len=BlockLen(handle);
    long pos=_llseek(handle,0,1);       // move 0 bytes from HERE, i.e. get pos
    buf[0]=-1;                          // sentinel
    if (len==-2) return -1;
    read_file(handle, buf, 1);
    switch (buf[0]) {
        case 0x11:                              // turbo speed
            read_file(handle, buf+1, 14);       // throw away hi-speed data
        case 0x10:                              // ordinary speed
            read_file(handle, buf+1, 23);
            *((WORD*)(buf+1))=
                (WORD)(len-(buf[0]==0x10?7:21));
                                                // because turbo len = 3 bytes
            break;
        case 0x21:                              // group
        case 0x30:                              // text descr
        case 0x31:                              // message
        case 0x35:                              // custom info block
            read_file(handle, buf+1, 27); break;
        case 0x14:
        case 0x15:
            *((long*)(buf+1))=len;break;
    }
    _llseek(handle,pos+len,0);                  // skip this block
    return 0;
}


void PrintTzxBuf(BYTE *buf, int ypos)
{
    char *msg;
    char *str=(void*)0L;
    long len=-1;
    prat(3,ypos,"");
    switch (buf[0]) {
        case 0xff:  msg="-End of file-";break;
        case 0x5a:  msg="ZXTape identifier";break;
        case 0x12:  msg="Pure tone";break;
        case 0x13:  msg="Pulse sequence";break;
        case 0x20:  msg="Pause";break;
        case 0x22:  msg="Group end";break;
        case 0x23:  msg="Jump";break;
        case 0x24:  msg="Loop";break;
        case 0x25:  msg="EndLoop";break;
        case 0x26:  msg="Call sequence";break;
        case 0x27:  msg="Return";break;
        case 0x28:  msg="User select block";break;
        case 0x2a:  msg="Stop tape in 48K mode";break;
        case 0x32:  msg="Archive info";break;
        case 0x33:  msg="Hardware type";break;
        case 0x34:  msg="Emulator info";break;
        case 0x40:  msg="Snapshot";break;
        case 0x11:
            prat(3,ypos,"Turbo:");
        case 0x10:
            if ((buf[3]==19)&&(buf[4]==0)&&(buf[5]==0)) {
                str=buf+7;
                str[10]=0;
                switch(buf[6]) {
                    case 0: msg=" Program:";break;
                    case 1: msg=" Nmbr Arr:";break;
                    case 2: msg=" Char Arr:";break;
                    default: msg=" Bytes:";break;
                }
            } else {
                msg = " Data ";
                len = *(WORD*)(buf+1);
            }
            break;
        case 0x14:
            msg="Pure data ";
            len=*(long*)(buf+1);
            break;
        case 0x15:
            msg="Sampled data ";
            len=*(long*)(buf+1);
            break;
        case 0x21:
            msg="Group:";
            goto getname;
        case 0x35:
            msg="Custom info:\"";
            str=buf+1;
            *(int*)(&str[0x10])=34;
            break;
        case 0x30:
            msg="Descr.:";
            getname:
            str=buf+2;
            buf[27]=0;
            if (buf[1]<25) str[buf[1]]=0;
            break;
        case 0x31:
            msg="Msg:";
            str=buf+3;
            buf[27]=0;
            if (buf[2]<24) str[buf[2]]=0;
            break;
        default:
            msg="!Unknown block type!";
            break;
    }
    prstr(msg);
    if (str) {
        int i;
        for (i=0;str[i] && (xcurs < xwmax2-1);i++) {
            if (str[i]==13)
                str[i]=' ';
            prw(str[i]);
        }
    }
    if (len!=-1) {
        prw('(');
        prgetal(len,1);
        prw(')');
    }
}



void TzxMessage(char *msg,int duration)
{
    int i;
    ywmax2 = 14;
    clearwindow();
    if (duration != -1) {
        prat(1,1,"TZX message:");
    } else {
        prat(1,1,"TZX status message:");
    }
    prat(1,3,msg);
    if (duration <= 0)
        prat(26,ywmax2-2,"key..");         /* HMc MOD */
    screenbagger=1;
    updvid=1;
    prwind();
    do{}while(getkey());
    if (duration>0) {
        while (duration>0) {
            for (i=0;i<18;i++) {
                pauze();
                if (getkey()) {
                    i=18;
                    duration=0;
                }
            }
        }
    } else {
        do{}while(!getkey());
    }
    ywmax2 = ywmax;
}


char read_file_check(int handle,void *buf,WORD len)
// reads bytes, returns TRUE if error, else updates TapeInFilePos
{
    WORD w;
    w = read_file(handle,buf,len);
    if (w != len) return 1;
    TapeInFilePos += w;
    return 0;
}


void CloseTzxFile(void)
{
    if (hInFile) {
        close_file(hInFile);
        hInFile=0;
    }
}


int OpenTzxFile(void)
{
    if (!hInFile) {
        chdir(rtapdir);
        hInFile=open_file(gszTzxFile,0);
        chdir(defdir);
    }
    return hInFile;
}


void readtzxfile(skip)
long skip;
{
	// Reads part of TZX file, and puts result in sample bit buffer.
	// skip is ignored.
    long timediff;
	DWORD d;
    WORD w1,w;
    int i,j;
	int b0,b1,p0,q0,p1,q1;
    char msg[256];
    long bitpos=skip;
	long bytepos,bytesread;

	// Algorithm:
    // 1. update time base of sample (voc) buffer
	// 2. read portion of TZX file, and store in sound bit buffer
	// 3. repeat until time of last sample in sound bit buffer is > current time

    // At entry, lasttlo and lastthi correspond to first new byte in buffer.
    // No unused bytes are left.

    if (!vocplay) return;       // should not be possible, but bbsts
    if (vochandle) return;      // same

    // assembly already updated _lastt(hi/lo) for first bit of new block, as if
    // sample rate stays the same.  So first reclaim some time
    // But not for first block

    if (TapeInBlockPos) {
//          d = (long)lasttlo + (long) srate + (long)tquarter;
//          lasttlo = (d % tquarter);
//          lastthi -= ((d / tquarter)-1);
    }
    OpenTzxFile();
    vocdatalen = 0;
	do {
		if (TapeInFilePos==-1) {		// first seek current block if necessary
			WindTape(hInFile);
			tzx_curblock=0;
            tzx_out = 0;            // set current output level to 'low'
		} else {
			_llseek(hInFile,TapeInFilePos,0);
		}
		if (!tzx_curblock) {		// no current block; load block type byte
            if (read_file_check(hInFile,&tzx_curblock,1)) {
				// end of TZX file
                CloseTzxFile();
                vocplay=0;
                if (tape_pause) TzxMessage("TZX file played.",-1);
				return;
			}
			if (TapeInFilePos==0) {	// first block?
                tzx_out = 0;        // set current output level to 'low'
			}
			switch (tzx_curblock) {
			case 0x10:		// ordinary block
				tzx_datahdr.pilot = 2168;
				tzx_datahdr.sync1 = 667;
				tzx_datahdr.sync2 = 735;
				tzx_datahdr.zero = 855;
				tzx_datahdr.one = 1710;
				tzx_datahdr.pilotlen = 8064;		// short is 3220
				tzx_datahdr.blb = 8;
				tzx_datahdr.pause = 1000;
				tzx_datahdr.datlen = 0;
                if (read_file_check(hInFile,&(tzx_datahdr.pause),5)) {
					tzx_err:
                    vocplay=0;
                    CloseTzxFile();
                    TzxMessage("Error reading TZX file.",-1);
					return;
				}
                TapeInFilePos--;        // First data byte has been loaded also
				if (tzx_datahdr.datlen & 0x800000L) {	// not header but data
					tzx_datahdr.pilotlen = 3220;
				}
				tzx_bytesleft = tzx_datahdr.datlen & 0xFFFFL;
				tzx_outtype = 1;
				break;
			case 0x11:		// turbo block
				tzx_datahdr.datlen = 0;
                if (read_file_check(hInFile,&(tzx_datahdr.pilot),18))
					goto tzx_err;
				tzx_bytesleft = tzx_datahdr.datlen;
				tzx_outtype = 1;
				break;
			case 0x12:		// pure tone
                if (read_file_check(hInFile,&(tzx_datahdr.pilot),4))
					goto tzx_err;
				tzx_datahdr.pilotlen = tzx_datahdr.sync1;
				tzx_outtype = 0;
				tzx_bytesleft = 0;
				break;
			case 0x13:		// pulse train
				tzx_numpulses=0;
                if (read_file_check(hInFile,&tzx_numpulses,1))
					goto tzx_err;
				tzx_bytesleft = 2*tzx_numpulses;
				tzx_outtype = 6;
				break;
			case 0x14:		// pure data
				tzx_datahdr.datlen = 0;
                if (read_file_check(hInFile,&(tzx_datahdr.one),10))
					goto tzx_err;
				tzx_datahdr.zero = tzx_datahdr.one;		// shift down because of pilotlen
				tzx_datahdr.one = tzx_datahdr.pilotlen;
				tzx_bytesleft = tzx_datahdr.datlen;
				tzx_outtype = 4;
				break;
			case 0x15:		// direct recording
				tzx_datahdr.datlen = 0;
                if (read_file_check(hInFile,&(tzx_datahdr.one),5))
					goto tzx_err;
                if (read_file_check(hInFile,&(tzx_datahdr.datlen),3))
					goto tzx_err;
				tzx_datahdr.pause = tzx_datahdr.pilotlen;
				// 'one' contains the sample rate, in T's per bit.
				tzx_bytesleft = tzx_datahdr.datlen;
				tzx_outtype = 7;
				break;
			case 0x20:		// pause
                if (read_file_check(hInFile,&(tzx_datahdr.pause),2))
					goto tzx_err;
				tzx_bytesleft = 0;
				tzx_outtype = 5;
				if (!tzx_datahdr.pause) {
                      TzxMessage("TZX file paused.",-1);
                      vocplay=0;
                      return;
				}
				break;
			case 0x23:		// relative jump
                if (read_file_check(hInFile,&i,2))
					goto tzx_err;
                TapeInFilePos = -1;
				TapeInBlockPos += i-1;		// -1 compensates ++ below as _curblock=0
				tzx_curblock = 0;
				tzx_bytesleft = 0;
				break;
			case 0x24:		// FOR loop
                if (read_file_check(hInFile,&TzxForCounter,2))
					goto tzx_err;
				TzxForPos = TapeInFilePos;
				TzxForBlk = TapeInBlockPos + 1;
				TzxForLooping = 1;
				tzx_curblock = 0;
				tzx_bytesleft = 0;
				break;
			case 0x25:		// NEXT
				TapeInFilePos++;
				if (TzxForLooping) {
					TzxForCounter--;
					if (TzxForCounter) {
						TapeInFilePos = TzxForPos;
						TapeInBlockPos = TzxForBlk - 1;
					} else {
						TzxForLooping=0;
					}
				}
				tzx_curblock = 0;
				tzx_bytesleft = 0;
				break;
			case 0x26:		// CALL sequence
				TzxCallPos = TapeInFilePos;
				TzxCallBlk = TapeInBlockPos;
				TzxReturnAddress = 1;
				// continue into RETURN code
			case 0x27:		// RETURN from CALL
				if (!TzxReturnAddress) {		// no CALL to RETURN to
					TapeInFilePos = _llseek(hInFile,0,2);		// go to end
				} else {
					_llseek(hInFile,TzxCallPos+1,0);				// go to CALL blk
                    if (read_file_check(hInFile,&w1,2))
                        goto tzx_err;
					if (w1 < TzxReturnAddress) {					// no more calls
						TapeInFilePos = TzxCallPos+2*w1+3;
						TapeInBlockPos = TzxCallBlk+1;
						TzxReturnAddress = 0;
					} else {
						_llseek(hInFile, TzxCallPos+2*TzxReturnAddress+1, 0);
                        read_file(hInFile,(void*)&TapeInBlockPos,2);
						TapeInFilePos = -1;
						TzxReturnAddress++;
					}
				}
				tzx_curblock = 0;
				tzx_bytesleft = 0;
				break;
            case 0x31:      // message block
                if (read_file_check(hInFile,&w1,2))
                    goto tzx_err;
                if (read_file_check(hInFile,msg,w1>>8))
                    goto tzx_err;
                msg[w1>>8]=0;
                TzxMessage(msg,w1 & 0xff);
                tzx_curblock = 0;
                break;
            case 0x28:      // 'select' block
                tzx_bytesleft = 0;
                tzx_outtype = 5;                      // pause
                tzx_datahdr.pause = 100;              // ms
                TapeInFilePos = -1;
                i=--TapeInBlockPos;
                browsetapefile(2);
                if (i==TapeInBlockPos)
                    TapeInBlockPos+=2;           // continue if no selection
                break;
            case 0x2a:
                if (rommod<=3) {
                    tzx_outtype = 5;
                    tzx_datahdr.pause = 100;
                    vocplay = 0;
                    goto skipthis;
                }
                // continue into default
			default:			// all other blocks are skipped
                tzx_curblock = 0;                       // set 'no current block, load new'
                skipthis:
				TapeInFilePos--;							// back up to type byte
				_llseek(hInFile,TapeInFilePos,0);		// set file pos
				TapeInFilePos += BlockLen(hInFile);	// skip current block
				tzx_bytesleft = 0;
				break;
			}
		}

        // If (tzx_curblock), translate current block into buffer.
		// If not, just continue into next

        if (tzx_curblock) {
			switch (tzx_outtype) {
			case 0:	// pure tone
			case 1:	// pilot tone

                w = tzx_datahdr.pilot;

                syncpulse:

                i = (w+36)/95;                      // samples per pulse
                srate = 95;

                // compute # of pulses to play now -- include factor i later

                vocdatalen = min(tzx_datahdr.pilotlen, vocbuflen/i);

                // compute new pilotlen

                tzx_datahdr.pilotlen -= vocdatalen;
                if (!tzx_datahdr.pilotlen) {
                    if (tzx_outtype==0) {           // pure tone
						tzx_curblock = 0;
                    } else {                        // pilot tone, or sync
						tzx_outtype++;
					}
                }

                // fill buffer

                if (tzx_out) {
                    b0 = 0xf0;      // play high pulse first
                } else {
                    b0 = 0x10;
                }
                w1 = 0;
                for (j=0;j<vocdatalen;j++) {
                    for (w=0;w<i;w++)
                        ((BYTE*)vocbuffer)[w1++] = b0;
                    b0 ^= 0xe0;
                }

                // compute new tzx_out

                if (vocdatalen & 1)
                    tzx_out = !tzx_out;

                // set correct value of vocdatalen

                vocdatalen *= i;
                break;

//                if (tzx_out)
//                    d = 0x10f0;         // was 55555555L; lsb played first, high
//                else
//                    d = 0xf010;         // was AAAAAAAAL; lsb played first, low
//                // compute # of bytes to play now
//                vocdatalen = min(tzx_datahdr.pilotlen, vocbuflen);
//                // set sample rate
//                srate = tzx_datahdr.pilot;
//                // compute new tzx_out
//                if (vocdatalen & 1)
//                    tzx_out = !tzx_out;
//                // compute # of pulses left, new block if none
//                tzx_datahdr.pilotlen -= vocdatalen;
//                if (!tzx_datahdr.pilotlen) {
//                    if (tzx_outtype==0) {   // pure tone
//                        tzx_curblock = 0;
//                    } else {                        // pilot tone
//                        tzx_outtype++;
//                    }
//                }
//                // actually fill the sound bit buffer
//               for (i=0;i<(vocdatalen+1)/2;i++)
//                    ((WORD*)vocbuffer)[i] = d;
//               break;
			case 2:	// sync1
			case 3:	// sync2
                  if (tzx_outtype==2)
                    w = tzx_datahdr.sync1;
                  else
                    w = tzx_datahdr.sync2;

                  onepulse:

                  tzx_datahdr.pilotlen = 1;
                  goto syncpulse;

//                vocbuffer[0] = (tzx_out?0xf0:0x10);
//                tzx_out = !tzx_out;
//                if (tzx_outtype==2)
//                    srate = tzx_datahdr.sync1;
//                else
//                    srate = tzx_datahdr.sync2;
//                vocdatalen = 1;
//                tzx_outtype++;
//                break;
			case 4:	// data
                  p0 = (tzx_datahdr.zero + 36)/95;
                  p1 = (tzx_datahdr.one  + 36)/95;
                  srate = 95;

                  // compute how many bytes to load

                  bytesread = min(tzx_bytesleft, (vocbuflen-1)/(2*8*max(p0,p1)));
                  w1 = vocbuflen-bytesread;
                  if (read_file_check(hInFile, vocbuffer+w1, bytesread))
                      goto tzx_err;
                  tzx_bytesleft -= bytesread;

                  // translate bytes into samples

                  if (tzx_out)
                    b0 = 0xf0;               // signal value first pulse
                  else
                    b0 = 0x10;
                  b1 = b0 ^ 0xe0;
                  d = 8*(long)bytesread;
                  if (!tzx_bytesleft) d-=(8-tzx_datahdr.blb);   // # bits to translate
                  bitpos = 0;                                   // output ptr
                  bytepos = w1;                                 // input byte ptr
                  i = 0x80;                                     // input bit ptr
                  while (d) {
                    if (((BYTE*)vocbuffer)[bytepos] & i)
                        q0 = p1;        // duration 1 bit
                    else
                        q0 = p0;        // duration 0 bit
                    for (j=0;j<q0;j++) {
                        ((BYTE*)vocbuffer)[bitpos] = b0;
                        ((BYTE*)vocbuffer)[q0+bitpos++] = b1;
                    }
                    bitpos += q0;
                    d--;
                    i/=2;
                    if (!i) {
                        bytepos++;
                        i=0x80;
                    }
                  }

                  if (!tzx_bytesleft) {
                    tzx_outtype++;
                    // If TZX file ends with zero pause, then state of EAR port is
                    // undefined and final edge may not be produced, according to
                    // TZX specification.  However some TZX files depend on the EAR
                    // going low (ghould'n'ghosts, game over 2, winter games).  So
                    // here's a patch.  It is not foolproof.
                    i=0;
                    if (!read_file(hInFile, (void*)&i, 1) || (i==0x22)) {
                        // no more blocks, or 'group end' block
                        if (!tzx_datahdr.pause)
                            tzx_datahdr.pause = 1;      // add 1ms pause
                    }
                    // Next fix:
                    // If last pulse is 'low', and there is some pause, add a final
                    // short 'high' pulse to make sure final edge is present.  This
                    // is NOT according to TZX specification, but some .TZX files need
                    // it.
                    if (tzx_out && tzx_datahdr.pause) {
                        for (i=0;i<p0+p1;i++) {
                            ((BYTE*)vocbuffer)[bitpos] = 0xf0;
                            bitpos++;
                        }
                    }
                  }
                  vocdatalen = bitpos;
                  break;


//                // First compute basic time step dt, and integers such that
//                //  p*dt and q*dt are approximations to lengths of bit 0 and 1 (b0, b1)
//                // Constraints: (p+q)*dt = (b0+b1),  |b0-p*dt|<58 T, |b1-q*dt|<58 T
//                //  (best compare time remains same, absolute difference approx. 1 loop
//                //   through standard sample routine)
//                // Algorithm: sort of Euclidian GCD
//                b0 = tzx_datahdr.zero;
//                b1 = tzx_datahdr.one;
//                srate = b0+b1;
//                p0 = 1; q0 = 0;
//                p1 = 0; q1 = 1;
//                #define swap(a,b) a^=b^=a^=b
//                #define sgn(a) (a<0?-1:1)
//                #define mydiv(a,b) (sgn(a)*sgn(b)*(abs(a)+abs(b)/2)/abs(b))
//                while (abs(b0)>=58 * (abs(p0)+abs(q0))) {
//                    i = mydiv(b1,b0);
//                    b1 -= i*b0;
//                    q1 -= i*q0;
//                    p1 -= i*p0;
//                    swap(b0,b1);
//                    swap(p0,p1);
//                    swap(q0,q1);
//                }
//                srate /= (abs(p0)+abs(q0));
//                p0 = abs(p0);
//                q0 = abs(q0);
//                swap(p0,q0);
//                // Now p0 and q0 are multipliers for bit 0 and 1 respectively
//                // Compute how many bytes to load.  Make sure bit buffer is large enough
//                bytesread = min(tzx_bytesleft, (vocbuflen-1)/(2*max(p0,q0)*8));
//                w1 = vocbuflen-bytesread;
//                if (read_file_check(hInFile, vocbuffer+w1, bytesread))
//                    goto tzx_err;
//                // translate data bytes into samples
//                bitpos=0;
//                tzx_bytesleft -= bytesread;
//                for (bytepos=w1;bytepos<vocbuflen-1;bytepos++) {
//                    for (i=7;i>=0;i--) {
//                        if (vocbuffer[bytepos] & (1<<i))
//                            p1 = q0;        // bit 1
//                        else
//                            p1 = p0;        // bit 0
//                        for (q1=0;q1<p1;q1++) {
//                            vocbuffer[bitpos] = (tzx_out?0xf0:0x10);
//                            bitpos++;
//                        }
//                        for (q1=0;q1<p1;q1++) {
//                            vocbuffer[bitpos] = (!tzx_out?0xf0:0x10);
//                            bitpos++;
//                        }
//                    }
//                }
//                for (i=7;i>=(tzx_bytesleft?0:8-tzx_datahdr.blb);i--) {
//                    if (vocbuffer[bytepos] & (1<<i))
//                        p1 = q0;        // bit 1
//                    else
//                        p1 = p0;        // bit 0
//                        for (q1=0;q1<p1;q1++) {
//                            vocbuffer[bitpos] = (tzx_out?0xf0:0x10);
//                            bitpos++;
//                        }
//                        for (q1=0;q1<p1;q1++) {
//                            vocbuffer[bitpos] = (!tzx_out?0xf0:0x10);
//                            bitpos++;
//                        }
//                }
//                if (!tzx_bytesleft) {
//                    tzx_outtype++;
//                    // If TZX file ends with zero pause, then state of EAR port is
//                    // undefined and final edge may not be produced, according to
//                    // TZX specification.  However some TZX files depend on the EAR
//                    // going low (ghould'n'ghosts, game over 2, winter games).  So
//                    // here's a patch.  It is not foolproof.
//                    i=0;
//                    if (!read_file(hInFile, (void*)&i, 1) || (i==0x22)) {
//                        // no more blocks, or 'group end' block
//                        if (!tzx_datahdr.pause)
//                            tzx_datahdr.pause = 1;      // add 1ms pause
//                    }
//                    // Next fix:
//                    // If last pulse is 'low', and there is some pause, add a final
//                    // short 'high' pulse to make sure final edge is present.  This
//                    // is NOT according to TZX specification, but some .TZX files need
//                    // it.
//                    if (tzx_out && tzx_datahdr.pause) {
//                        for (i=0;i<p0+q0;i++) {
//                            vocbuffer[bitpos] = 0xf0;
//                            bitpos++;
//                        }
//                    }
//                }
//                vocdatalen = bitpos;
//                break;
			case 5:	// pause
				if (tzx_datahdr.pause)
                    tzx_out=0;                  // low level
                w1 = vocbuflen/((3500+36)/95);
                if (tzx_datahdr.pause < w1)
                    w1 = tzx_datahdr.pause;
                tzx_datahdr.pause -= w1;
                if (!tzx_datahdr.pause)
                    tzx_curblock = 0;
                w1 *= (3500+36)/95;
                for (w=0;w<w1;w++)
                    vocbuffer[w] = 0x10;
                vocdatalen = w1;
                srate = 95;
				break;
			case 6:	// pulse
                w1 = 0;
                do {
                    if (read_file_check(hInFile,&w,2))
                        goto tzx_err;
                    i = (w+36)/95;
                    if (w1 + (WORD)i > vocbuflen) {
                        TapeInFilePos -= 2;     // back-up 2 bytes
                        break;
                    }
                    tzx_bytesleft -= 2;
                    if (tzx_out) {
                        b0 = 0xf0;      // play high pulse first
                    } else {
                        b0 = 0x10;
                    }
                    while (i--)
                        vocbuffer[w1++] = b0;
                    tzx_out = !tzx_out;
                } while (tzx_bytesleft);

                if (!tzx_bytesleft)
                    tzx_curblock = 0;       // no more pulses? Then stop.

                srate = 95;
                vocdatalen = w1;
                break;

//                do {
//                    if (read_file_check(hInFile,&(tzx_datahdr.pilot),2))
//                        goto tzx_err;
//                    tzx_bytesleft -= 2;
//                    vocbuffer[0] = (tzx_out?0xf0:0x10);
//                    tzx_out = !tzx_out;
//                    srate = tzx_datahdr.pilot;
//                    vocdatalen = 1;
//                    if (!tzx_bytesleft)         // no more pulses? Then stop.
//                        tzx_curblock=0;
//                } while ( (tzx_curblock && !srate) );       // Necessary for levia48.tzx
//                if (!srate) {
//                    srate = 100;
//                    vocdatalen = 0;
//                }
//                break;

			case 7:	// direct recording, then pause
                bytesread = min(vocbuflen/8, tzx_bytesleft);
				tzx_bytesleft -= bytesread;
                w1 = vocbuflen - bytesread;
                if (read_file_check(hInFile,vocbuffer+w1,bytesread))
					goto tzx_err;
                srate = tzx_datahdr.one;        // see init code above
				if (tzx_bytesleft)
                    vocdatalen = 8*bytesread;
				else {
                    vocdatalen = 8*bytesread - 8 + tzx_datahdr.blb;
                    tzx_out = !!((vocbuffer[vocbuflen-1]<<(tzx_datahdr.blb-1))&0x80);
					tzx_outtype = 5;					// pause
				}
                for (w=0;w<vocdatalen;w++) {
                    vocbuffer[w] = (((vocbuffer[w1+(w/8)]<<(w%8))&0x80) ? 0xf0 : 0x10);
				}
				break;
			}
		}
		if (!tzx_curblock) {		// i.e. block finished
            TapeInBlockPos++;       // advance block counter
		}
        // Now set up variables for assembly code
        vocdatapoint = 0;       // first fresh sample
                                // lastt(hi/lo) are still good
                                // srate has been set above
                                // vocdatalen has been set above
        voctoreadhi = 0;
        voctoread = 0;
    } while (vocdatalen == 0);

    // Update time to account for first bit.

//    i = ((DWORD)srate+(DWORD)tquarter)/(DWORD)tquarter;
//    d = lasttlo + i*tquarter - srate;
//    lastthi += i - (d/tquarter);
//    lasttlo = (d % tquarter);
}


