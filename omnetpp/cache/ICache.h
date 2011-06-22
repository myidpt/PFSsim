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

#ifndef CACHE_H_
#define CACHE_H_

#include <stdio.h>

using namespace std;

class ICache {
public:
	struct pr_type{ // page range type. unit: blk/page, 4096B.
		long long start;
		long long end; // actually "end" page is not in cache. So only pages in the range [start, end-1] are in cache.
		bool refered;
		bool modified;
		struct pr_type * next;
		pr_type(long long st, long long e, bool ref, bool mod, struct pr_type * n);
	};
protected:
	int pageSize;
	int maxPageNum;
	int curPageNum;
	struct pr_type * pageTable; // This data structure has the same effect of pagetable.
public:
	ICache(int pagesize, int maxpagenum);
	~ICache();
	// Conducts the cache reading, and return the pages that do not previously exist in the
	// cache (page fault). Always apply the entire page range to the cache, this may lead to
	// a situation that the data held in cache is temporarily bigger than the page size limit
	// because page flushing algorithm runs afterward. This is solved by calling the
	// flushCache method right after calling this method. The caller specifies the pages to
	// access in the input list.
	// <Parameters> A list of page ranges to be read.
	// <Return value> A list of page ranges where data is not in the cache.
	virtual pr_type * readCache(pr_type * pr) = 0;
	// Conducts the cache writing. Always apply the entire page range to
	// the cache, this may lead to a situation that the data held in cache is
	// temporarily bigger than the page size limit because page flushing algorithm
	// runs afterward. This is solved by calling the flushCache method right
	// after calling this method. The caller specifies the pages to access in the
	// input list.
	// <Parameters> A list of page ranges to be written.
	// <Return value> None.
	virtual void writeCache(pr_type * pr) = 0;
	// flushCache: check and if necessary, flush out some pages according to the
	// algorithm. If dirty pages are to be written back, return a list of these dirty
	// pages.This method is typically called after calling the method rwCache.
	// <Parameters> None.
	// <Return value> A list of dirty page ranges which need to be written back.
	virtual pr_type * flushCache() = 0;
	virtual void printPageTable() = 0;
	virtual int getSize(pr_type *);
	virtual int getCachedSize(pr_type *);
	static void printPRList(pr_type *);
};

#endif /* CACHE_H_ */
