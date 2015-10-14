/*
    encoding

    Detects encoding. If unicode, guesses best fitting iso code.

    Copyright (C) 2014  Center for Sprogteknologi, University of Copenhagen

    This file is part of makeUTF8.

    makeUTF8 is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Encoding is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Encoding.  If not, see <http://www.gnu.org/licenses/>.
*/
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <string.h>
#include "iso2unicode.h"

int surrogatesfound = 0;
bool ascii = true;
bool utf8 = false;
bool le = false;
bool be = false;

char * type = "";
int result = 0;


void getbyte(int s)
    {
    int eight = getEightBit(s);
    if(eight > 127 || eight < 0)
        ascii = false;
    }

void surrogate(int s)
    {
    static int prev = 0;
    if(prev == 0)
        {
        if(surrogatesfound >= 0 && (s & 0xFC00) == 0xD800) // first word surrogat
            {
            ++surrogatesfound;
            prev = s;
            }
        else
            {
            getbyte(s);
            }
        }
    else
        {
        if(surrogatesfound >= 0 && (s & 0xFC00) == 0xDC00) // second word surrogat
            {
            s = (s & 0x3ff) + ((prev & 0x3ff) << 10) + 0x10000;
            getbyte(s);
            }
        else
            {
            // Assume it is UCS-2, not UTF-16
            surrogatesfound = -1;
            getbyte(s);
            }
        prev = 0;
        }
    }

void printmb(bool BOMfound)
    {
    if(type[0])
        {
        if(surrogatesfound == -1)
            {
            if(  (  BOMfound 
                 && (  !strcmp(type,"UCS-2") 
                    || le && !strcmp(type,"UCS-2-LE") 
                    || be && !strcmp(type,"UCS-2-BE")
                    )
                 ) 
              || !strcmp(type,"BIN")
              )
                result = 1;
            }
        else if(surrogatesfound == 0)
            {
            if(  !strcmp(type,"UCS-2") || !strcmp(type,"UTF-16")
              || le && (!strcmp(type,"UCS-2-LE") || !strcmp(type,"UTF-16-LE"))
              || be && (!strcmp(type,"UCS-2-BE") || !strcmp(type,"UTF-16-BE"))
              )
                result = 1;
            }
        else
            if(  !strcmp(type,"UTF-16") 
              || le && !strcmp(type,"UTF-16-LE") 
              || be && !strcmp(type,"UTF-16-BE")
              )
                result = 1;
        }
    else
        {
        printf("encoding: %s\n",
              surrogatesfound == -1 ? (BOMfound ? "UCS-2" : "binary") : 
              surrogatesfound == 0  ? "UTF-16 or UCS-2, no surrogates" 
                                    : "UTF-16, with surrogates"
              );
        }
    }

void littleendian(FILE * fi)
    {
    int kar;
    int s;
    while((kar = fgetc(fi)) != EOF)
        {
        s = kar;
        if((kar = fgetc(fi)) == EOF)
            break;
        s += kar << 8;
        surrogate(s);
        }
    printmb(true);
    }

void bigendian(FILE * fi,bool BOMfound)
    {
    if(BOMfound)
        {
        if(type[0])
            {
            be = true;
            if(!strcmp(type,"BE"))
                result = 1;
            }
        else
            {
            printf("Byte order: big endian\n");
            }
        }
    else if(!type[0])
        printf("Unkown byte order. Assuming big endian.\n");
    int kar;
    int s;
    while((kar = fgetc(fi)) != EOF)
        {
        s = kar << 8;
        if((kar = fgetc(fi)) == EOF)
            break;
        s += kar;
        surrogate(s);
        }
    printmb(BOMfound);
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
bool UTF8(FILE * fi)
    {
    int k[6];
    while((k[0] = fgetc(fi)) != EOF)
        {
        switch(k[0] & 0xc0) // 11bbbbbb
            {
            case  0xc0:
                {
                ascii = false;
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
                getbyte(K);
                break;
                }
            case 0x80: // 10bbbbbb
                // Not UTF-8
                return false;
            default:
                getbyte(k[0]);
            }        
        }
    return true;
    }

int main(int argc,char * argv[])
    {
    int b1,b2,b3 = 0;
    FILE * fi = stdin;
    if(argc > 1)
        {
        fi = fopen(argv[1],"rb");
        }
    if(argc > 2)
        {
        type = argv[2];
        char * valid[] =
            {"LE"
            ,"BE"
            ,"UCS-2"
            ,"UCS-2-LE"
            ,"UCS-2-BE"
            ,"BIN"
            ,"UTF-16"
            ,"UTF-16-LE"
            ,"UTF-16-BE"
            ,"ASCII"
            ,"ISO"
            ,"UTF-8"
            ,"UTF-8-BOM"
            };
        int i;
        for(i = sizeof(valid)/sizeof(valid[0]);--i >= 0;)
            if(!strcmp(type,valid[i]))
                break;
        if(i < 0)
            {
            printf("second argument must be one of:\n");
            for(i = 0;i < sizeof(valid)/sizeof(valid[0]);++i)
                printf("%s\n",valid[i]);
            return 1;
            }
        }
    if(!fi)
        {
        printf("Cannot open input %s\n",argv[1]);
        return 1;
        }
    makeDuples();
    b1 = fgetc(fi);
    b2 = fgetc(fi);

    if(b1 == 0xff && b2 == 0xfe)
        {
        if(type[0])
            {
            le = true;
            if(!strcmp(type,"LE"))
                result = 1;
            }
        else
            {
            printf("Byte order: little endian\n");
            }
        littleendian(fi);
        }
    else if(b1 == 0xfe && b2 == 0xff)
        {
        bigendian(fi,true);
        }
    else if(b1 == 0 || b2 == 0) // No BOM, but still looks like UTF-16
        { // assume big endian
        surrogate((b1 << 8) + b2);
        bigendian(fi,false);
        }
    else
        {
        FILE * ftempInput = tmpfile();
        b3 = fgetc(fi);
        if(b3 == 0)// No BOM, but still looks like UTF-16
            { // assume big endian
            surrogate((b1 << 8) + b2);
            surrogate((b3 << 8) + fgetc(fi));
            bigendian(fi,false);
            }
        else
            {
            if(b1 == 0xEF && b2 == 0xBB && b3 == 0xBF) // BOM found, probably UTF8
                {
                ;// remove BOM
                }
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
                    surrogate((b1 << 8) + b2);
                    surrogate((b3 << 8) + fgetc(ftempInput));
                    }
                bigendian(ftempInput,false);
                }
            else
                {
                bool bom = false;
                if(b1 && b2 && b3)
                    {
                    if(type[0])
                        {
                        bom = true;
                        }
                    else
                        printf("BOM found, but not UTF-16. (UTF-8 file created in Windows?)\n");
                    }
                if(UTF8(ftempInput))
                    {
                    utf8 = true;
                    if(ascii)
                        {
                        if(type[0])
                            {
                            if(  !bom && (!strcmp(type,"ASCII") || !strcmp(type,"ISO"))
                              || !strcmp(type,"UTF-8") || bom && !strcmp(type,"UTF-8-BOM")
                              )
                                result = 1;
                            }
                        else
                            {
                            printf("encoding: ASCII (subset of UTF-8 and all ISO-8859 encodings)\n");
                            }
                        }
                    else
                        {
                        if(type[0])
                            {
                            if(!strcmp(type,"UTF-8") || bom && !strcmp(type,"UTF-8-BOM"))
                                result = 1;
                            }
                        else
                            {
                            printf("encoding: UTF-8\n");
                            }
                        }
                    }
                else
                    {
                    int c = 0;
                    while((k = fgetc(ftempInput)) != EOF)
                        getbyte(k);
                    if(type[0])
                        {
                        if(!bom && (!strcmp(type,"ASCII") || !strcmp(type,"ISO")))
                            result = 1;
                        }
                    else
                        {
                        printf("encoding: 8-bits\n");
                        }
                    }
                }
            }
        }
    if(fi != stdin)
        fclose(fi);
    if(!type[0])
        fprintf(stderr,"%s",report());
    if(!utf8 && ascii)
        if(!type[0])
            printf("File could have been encoded in ASCII!\n");
    deleteDuples();
    if(type[0])
        {
        printf("[%d]\t%s\n",result,argv[1]);
        }
    return 0;
    }
