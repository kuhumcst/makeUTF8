/*
    makeUNICODE 

    Converts input to UTF-16, littleendian.

    Copyright (C) 2014  Center for Sprogteknologi, University of Copenhagen

    This file is part of makeUTF8.

    makeUTF8 is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    makeUNICODE is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with makeUNICODE.  If not, see <http://www.gnu.org/licenses/>.
*/
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>

void Put(int k,FILE * fo)
    { // littleendian
    fputc(k & 0xff,fo);
    fputc(k >> 8,fo);
    }

void putUnicode(int k,FILE * fo)
    {
    if(0x10000 <= k && k <= 0x10FFFF) // need surrogate pair
        {
        k -= 0x10000; // 1111  1111 1111  1111 1111
        int h = (k >> 10) | 0xD800;   // 1101 10 00 0000 0000
        int l = (k & 0x3ff) | 0xDC00; // 1101 11 00 0000 0000
        Put(h,fo);
        Put(l,fo);
        }
    else
        {
        Put(k,fo);
        }
    }

void littleendian(FILE * fi,FILE * fo)
    {
    int kar;
    while((kar = fgetc(fi)) != EOF)
        {
        fputc(kar,fo);
        }
    }

void bigendian(FILE * fi,FILE * fo)
    {
    int kar;
    unsigned short s;
    while((kar = fgetc(fi)) != EOF)
        {
        s = kar << 8;
        if((kar = fgetc(fi)) == EOF)
            break;
        s += kar;
        Put(s,fo);
        }
    }

void copy(FILE * fi,FILE * fo)
    {
    int kar;
    while((kar = fgetc(fi)) != EOF)
        {
        Put(kar,fo);
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

bool UTF8(FILE * fi,FILE * fo)
    {
    int k[6];
    FILE * ftempOutput = tmpfile();

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
                while((k[0] << i) & 0x80)
                    {
                    k[i] = fgetc(fi);
                    //bitpat(k[i],8);
                    if((k[i++] & 0xc0) != 0x80) // 10bbbbbb
                        return false;
                    }
                int K = ((k[0] << i) & 0xff) << (5 * i - 6);
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
                if(K <= 0xff)
                    {
                    Put(K,ftempOutput);
                    }
                else
                    {
                    putUnicode(K,ftempOutput);
                    }
                break;
                }
            case 0x80: // 10bbbbbb
                // Not UTF-8
                return false;
            default:
                Put(k[0],ftempOutput);
            }        
        }
    rewind(ftempOutput);
    while((k[0] = fgetc(ftempOutput)) != EOF)
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
    Put(0xfeff,fo);
    b1 = fgetc(fi);
    b2 = fgetc(fi);
    if(b1 == 0xff && b2 == 0xfe)
        littleendian(fi,fo);
    else if(b1 == 0xfe && b2 == 0xff)
        bigendian(fi,fo);
    else if(b1 == 0 || b2 == 0) // No BOM, but still looks like UTF-16
        { // assume big endian
        Put((b1 << 8) + b2,fo);
        bigendian(fi,fo);
        }
    else
        {
        FILE * ftempInput = tmpfile();
        b3 = fgetc(fi);
        if(b3 == 0)// No BOM, but still looks like UTF-16
            { // assume big endian
            Put((b1 << 8) + b2,fo);
            Put((b3 << 8) + fgetc(fi),fo);
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
                    Put((b1 << 8) + b2,fo);
                    Put((b3 << 8) + fgetc(ftempInput),fo);
                    }
                bigendian(ftempInput,fo);
                }
            else if(!UTF8(ftempInput,fo))
                {
                rewind(ftempInput);
                if(b1 && b2 && b3)// "BOM" found, but not in UTF8 file!
                    {// write "BOM"
                    Put(b1,fo);
                    Put(b2,fo);
                    Put(b3,fo);
                    }
                copy(ftempInput,fo);
                }
            }
        }
    if(fi != stdin)
        fclose(fi);
    if(fo != stdout)
        fclose(fo);
    return 0;
    }
