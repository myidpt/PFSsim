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

#include "PrintError.h"

using namespace std;

PrintError::PrintError() {

}

void PrintError::check(){
//	while(1)
//		sleep(5);
}

void PrintError::print(string locname, string sentence){
	cerr << "[ERROR] " << locname << ": " << sentence << endl;
	fflush(stderr);
	check();
}

void PrintError::print(string locname, string sentence, const int lastnum){
	cerr << "[ERROR] " << locname << ": " << sentence << " ( " << lastnum  << " )" << endl;
	fflush(stderr);
	check();
}

void PrintError::print(string locname, string sentence, const long lastnum){
	cerr << "[ERROR] " << locname << ": " << sentence << " ( " << lastnum  << " )" << endl;
	fflush(stderr);
	check();
}

void PrintError::print(string locname, string sentence, const double lastnum){
	cerr << "[ERROR] " << locname << ": " << sentence << " ( " << lastnum  << " )" << endl;
	fflush(stderr);
	check();
}

void PrintError::print(string locname, const int number, string sentence){
	cerr << "[ERROR] " << locname << " #" << number << ": " << sentence << endl;
	fflush(stderr);
	check();
}

void PrintError::print(string locname, const int number, string sentence, const int lastnum){
	cerr << "[ERROR] " << locname << " #" << number << ": " << sentence << " ( " << lastnum  << " )" << endl;
	fflush(stderr);
	check();
}

void PrintError::print(string locname, const int number, string sentence, const long lastnum){
	cerr << "[ERROR] " << locname << " #" << number << ": " << sentence << " ( " << lastnum  << " )" << endl;
	fflush(stderr);
	check();
}

void PrintError::print(string locname, const int number, string sentence, const double lastnum){
	cerr << "[ERROR] " << locname << " #" << number << ": " << sentence << " ( " << lastnum  << " )" << endl;
	fflush(stderr);
	check();
}

PrintError::~PrintError() {
}
