/*
    makeUTF8 

    Converts UTF-16 (BE/LE), UTF-32 (BE/LE), ISO-8859-N to UTF-8.
    Removes BOM and surrogate pairs from UTF-8, converting a
    codepoint between U-D800 and U-DBFF followed by a codepoint
    between U-DC00 and U-DFFF to one valid codepoint > U-FFFF.

    Copyright (C) 2014  Center for Sprogteknologi, University of Copenhagen

    This file is part of makeUTF8.

    makeUTF8 is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    makeUTF8 is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with makeUTF8.  If not, see <http://www.gnu.org/licenses/>.
*/
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <errno.h>
#include "iso2unicode.h"
#ifdef WIN32
#include <io.h>
#include <fcntl.h>
#endif

#define RFC3629compliant 1  // Don't allow codepoints above U-10FFFF
#define NOUCS2 0            // Don't allow codepoints between U-D800 and U-DFFF in output

bool possiblyUTF8 = true;
bool possiblyUTF16LE = true;
bool possiblyUTF16BE = true;
bool possiblyUTF32LE = true;
bool possiblyUTF32BE = true;
bool possiblyISO8859 = true;
bool isUTF16 = false;
bool isUTF32 = false;
bool hasEBBBBF = false;
bool hasFEFF = false;
bool hasFFFE = false;
bool UTF8containsSurrogates = false;
bool UTF8containsOverlongCharacters = false;

bool UnicodeToUtf8(int w,FILE * fo )
    {
#if RFC3629compliant
    if(w > 0x10ffff)
        return false;
#if NOUCS2
    if(0xd800 <= w && w <= 0xdfff)
        return false;
#endif
#endif
    // Convert unicode to utf8
    if ( w < 0x0080 )
        fputc(w,fo);
    else 
        {
        if(w < 0x0800) // 7FF = 1 1111 111111
            {
            fputc(0xc0 | (w >> 6 ),fo);
            }
        else
            {
            if(w < 0x10000) // FFFF = 1111 111111 111111
                {
                fputc(0xe0 | (w >> 12),fo);
                } 
            else // w < 110000
                { // 10000 = 010000 000000 000000, 10ffff = 100 001111 111111 111111
                fputc(0xf0 | (w >> 18),fo);
                fputc(0x80 | ((w >> 12) & 0x3f),fo);
                } 
            fputc(0x80 | ((w >> 6) & 0x3f),fo);
            }
        fputc(0x80 | (w & 0x3f),fo);
        }
    return true;
    }

void surrogate(int s,FILE * fo)
    {    
    static int prev = 0;
    if(prev == 0)
        {
        if((s & 0xFC00) == 0xD800) // first word surrogate
            {
            prev = s;
            }
        else
            {
            UnicodeToUtf8(s,fo);
            }
        }
    else
        {
        if((s & 0xFC00) == 0xDC00) // second word surrogate
            {
            s = (s & 0x3ff) + ((prev & 0x3ff) << 10) + 0x10000;
            UnicodeToUtf8(s,fo);
            }
        else
            {
            // Assume it is UCS-2, not UTF-16
            UnicodeToUtf8(prev,fo);
            UnicodeToUtf8(s,fo);
            }
        prev = 0;
        }
    }

void littleendian(FILE * fi,FILE * fo)
    {
    int kar;
    int s;
    while((kar = fgetc(fi)) != EOF)
        {
        s = kar;
        if((kar = fgetc(fi)) == EOF)
            {
            break;
            }
        s += kar << 8;
        surrogate(s,fo);
        }
    }

bool littleendian32(FILE * fi,FILE * fo)
    {
    int kar;
    while((kar = fgetc(fi)) != EOF)
        {
        int s = kar;
        for(int i = 3;i;--i)
            {
            if((kar = fgetc(fi)) == EOF)
                {
                return false;
                }
            s += kar << 8;
            }
        surrogate(s,fo); // There SHOULD not be any surrogate pairs...
        }
    return true;
    }

bool bigendian(FILE * fi,FILE * fo)
    {
    int kar;
    while((kar = fgetc(fi)) != EOF)
        {
        int s = kar << 8;
        if((kar = fgetc(fi)) == EOF)
            return false;
        s += kar;
        surrogate(s,fo);
        }
    return true;
    }

bool bigendian32(FILE * fi,FILE * fo)
    {
    int kar;
    while((kar = fgetc(fi)) != EOF)
        {
        int s = kar << 24;
        for(int i = 2;i >= 0;--i)
            {
            if((kar = fgetc(fi)) == EOF)
                return false;
            s += kar << (i * 4);
            }
        surrogate(s,fo); // There SHOULD not be any surrogate pairs...
        }
    return true;
    }

void iso(FILE * fi,FILE * fo)
    {
    int kar;
    while((kar = fgetc(fi)) != EOF)
        {
        UnicodeToUtf8(kar,fo); // No reason to call surrogate() instead.
        }
    }

void iso(FILE * fi,FILE * fo,int * code)
    {
    int kar;
    while((kar = fgetc(fi)) != EOF)
        {
        if(kar >= 0x80)
            {
            assert(0 <= kar - 0x80);
            assert(kar - 0x80 < 128);
            kar = code[kar - 0x80];
            }
        UnicodeToUtf8(kar,fo); // No reason to call surrogate() instead.
        }
    }

void copy(FILE * fi,FILE * fo)
    {
    int kar;
    while((kar = fgetc(fi)) != EOF)
        {
        fputc(kar,fo);
        }
    }

void bitpat(int c,int n)
    {
    for(int i = n; --i >= 0;)
        {
        if(c & (1 << i))
            putchar('1');
        else
            putchar('0');
        if(!(i % 8))
            putchar(' ');
        if(!(i % 4))
            putchar(' ');
        }
    }

int n = -1;
bool isEOF = false;

bool validCodePoint(unsigned int val)
    {
    if  (
#if RFC3629compliant
        0x0010ffff < val         // illegal code point
#else
        0x80000000 <= val        // does not fit in 31 bits
#endif
        )
        return false;
    return true;
    }

/*
Counters used to guess encoding if no BOM is present and UTF-8 already is
discarded.
*/
int UTF16LEascii = 0;// higher 25 bits all zero
int UTF16BEascii = 0;
int UTF32LEascii = 0;
int UTF32BEascii = 0;
int UTF32LE16 = 0; // higher 16 bits all zero
int UTF32BE16 = 0; 
int isoascii = 0; // higher bit zero
int UTF160 = 0; // all 16 bits zero
int UTF320 = 0; // all 32 bits zero
int iso0 = 0; // all 8 bits zero

int readwrite(FILE * fi,FILE * fo) // fo may be NULL!
    {
    static int UTF16LE = 0;
    static int UTF16BE = 0;
    static int UTF32LE = 0;
    static int UTF32BE = 0;
    int k = fgetc(fi);
    if(k != EOF)
        {
        if(k < 0x80)
            {
            ++isoascii;
            if(k == 0)
                ++iso0;
            }
        ++n;
        int N32 = n % 4;
        switch(N32)
            {
            case 0:
                UTF16LE = k;
                UTF16BE = k << 8;
                UTF32LE = k;
                UTF32BE = k << 24;
                break;
            case 1:
                UTF16LE += k << 8;
                UTF16BE += k;
                UTF32LE += k << 8;
                UTF32BE += k << 16;
                if(!validCodePoint(UTF16LE))
                    possiblyUTF16LE = false;
                if(!validCodePoint(UTF16BE))
                    possiblyUTF16BE = false;
                if(UTF16LE < 0x80)
                    ++UTF16LEascii;
                if(UTF16BE < 0x80)
                    ++UTF16BEascii;
                if(UTF16LE == 0)
                    {
                    ++UTF32BE16;
                    ++UTF160;
                    }
                break;
            case 2:
                UTF16LE = k;
                UTF16BE = k << 8;
                UTF32LE += k << 16;
                UTF32BE += k << 8;
                break;
            case 3:
                UTF16LE += k << 8;
                UTF16BE += k;
                UTF32LE += k << 24;
                UTF32BE += k;
                if(!validCodePoint(UTF16LE))
                    possiblyUTF16LE = false;
                if(!validCodePoint(UTF16BE))
                    possiblyUTF16BE = false;
                if(!validCodePoint(UTF32LE))
                    possiblyUTF32LE = false;
                if(!validCodePoint(UTF32BE))
                    possiblyUTF32BE = false;
                if(UTF16LE < 0x80)
                    ++UTF16LEascii;
                if(UTF16BE < 0x80)
                    ++UTF16BEascii;
                if(UTF16LE == 0)
                    {
                    ++UTF32LE16;
                    ++UTF160;
                    }
                if(UTF32LE < 0x80)
                    ++UTF32LEascii;
                if(UTF32BE < 0x80)
                    ++UTF32BEascii;
                if(UTF32BE == 0)
                    ++UTF320;
                break;
            }
        if(fo)
            fputc(k,fo);
        }
    else
        isEOF = true;
    return k;
    }

int readUTF8char(FILE * fi)
/* makes no check of validity! */
    {
    int K;
    int k[6]; // k[0] unused
    K = fgetc(fi);
    if(K != EOF)
        {
        if((K & 0xc0) == 0xC0) // 11bbbbbb
            {
            int i = 1;
            while((K << i) & 0x80)
                {
                k[i++] = fgetc(fi);
                }
            K = ((K << i) & 0xff) << (5 * i - 6);
            int I = --i;
            while(i > 0)
                {
                K |= (k[i] & 0x3f) << ((I - i) * 6);
                --i;
                }
            }        
        }
    return K;
    }

bool UTF8(FILE * fi,FILE * fo)
    {
    int k[6];
    bool validUTF8 = true;
    bool surrogateFirstWordFound = false;
    while(validUTF8 && (k[0] = readwrite(fi,fo)) != EOF)
        {
        switch(k[0] & 0xc0) // 11bbbbbb
            {
            case  0xc0:
                {
#if RFC3629compliant
                if(k[0] > 0xF4)
#else
                if(k[0] > 0xFD)
#endif
                    {
                    validUTF8 = false;
                    break;
                    }
                // Start of multibyte
                int i = 1;
                /*
                Unicode 	        Byte1 	    Byte2 	    Byte3 	    Byte4
                U+000000-U+00007F 	0xxxxxxx 			
                U+000080-U+0007FF 	110xxxxx 	10xxxxxx 		
                U+000800-U+00FFFF 	1110xxxx 	10xxxxxx 	10xxxxxx 	
                U+010000-U+10FFFF 	11110xxx 	10xxxxxx 	10xxxxxx 	10xxxxxx
                */
                while((k[0] << i) & 0x80)
                    {
    #if RFC3629compliant
                    if(i >= 4)
    #else
                    if(i >= 6)
    #endif
                        {
                        validUTF8 = false;
                        break;
                        }
                    k[i] = readwrite(fi,fo);
                    if(isEOF)
                        {
                        validUTF8 = false;
                        break;
                        }
                    if((k[i++] & 0xc0) != 0x80) // 10bbbbbb
                        {
                        validUTF8 = false;
                        break;
                        }
                    }
                if(!validUTF8)
                    break;
                int K = ((k[0] << i) & 0xff) << (5 * i - 6);
                int I = --i;
                assert(i < 6);
                while(i > 0)
                    {
                    K |= (k[i] & 0x3f) << ((I - i) * 6);
                    --i;
                    }
                if(  K <= 0x7f              // overlong UTF-8 sequence
                  || K <= 0x07FF && I > 2   // overlong UTF-8 sequence
                  || K <= 0xFFFF && I > 3   // overlong UTF-8 sequence
                  )
                    {
                    UTF8containsOverlongCharacters = true;
                    fprintf(stderr,"Overlong character found.\n");
                    }
                if(!validCodePoint(K))
                    {
                    validUTF8 = false;
                    break;
                    }
                if(!UTF8containsSurrogates)
                    {
                    if (!surrogateFirstWordFound)
                        {
                        if(0xd800 <= K 
                            && K < 0xdc00
                            ) // illegal code point that possibly can be repaired
                            {
                            surrogateFirstWordFound = true;
                            }
#if NOUCS2
                        else if(0xdc00 <= K && K <= 0xdfff) // illegal UCS-2 code point
                            {
                            return false;
                            }
#endif
                        }
                    else 
                        {
                        if  (  surrogateFirstWordFound
                            && 0xdc00 <= K 
                            && K <= 0xdfff
                            ) // illegal code point that can be repaired
                            {
                            UTF8containsSurrogates = true;
                            fprintf(stderr,"Surrogate pair found in UTF-8\n");
//                          exit(-1);
                            }
#if NOUCS2
                        else // illegal UCS-2 code point
                            {
                            return false;
                            }
#endif
                        surrogateFirstWordFound = false;
                        }
                    }
                if(K == 0xFEFF && n == 2)
                    {
                    possiblyISO8859 = false;
                    hasEBBBBF = true;
                    }
                break;
                }
            case 0x80: // 10bbbbbb
                { // Not UTF-8
                validUTF8 = false;
                break;
                }
            default:
                ;
            }        
        }
    if(!validUTF8 && !isEOF)
        {
        int K;
        if(  (n == 0 || n == 2)
          && (k[0] & 0xfe) == 0xfe
          )
            {
            K = readwrite(fi,fo);
            if((K^k[0]) == 1)
                {
                if(K == 0xff)
                    {
                    hasFEFF = true;
                    possiblyISO8859 = false;
                    possiblyUTF16LE = false;
                    possiblyUTF32LE = false;
                    }
                else
                    {
                    hasFFFE = true;
                    possiblyISO8859 = false;
                    possiblyUTF16BE = false;
                    possiblyUTF32BE = false;
                    }
                if(n == 1)
                    {
                    if(hasFFFE)
                        {
                        if(  readwrite(fi,fo) == 0
                          && readwrite(fi,fo) == 0
                          )
                            isUTF32 = true;
                        else
                            isUTF16 = true;
                        }
                    else
                        isUTF16 = true;
                    }
                else
                    isUTF32 = true;
                if(isUTF16)
                    {
                    possiblyUTF32LE = false;
                    possiblyUTF32BE = false;
                    }
                else if(isUTF32)
                    {
                    possiblyUTF16LE = false;
                    possiblyUTF16BE = false;
                    }
                }
            }
        if(!isEOF)
            {
            while((K = readwrite(fi,fo)) != EOF)
                {
                }
            }
        if(possiblyUTF16LE && possiblyUTF16BE)
            {
            if(UTF16LEascii < UTF16BEascii)
                {
                possiblyUTF16LE = false; // assume some ASCII values in text
                }
            else if(UTF16LEascii > UTF16BEascii)
                {
                possiblyUTF16BE = false; // assume some ASCII values in text
                }
            }
        if(possiblyUTF16LE && possiblyUTF32BE)
            {                 
            if(UTF16LEascii < UTF32BEascii)
                {
                possiblyUTF16LE = false; // assume some ASCII values in text
                }
            else if(UTF16LEascii > UTF32BEascii)
                {
                possiblyUTF32BE = false; // assume some ASCII values in text
                }
            }
        if(possiblyUTF16BE && possiblyUTF32LE)
            {                 
            if(UTF16BEascii < UTF32LEascii)
                {
                possiblyUTF16BE = false; // assume some ASCII values in text
                }
            else if(UTF16BEascii > UTF32LEascii)
                {
                possiblyUTF32LE = false; // assume some ASCII values in text
                }
            }
        if(possiblyUTF32LE && possiblyUTF32BE)
            {                 
            if(UTF32LEascii < UTF32BEascii)
                {
                possiblyUTF32LE = false; // assume some ASCII values in text
                }
            else if(UTF32LEascii > UTF32BEascii)
                {
                possiblyUTF32BE = false; // assume some ASCII values in text
                }
            }
        if(possiblyUTF32LE && possiblyUTF32BE)
            {
            if(UTF32LE16 > UTF32BE16)
                possiblyUTF32BE = false;
            else if(UTF32LE16 < UTF32BE16)
                possiblyUTF32LE = false;
            }
        int possibilities = 0;
        if(possiblyUTF16LE)
            ++possibilities;
        if(possiblyUTF16BE)
            ++possibilities;
        if(possiblyUTF32LE)
            ++possibilities;
        if(possiblyUTF32BE)
            ++possibilities;
        if(possibilities > 1 && possiblyISO8859)
            {
            possiblyUTF16LE = 0;
            possiblyUTF16BE = 0;
            possiblyUTF32LE = 0;
            possiblyUTF32BE = 0;
            }
        return false;
        }
    possiblyUTF16LE = false;
    possiblyUTF16BE = false;
    possiblyUTF32LE = false;
    possiblyUTF32BE = false;
    possiblyISO8859 = false;
    return true;
    }

int main(int argc,char * argv[])
    {
    FILE * fi = stdin;
    FILE * fo = stdout;
    int retval = 0;
    int * convtable = NULL;
    int arg = 1;
    if(argc > 1)
        {
        if(argv[1][0] == '-')
            {long code = strtol(argv[1]+1,0,10);
            convtable = getCode(code);
            if(!convtable)
                {
                if(argv[1][1] == 'h')
                    {
                    printf("makeUTF8 converts UTF-32, UTF-16 (LE and BE) and ISO-8859 to UTF-8.\n");
                    printf("makeUTF8 corrects UTF-8 by recombining surrogate pairs as introduced by e.g. MS-Word\n");
                    printf("         and by changing overlong characters to valid UTF-8 characters.\n");
                    printf("makeUTF8 removes the UTF-8 signature (\"BOM\") at the start of a UTF-8 file.\n");
                    printf("usage:\nmakeUTF8 [-n] input output\nwhere 0 < n < 12 or 12 < n <= 16 indicates iso 8859-n\n");
                    printf("1 	Latin-1 Western European\n");
                    printf("2 	Latin-2 Central European (Bosnian, Polish, Croatian, Czech, Slovak, Slovenian, Serbian, and Hungarian)\n");
                    printf("3 	Latin-3 South European (Turkish, Maltese, and Esperanto)\n");
                    printf("4 	Latin-4 North European (Estonian, Latvian, Lithuanian, Greenlandic, and Sami)\n");
                    printf("5 	Latin/Cyrillic 	(Belarusian, Bulgarian, Macedonian, Russian, Serbian, and Ukrainian)\n");
                    printf("6 	Latin/Arabic\n");
                    printf("7 	Latin/Greek\n");
                    printf("8 	Latin/Hebrew\n");
                    printf("9 	Latin-5 Turkish\n");
                    printf("10 	Latin-6 Nordic\n");
                    printf("11 	Latin/Thai\n");
                    printf("13 	Latin-7 Baltic Rim\n");
                    printf("14 	Latin-8 Celtic\n");
                    printf("15 	Latin-9 euro sign € and the letters Š, š, Ž, ž, Œ, œ, and Ÿ\n");
                    printf("16 	Latin-10 South-Eastern European (Albanian, Croatian, Hungarian, Italian, Polish, Romanian and Slovenian, but also Finnish, French, German and Irish Gaelic)\n");
                    printf("850 	CodePage 850\n");
                    return 0;
                    }
                if(code)
                    {
                    }
                else
                    {
                    printf("invalid option \'%s\'\n",argv[1]);
                    return 1;
                    }
                }
            ++arg;
            }
        if(argc > arg)
            {
            fi = fopen(argv[arg],"rb");
            if(!fi)
                {
                fprintf(stderr,"Cannot open file %s for reading\n",argv[arg]);
                exit(-1);
                }
            ++arg;
            if(argc > arg)
                {
                fo = fopen(argv[arg],"wb");
                if(!fi)
                    {
                    fprintf(stderr,"Cannot open file %s for writing\n",argv[arg]);
                    exit(-1);
                    }
                }
            }
        }
#ifdef WIN32
    char * temp = NULL;
#endif
    FILE * fx;
    FILE * ftemp = NULL;
    if(fi == stdin)
        {
#ifdef WIN32
        _setmode(_fileno(stdin), O_BINARY);
        temp = tmpnam(NULL);
        if(!temp)
            {
            fprintf(stderr,"Cannot create temporary file\n");
            exit(-1);
            }
        ftemp = fopen(temp,"wb+");
#else
        ftemp = tmpfile();
#endif
        if(!ftemp)
            {
#ifdef WIN32
            fprintf(stderr,"Cannot create temporary file %s\n",temp);
#else
            fprintf(stderr,"Cannot create temporary file\n");
#endif
            exit(-1);
            }
        fx = ftemp;
        }
    else
        {
        ftemp = NULL;
        fx = fi;
        }
#ifdef WIN32
    if(fo == stdout)
        _setmode(_fileno(stdout), O_BINARY);
#endif
    if(UTF8(fi,ftemp))
        {
//      rewind(fx);
        if(fseek(fx,0,SEEK_SET))
            {
            fprintf(stderr,"Cannot rewind\n");
            exit(-1);
            }
        if(hasEBBBBF)
            {
            fgetc(fx);
            fgetc(fx);
            fgetc(fx);
            }
        if(UTF8containsSurrogates || UTF8containsOverlongCharacters)
            {
            int K;
            while((K = readUTF8char(fx)) != EOF)
                surrogate(K,fo);
            }
        else
            copy(fx,fo);
        }
    else
        {
//      rewind(fx);
        if(fseek(fx,0,SEEK_SET))
            {
            fprintf(stderr,"Cannot rewind\n");
            exit(-1);
            }
        if(possiblyUTF32LE)
            {
            if(hasFFFE)
                fseek(fx,4,SEEK_SET);
            littleendian32(fx,fo);
            }
        else if(possiblyUTF32BE)
            {
            if(hasFEFF)
                fseek(fx,4,SEEK_SET);
            bigendian32(fx,fo);
            }
        else if(possiblyUTF16BE)
            {
            if(hasFEFF)
                fseek(fx,2,SEEK_SET);
            bigendian(fx,fo);
            }
        else if(possiblyUTF16LE)
            {
            if(hasFFFE)
                fseek(fx,2,SEEK_SET);
            littleendian(fx,fo);
            }
        else if(possiblyISO8859)
            {
            if(convtable)
                iso(fx,fo,convtable);
            else
                iso(fx,fo);
            }
        else
            {
            copy(fi,fo);
            retval = 1;
            }
        }
    if(fi != stdin)
        fclose(fi);
    if(fo != stdout)
        fclose(fo);
    if(ftemp)
        fclose(ftemp);
#ifdef WIN32
    if(temp)
        {
        if(remove(temp))
            {
            fprintf(stderr,"Cannot remove file %s: %s\n",errno == EACCES ? "EACCES" : errno == ENOENT ? "ENOENT" : "unknown cause" );
            }
        }
#endif
    return retval;
    }
