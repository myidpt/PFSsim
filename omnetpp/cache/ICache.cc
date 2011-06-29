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

#include "ICache.h"

ICache::ICache(int maxpagenum) {
	maxPageNum = maxpagenum;
	curPageNum = 0;
	pageTable = NULL;
}

ICache::pr_type::pr_type(long long st, long long e, bool ref, bool mod, struct pr_type * n):
							start(st),end(e),refered(ref),modified(mod),next(n){
}

int ICache::getSize(pr_type * pr){
	int size = 0;
	while(pr != NULL){
		size += (pr->end - pr->start);
		pr = pr->next;
	}
	return size;
}

// This method does not change the pageTable,
// it returns the size (number of pages) of the requested data that resides in the cache.
int ICache::getCachedSize(pr_type * req){
	// First, set initial size to be 0.
	int size = 0;
	// Calculate the size of the intersections between req and pageTable.
	pr_type * ptIter = pageTable;
	long long start;
	long long end;
	while(ptIter != NULL && req != NULL){
		if(!(ptIter->start > req->end || ptIter->end < req->start)){// They intersect
			// Get the bigger start - intersection starts
			if(ptIter->start > req->start)
				start = ptIter->start;
			else
				start = req->start;
			// Get the smaller end -  intersection ends
			if(ptIter->end < req->end)
				end = ptIter->end;
			else
				end = req->end;
			// Increment the size by the intersection size.
			size += end - start;
			// For the next ones.
			if(ptIter->end > req->end)
				req = req->next;
			else if(ptIter->end < req->end)
				ptIter = ptIter->next;
			else{
				ptIter = ptIter->next;
				req = req->next;
			}
		}
		else if(ptIter->start > req->end){
			req = req->next;
		}
		else{
			ptIter = ptIter->next;
		}
	}
	return size;
}

void ICache::printPRList(pr_type * prp){
	printf("-------------PR LIST---------------\n");
	while(prp != NULL){
		printf("%lld %lld %d %d\n", prp->start,prp->end,prp->refered,prp->modified);
		prp = prp->next;
	}
	printf("-----------------------------------\n");
}

ICache::~ICache() {
}
