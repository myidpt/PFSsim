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

#include "scheduler/IQueue.h"

IQueue::IQueue(int id){
	myid = id;
	char fname[24] = {'s','c','h','e','d','u','l','e','i','n','f','o','/',
			's','c','h','e','d','u','l','e','0','0','\0'};
	fname[21] = id/10 + '0';
	fname[22] = id%10 + '0';
	if((sfile = fopen(fname,"w")) == NULL){
		fprintf(stderr, "ERROR: Can't open schedule information file.\n");
	}
	srand((unsigned)time(0));
}

IQueue::~IQueue(){
	if(sfile != NULL)
		fclose(sfile);
}
