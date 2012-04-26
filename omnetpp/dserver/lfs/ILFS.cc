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

#include "lfs/ILFS.h"

ILFS::ILFS(int id, int deg, long long disksize, int pagesize, int blksize, const char * inpath, const char * outpath) {
	myId = id;
	degree = deg;
	page_size = pagesize;
	blk_size = blksize;
	disk_size = disksize;
	pageReqQ = new list<PageRequest *>();
//	spaceallocp = 1;
	ifp = NULL;
	ofp = NULL;

	for(int i = 0; i < MAX_LFILE_NUM; i ++) // init
	    extlist[i] = NULL;

	// Open files.
	char infname[200];
	strcpy(infname, inpath);
	int len = strlen(infname);
	// Note: currently we only support less than 1000 data servers.
	infname[len] = myId/100 + '0';
	infname[len+1] = myId%100/10 + '0';
	infname[len+2] = myId%10 + '0';
	infname[len+3] = '\0';

	if( (ifp = fopen(infname, "r")) == NULL){
		printf("ILFS - init: extent input file %s does not exist, we consider the FS as empty!\n", infname);
	}

	if(ifp != NULL){ // It has input.
		initExtLists();
		fclose(ifp);
		ifp = NULL;
	}

	char outfname[200];
	strcpy(outfname, outpath);
	len = strlen(outfname);
	outfname[len] = myId/100 + '0';
	outfname[len+1] = myId%100/10 + '0';
	outfname[len+2] = myId%10 + '0';
	outfname[len+3] = '\0';

	if( (ofp = fopen(outfname, "w+")) == NULL)
		PrintError::print("ILFS - init", string("Output file open failure.") + outfname);

	cout << "ILFS #" << myId << " initialized." << endl;
}

void ILFS::initExtLists(){
	int fid;
	char line[200];
	if(!fgets(line, 199, ifp))
		return;
	if(strchr(line, ':') == NULL){
		PrintError::print("ILFS - initExtLists", "extent file format error.");
		return;
	}

	while(1){
		if(strchr(line, ':') == NULL){
			break; // Finished.
		}

		if(sscanf(line, "%d:\n", &fid) < 0)
			break;

		if(fid > MAX_LFILE_NUM || fid < 0){
			PrintError::print("ILFS - initExtLists", "fid not valid.", fid);
			return;
		}

		// Allocate space.
		extlist[fid] = (ext_t *)malloc(MAX_EXT_ENTRY_NUM * sizeof(ext_t));
		for(int i = 0; i < MAX_EXT_ENTRY_NUM; i ++){ // init
		    extlist[fid][i].logistart = -1;
		    extlist[fid][i].accessed = -1;
		}

		// Put the data in.
		int i = 0;
		while(fgets(line, 199, ifp)){
			if(strchr(line, ':') != NULL)
				break;
		    if(sscanf(line, "%ld %ld %d;\n",
		            &(extlist[fid][i].logistart),
		            &(extlist[fid][i].phystart),
		            &(extlist[fid][i].length)) < 0){
		    	line[0] = '\0'; // This file is done.
		    	break;
		    }
		    i++;
		}
        extentryNum[fid] = i;
	}
	printf("ILFS: disk data extent information read successful.\n");
	fflush(stdout);
}

int ILFS::findExtEntry(int fid, long logiaddr){
    if(extlist[fid] == NULL){
        PrintError::print("ILFS - findExtEntry","No entry registered for this file.", fid);
        return -1;
    }
    // Binary search
    int start = 0;
    int end = extentryNum[fid] - 1;
    int middle;
    if(logiaddr < 0 || logiaddr > extlist[fid][end].logistart + extlist[fid][end].length){
        PrintError::print("ILFS - findExtEntry","Logical address query is out of range", logiaddr);
        cerr << "fid=" << fid << ", end_logistart=" << extlist[fid][end].logistart
        		<< ", end_length=" << extlist[fid][end].length << endl;
        fflush(stderr);
        return -1;
    }

    if(logiaddr == extlist[fid][end].logistart + extlist[fid][end].length) // Should create a new entry.
        return extentryNum[fid];
    while(1){
        middle = (start + end) >> 1;
        if(logiaddr < extlist[fid][middle].logistart){
            end = middle - 1;
        }else if(logiaddr >= extlist[fid][middle].logistart + extlist[fid][middle].length){
            start = middle + 1;
        }else{ // Found it.
            return middle;
        }
        if(start > end){
            PrintError::print("ILFS - findExtEntry","logical address is not contiguous? FID = ", fid);
            return -1;
        }
//        cout << start << " " << end << " " << middle
//                << " " << extlist[fid][middle].logistart << " "
//                << extlist[fid][middle].logistart + extlist[fid][middle].length
//                << " " << logiaddr << endl;
    }
    return -1;
}

void ILFS::printExtLists(){
#ifdef LFS_DEBUG
	printf("Writing out extent information...\n");
	fflush(stdout);
#endif
	for(int fid = 0; fid < MAX_LFILE_NUM; fid ++){
		if(extlist[fid] == NULL)
			continue;
		fprintf(ofp, "%d:\n", fid);
		for(int exti = 0; extlist[fid][exti].logistart != -1; exti ++){
			fprintf(ofp, "%ld %ld %d;\n", extlist[fid][exti].logistart, extlist[fid][exti].phystart, extlist[fid][exti].length);
		}
	}
	fflush(ofp);
#ifdef LFS_DEBUG
	printf("Extent information written.\n");
	fflush(stdout);
#endif
}

ILFS::~ILFS() {
	printExtLists(); // Output the extent list.

	if(pageReqQ != NULL){
		pageReqQ->clear();
		delete pageReqQ;
	}

	for(int i =  0; i < MAX_LFILE_NUM; i ++){
		if(extlist[i] != NULL)
			free(extlist[i]);
	}

	if(ofp != NULL)
		fclose(ofp);
}
