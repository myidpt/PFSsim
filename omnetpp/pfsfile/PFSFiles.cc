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

#include "PFSFiles.h"

PFSFiles::PFSFiles() {

}

void PFSFiles::addPFSFile(PFSFile * file) {
	if (file == NULL){
		PrintError::print("PFSFile::setLayout", "Provided PFSFile is NULL.");
	}

	map<int, PFSFile *>::iterator it = fileDirectory.find(file->getID());
	if (it == fileDirectory.end()){
		fileDirectory.insert(pair<int, PFSFile *>(file->getID(), file));
	}
}

PFSFile * PFSFiles::findPFSFile(int id) {
	map<int, PFSFile *>::iterator it = fileDirectory.find(id);
	if (it == fileDirectory.end()){
		return NULL;
	}
	return it->second;
}

PFSFiles::~PFSFiles() {

}
