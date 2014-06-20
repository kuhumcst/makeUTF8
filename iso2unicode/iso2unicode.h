/*
    iso2unicode

    Copyright (C) 2014  Center for Sprogteknologi, University of Copenhagen

    This file is part of makeUTF8.

    makeUTF8 is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Foobar is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef ISO2UNICODE_H
#define ISO2UNICODE_H

class duple
    {
    public:
        int eightbit;
        int isocodes;
    };

extern duple * duples;

int * getCode(int isocode);
int makeDuples(void);
void deleteDuples(void);
int getEightBit(int unicode);
char * report();

#endif
