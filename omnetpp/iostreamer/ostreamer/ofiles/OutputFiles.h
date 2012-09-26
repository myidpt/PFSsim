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

#ifndef OUTPUTFILES_H_
#define OUTPUTFILES_H_

#include <fstream>
#include <iostream>
#include <vector>
#include <string.h>

#include "General.h"

using namespace std;

class OutputFiles {
public:
	vector<ofstream *> streams;

	OutputFiles(int num, int digits, const string & prefix, const string & postfix);
	int fileCount();
	void writeLine(int index, const string & line);
	virtual ~OutputFiles();
};

#endif /* OUTPUTFILES_H_ */
