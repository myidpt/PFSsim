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

#ifndef INPUTFILES_H_
#define INPUTFILES_H_

#include <iostream>
#include <fstream>
#include <vector>
#include <string.h>

#include "General.h"

using namespace std;

class InputFiles {
public:
	vector<ifstream *> streams;

	InputFiles(int num, int digits, const string & prefix, const string & postfix);
	int fileCount();
	bool readLine(int index, string & line);
	~InputFiles();
};

#endif /* INPUTFILES_H_ */
