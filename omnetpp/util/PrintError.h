//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

#ifndef PRINTERROR_H_
#define PRINTERROR_H_
#include <stdio.h>
#include <string>
#include <stdarg.h>
#include <iostream>
using namespace std;

class PrintError {
private:
	static void check();
public:
	PrintError();
	static void print(string, string);
	static void print(string, string, const int);
	static void print(string, string, const long);
	static void print(string, string, const double);
	static void print(string, const int, string);
	static void print(string, const int, string, const int);
	static void print(string, const int, string, const long);
	static void print(string, const int, string, const double);
	virtual ~PrintError();
};

#endif /* PRINTERROR_H_ */
