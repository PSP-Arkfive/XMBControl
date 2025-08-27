/*
    6.39 TN-A, XmbControl
    Copyright (C) 2011, Total_Noob
    Copyright (C) 2011, Frostegater

    main.h: XmbControl main header file
    
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef XMB_UTILS_H
#define XMB_UTILS_H

char * strtrim(char * text);
int utf8_to_unicode(wchar_t *dest, char *src);

#endif