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

#include "PFSFileInputStreamer.h"

PFSFileInputStreamer::PFSFileInputStreamer(InputFiles * files)
	: KiloUnitConvertion(1024), inputFiles(files) {
}

PFSFiles * PFSFileInputStreamer::readPFSFiles() {
	string line;
	PFSFiles * files = new PFSFiles();
	for (int i = 0; i < inputFiles->fileCount(); i ++) { // Process one file.
		while( inputFiles->readLine(i, line) ) { // Process one line.
			Layout * layout = new Layout();
			int i = 0;
			int index = 0;
			long val = -1;
			bool end = false; // True if it meets the end of line.
			bool sep = false; // True if a separation mark is found.
			enum position_type{file_id, server_id, server_stripe_size};
			int position = file_id;
			while(!end) { // Get one field.
				if(line[i] == '\n' || line[i] == '\0'){
					end = true;
					sep = true;
				}
				if(line[i] == ' ' || line[i] == '\t' || line[i] == ',' || line[i] == '[' || line[i] == ']'){
					sep = true;
				}
				if(line[i] >= '0' && line[i] <= '9'){
					if(val  == -1)
						val = 0;
					val = val * 10 + (line[i] - '0');
					sep = false;
				}
				if(line[i] == 'k' || line[i] == 'K'){
					sep = true;
					if(val == -1){ // Useless label if there's no numeral part right before k.
						i++;
					    continue;
					}
					val = val * KiloUnitConvertion;
				}
				if(line[i] == 'm' || line[i] == 'M'){
					sep = true;
					if(val == -1){ // Useless label if there's no numeral part right before m.
						i++;
					    continue;
					}
					val = val * KiloUnitConvertion * KiloUnitConvertion;
				}

				if(val != -1 && sep == true){
					if(position == file_id){
						if(val >= MAX_FILE){
							PrintError::print("PFSFileInputStreamer::readPFSFiles",
									"File ID in layout file out of range ", (int)val);
						}
						layout->setFileID((int)val);
						position = server_id;
					}else if(position == server_id){
						layout->setServerID(index, (int)val);
						position = server_stripe_size;
					}else{
						layout->setServerStripeSize(index, val);
						position = server_id;
						index ++;
					}
					val = -1;
				}
				i ++;
			}
			// Set the server number and window size.
			layout->setServerNum(index);
			layout->calculateWindowSize();

			// Create the PFSFile
			PFSFile * file = new PVFS2File(layout->getFileID());
			file->setLayout(layout);
			files->addPFSFile(file);
		}
	}
	return files;
}

PFSFileInputStreamer::~PFSFileInputStreamer() {
	if (inputFiles != NULL) {
		delete inputFiles;
	}
}
