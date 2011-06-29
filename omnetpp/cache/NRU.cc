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

#include "NRU.h"

NRU::NRU(int pn):ICache(pn){
	prePageTable = new struct ICache::pr_type
		(-200, -100, false, false, pageTable);
	// a pr that has a next pointer pointing to the page table list. Only used for simplicity of some functions.
	resetRefFlag();
}

struct ICache::pr_type * NRU::rwCache(struct pr_type * nlp){
	struct pr_type * preblp, * blp, * tmp, * tmp2, * pflist, * pflistp;
	bool processed = false;
	preblp = prePageTable;
	blp = pageTable;
	pflist = new struct pr_type
			(-200, -100, false, false, NULL); // page fault list. List head meaningless.
	pflistp = pflist;

	// In reality, page fault only exists in page reading.
	// But we count the "page fault" list for both read and write here.
	// For "write", it means the written content to a page not existing in the cache.
	while(nlp != NULL){

		if(nlp->start < preblp->end){ // {---]...[
			processed = true;
			if((preblp->refered == nlp->refered && preblp->modified == nlp->modified)
				|(preblp->refered && preblp->modified)){
				// Do not need to modify.
			}else if(nlp->start == preblp->start){ // [{----]
				if(nlp->end < preblp->end){ // [{--}-] --> [--][-]
					tmp = new struct pr_type // This part keeps the same.
						(nlp->end,preblp->end,preblp->refered,preblp->modified,preblp->next);

					preblp->end = nlp->end;
					preblp->refered = nlp->refered; // must be true
					preblp->modified |= nlp->modified; // If either one is true, it is true.
					preblp->next = tmp;

					preblp = tmp;
				}else{ // [{---]..} --> [---]--} No need to create new
					preblp->refered = nlp->refered;
					preblp->modified = nlp->modified|preblp->modified;
				}
			}else{ // [---{---]
				if(nlp->end < preblp->end){ // [---{---}-]---[ --> [---][---][-]---[
					tmp2 = new struct pr_type
						(nlp->end,preblp->end,preblp->refered,preblp->modified,blp);
					tmp = new struct pr_type
						(nlp->start,nlp->end,nlp->refered,preblp->modified|nlp->modified,tmp2);
					preblp->end = tmp->start;
					preblp->next = tmp;
					preblp = tmp2;
				}else{ // [---{---]...} --> [---][---]...}
					tmp = new struct pr_type
						(nlp->start, preblp->end, nlp->refered,
								preblp->modified|nlp->modified,blp);
					preblp->end = tmp->start;
					preblp->next = tmp;
					preblp = tmp;
				}
			}

			nlp->start = preblp->end;
			if(nlp->start >= nlp->end){
				tmp = nlp;
				nlp = nlp->next;
				delete tmp;
			}
		}

		if(nlp == NULL)
			break;
		if(blp == NULL){ // All of the rest nlps are page faults.
			if(!processed){
				preblp->next = nlp;
				while(nlp != NULL){ // Copy a list of nlp to pflist.
					tmp = new struct pr_type
						(nlp->start,nlp->end,nlp->refered,nlp->modified,NULL);
					pflistp->next = tmp;
					pflistp = pflistp->next;
					nlp = nlp->next;
				}
				break;
			}else{
				processed = false;
				continue;
			}
		}

		if(nlp->start < blp->start && nlp->start >= preblp->end){ // ]..{---[
			processed = true;
			tmp = new struct pr_type(nlp->start,(nlp->end < blp->start ? nlp->end : blp->start),
					nlp->refered, nlp->modified, blp);
			tmp2 = new struct pr_type(nlp->start,(nlp->end < blp->start ? nlp->end : blp->start),
					nlp->refered, nlp->modified, NULL);
			preblp->next = tmp;
			preblp = tmp;
			pflistp->next = tmp2;
			pflistp = pflistp->next;

			nlp->start = blp->start;
			if(nlp->start >= nlp->end){ // This nlp is done, move to the next one!
				tmp = nlp;
				nlp = nlp->next;
				delete tmp;
			}
		}
		if(processed == false){
			preblp = blp;
			blp = blp->next;
		}else{
			processed = false;
		}
	}
	pageTable = prePageTable->next;
	curPageNum = mergePTandGetSize();
//	printPageTable();
	// Delete the meaningless head of the page fault list.
	tmp = pflist;
	pflist = pflist->next;
	delete tmp;
	return pflist;
}

void NRU::resetRefFlag(){
	pr_type * ptp = pageTable;
	while(ptp != NULL){
		ptp->refered = false;
		ptp = ptp->next;
	}
	mergePTandGetSize();
}

// Runs the algorithm, and returns the dirty pages that need to be written back.
struct ICache::pr_type * NRU::flushCache(){
	// Class 1 : 0/0	ref/mod
	// Class 2 : 0/1
	// Class 3 : 1/0
	// Class 4 : 1/1
//	printf("NRU::flushCache:----cur = %d, max = %d------\n", curPageNum, maxPageNum);
	if(curPageNum <= maxPageNum){
		return NULL;
	}
	struct pr_type * wblist, * wblistp, * preptp, * ptp, * tmp; // write back list, page table pointer
	wblist = new struct pr_type(-200, -100, false, false, NULL);
	wblistp = wblist;
	bool refflag, modflag;
	refflag = false;
	modflag = false;
	while(true){
		preptp = prePageTable;
		ptp = pageTable;
		while(ptp != NULL){
			if(curPageNum <= maxPageNum){
				break;
			}
			if(ptp->refered == refflag && ptp->modified == modflag){
				if((curPageNum-maxPageNum) >= int(ptp->end - ptp->start)){
					curPageNum -= int(ptp->end - ptp->start);
					wblistp->next = ptp;
					wblistp = ptp;
					preptp->next = ptp->next;
					ptp = preptp->next;
					wblistp->next = NULL;
				}else{
					tmp = new struct pr_type(ptp->start,
						ptp->start + (curPageNum-maxPageNum),
						ptp->refered,ptp->modified, NULL);
					wblistp->next = tmp;
					wblistp = tmp;
					ptp->start += curPageNum - maxPageNum;
					curPageNum = maxPageNum;
				}
			}else{
				preptp = ptp;
				ptp = ptp->next;
			}
		}
		if(curPageNum <= maxPageNum)
			break;
		if(modflag == false){
			modflag = true;
		}else if(refflag == false){
			refflag = true;
			modflag = false;
		}
	}

	tmp = wblist;
	wblist = wblist->next;
	delete tmp; // Delete the meaningless head.
	return wblist;
}


struct ICache::pr_type * NRU::readCache(struct pr_type * pr){
	return rwCache(pr);
}
void NRU::writeCache(struct pr_type * pr){
	rwCache(pr);
	return;
}

/*
 * merge the list and calculate the memory size.
 */
long NRU::mergePTandGetSize(){
	pr_type * preblp, * blp;
	long size = 0;
	if(pageTable == NULL)
		return 0;
	preblp = prePageTable;
	blp = pageTable;
	while(blp != NULL){
		size += blp->end - blp->start;
		if(preblp->end == blp->start
				&& preblp->refered == blp->refered
				&& preblp->modified == blp->modified){
			preblp->end = blp->end;
			preblp->next = blp->next;
			delete blp;
			blp = preblp->next;
		}else{
			preblp = blp;
			blp = blp->next;
		}
	}
	return size;
}

void NRU::printPageTable(){
	pr_type * ptp = pageTable;
	printf("------------PAGE TABLE-------------\n");
	while(ptp != NULL){
		printf("%lld %lld %d %d\n", ptp->start,ptp->end,ptp->refered,ptp->modified);
		ptp = ptp->next;
	}
	printf("-----------------------------------\n");
}

NRU::~NRU() {
}
