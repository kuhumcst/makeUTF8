/*
    makeISO

    Copyright (C) 2014  Center for Sprogteknologi, University of Copenhagen

    This file is part of makeUTF8.

    makeUTF8 is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    makeISO is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with makeISO.  If not, see <http://www.gnu.org/licenses/>.
*/
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include "iso2unicode.h"

void surrogate(int s,FILE * fo)
    {
    static int prev = 0;
    if(prev == 0)
        {
        if((s & 0xFC00) == 0xD800) // first word surrogat
            {
            prev = s;
            }
        else
            {
            int eight = getEightBit(s);
            if(eight == 0 && s != 0)
                fprintf(fo,"&#x%x;",s);            
            else 
                fputc(eight,fo);
            }
        }
    else
        {
        if((s & 0xFC00) == 0xDC00) // second word surrogat
            {
            s = (s & 0x3ff) + ((prev & 0x3ff) << 10) + 0x10000;
            fprintf(fo,"&#x%x;",s);
            }
        else
            {
            // Assume it is UCS-2, not UTF-16
            fprintf(fo,"&#x%x;",prev);
            if(s > 0)
                fprintf(fo,"&#x%x;",s); // You can call surrogate with a zero or -1 to empty prev.
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
            break;
        s += kar << 8;
        surrogate(s,fo);
        }
    }

void bigendian(FILE * fi,FILE * fo)
    {
    int kar;
    int s;
    while((kar = fgetc(fi)) != EOF)
        {
        s = kar << 8;
        if((kar = fgetc(fi)) == EOF)
            break;
        s += kar;
        surrogate(s,fo);
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

// EF BB BF = 1110 1111  1011 1011 1011 1111 = 1111 11 1011 11 1111 = 11111110 11111111 = FEFF = Byte Order Mark
// Added by many Windows programs to UTF8 files.
bool UTF8(FILE * fi,FILE * fo)
    {
    int k[6];
    FILE * fto = tmpfile();
    while((k[0] = fgetc(fi)) != EOF)
        {
        switch(k[0] & 0xc0) // 11bbbbbb
            {
            case  0xc0:
                {
                if((k[0] & 0xfe) == 0xfe)
                    return false;
                // Start of multibyte
                int i = 1;
                //bitpat(k[0],8);
                while((k[0] << i) & 0x80) // Read the second, third and fourth
                            // highest bits and read a byte for every bit == 1
                    {
                    k[i] = fgetc(fi);
                    //bitpat(k[i],8);
                    if((k[i++] & 0xc0) != 0x80) // 10bbbbbb
                        return false;
                    }
                int K = ((k[0] << i) & 0xff) // shift high-bits out of byte
                        << (5 * i - 6); // shift to high position
                //putchar('\n');
                //bitpat(K,32);
                int I = --i;
                while(i > 0)
                    {
                    K |= (k[i] & 0x3f) << ((I - i) * 6);
                    //putchar('\n');
                    //bitpat(K,32);
                    --i;
                    }
                //putchar('\n');
                //char a[2] = {K,0};
                if(K <= 0x7f)
                    return false; // overlong UTF-8 sequence
                int eight = getEightBit(K);
                if(eight == 0 && K != 0)
                    fprintf(fto,"&#x%x;",K);
                else 
                    fputc(eight,fto);
                break;
                }
            case 0x80: // 10bbbbbb
                // Not UTF-8
                return false;
            default:
                fputc(k[0],fto);
            }        
        }
    rewind(fto);
    while((k[0] = fgetc(fto)) != EOF)
        fputc(k[0],fo);
    return true;
    }

int main(int argc,char * argv[])
    {
    int b1,b2,b3 = 0;
    FILE * fi = stdin;
    FILE * fo = stdout;
    if(argc > 1)
        {
        fi = fopen(argv[1],"rb");
        if(argc > 2)
            fo = fopen(argv[2],"wb");
        }
    if(!fi || !fo)
        return 1;
    makeDuples();
    b1 = fgetc(fi);
    b2 = fgetc(fi);
    if(b1 == 0xff && b2 == 0xfe)
        littleendian(fi,fo);
    else if(b1 == 0xfe && b2 == 0xff)
        bigendian(fi,fo);
    else if(b1 == 0 || b2 == 0) // No BOM, but still looks like UTF-16
        { // assume big endian
        surrogate((b1 << 8) + b2,fo);
        bigendian(fi,fo);
        }
    else
        {
        FILE * ftempInput = tmpfile();
        b3 = fgetc(fi);
        if(b3 == 0)// No BOM, but still looks like UTF-16
            { // assume big endian
            surrogate((b1 << 8) + b2,fo);
            surrogate((b3 << 8) + fgetc(fi),fo);
            bigendian(fi,fo);
            }
        else
            {
            if(b1 == 0xEF && b2 == 0xBB && b3 == 0xBF) // BOM found, probably UTF8
                ; // remove BOM
            else
                {
                fputc(b1,ftempInput);
                fputc(b2,ftempInput);
                fputc(b3,ftempInput);
                b1 = b2 = b3 = 0;
                }

            int k;
            bool zeroFound = false;
            while((k = fgetc(fi)) != EOF)
                {
                if(k == 0)
                    zeroFound = true;
                fputc(k,ftempInput);
                }
            rewind(ftempInput);
            if(zeroFound)
                {
                if(b1 && b2 && b3)
                    {
                    surrogate((b1 << 8) + b2,fo);
                    surrogate((b3 << 8) + fgetc(ftempInput),fo);
                    }
                bigendian(ftempInput,fo);
                }
            else if(!UTF8(ftempInput,fo))
                {
                rewind(ftempInput);
                if(b1 && b2 && b3) // "BOM" found, but not in UTF8 file!
                    { // write "BOM"
                    fputc(b1,fo);
                    fputc(b2,fo);
                    fputc(b3,fo);
                    }
                copy(ftempInput,fo);
                }
            }
        }
    if(fi != stdin)
        fclose(fi);
    if(fo != stdout)
        fclose(fo);
    fprintf(stderr,"%s",report());
    deleteDuples();
    return 0;
    }
