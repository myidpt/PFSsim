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

#include "OutputFiles.h"

OutputFiles::OutputFiles(int num, int digits, const string & prefix, const string & postfix) {
	string name = prefix;
	for (int i = 0; i < digits; i ++){
		name.append("0");
	}
	name += postfix;

	// For just one file case. Because the user may give digits=0 in this case, we need to deal with this
	// separately, otherwise the following process will give an error.
	if (num == 1){
		ofstream * stream = new ofstream();
		stream->open(name.c_str());
		cout << "Open file for output: " << name << endl;
		if (stream->fail())
		{
			string printStr = "Cannot open file " + name;
			PrintError::print("OutputFiles::OutputFiles", printStr.c_str());
		}
		streams.push_back(stream);
		return;
	}

	// To validate if the digits is enough.
	int power = 10;
	for (int i = 1; ; i ++){
		if (num >= power && digits < i){ // Starting from 1.
			PrintError::print("InputFiles::InputFiles", "Digits is not enough for the number of files ", num);
		}
		if (num < power){
			break;
		}
		power *= 10;
	}

	int digitStartBit = prefix.length() + digits - 1; // The start bit of the digit area.
	for (int i = 0; i < num; i ++){
		if (digits >= 3){
			name[digitStartBit - 2] = i / 100 + '0';
		}
		if (digits >= 2){
			name[digitStartBit - 1] = i % 100 / 10 + '0';
		}
		name[digitStartBit] = i % 10 + '0';

		ofstream * stream = new ofstream();
		stream->open(name.c_str());
		cout << "Open file for output: " << name << endl;
		if (stream->fail())
		{
			string printStr = "Cannot open file " + name;
			PrintError::print("InputFiles::InputFiles", printStr.c_str());
		}
		streams.push_back(stream);
	}
}

int OutputFiles::fileCount() {
	return streams.size();
}

void OutputFiles::writeLine(int index, const string & line) {
	if (index >= (signed int)streams.size()){
		PrintError::print("OutputFiles::writeLine", "Index is bigger than the streams size",
				(signed int)streams.size());
	}
	if (streams[index] != NULL && streams[index]->is_open()) {
		*streams[index] << line << endl;
	}
	else {
		PrintError::print("OutputFiles::writeLine", "stream is NULL or not open.");
	}
}

OutputFiles::~OutputFiles() {
	for (int i = 0; i < (signed int)streams.size(); i ++) {
		if (streams[i] != NULL) {
			streams[i]->close();
			delete streams[i];
			streams[i] = NULL;
		}
	}
}
