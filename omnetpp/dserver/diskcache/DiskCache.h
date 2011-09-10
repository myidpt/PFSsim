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

#ifndef _DISKCACHE_H_
#define _DISKCACHE_H_

#include <omnetpp.h>
#include <list>
#include <map>
#include "General.h"
using namespace std;

class DiskCache : public cSimpleModule
{
public:
	struct pr_t { // page range type. unit: page, 4096B.
		int fileid;
		long start;
		long end; // actually "end" page is not in cache. So only pages in the range [start, end-1] are in cache.
		int _count; // the count of processes accessing the pages.
		bool PG_dirty; // when the pages are dirty, and not written back.
		bool PG_writeback; // when writing the pages from cache to disk.
		bool PG_locked; // Only when writing to the pages on cache.
		bool refered; // used by reclaim algorithm
		bool modified; // used by reclaim algorithm
		struct pr_t * next;
		struct pr_t * prev;
		pr_t(int fid, long st, long e, bool dirty, struct pr_t * n, struct pr_t * p);
		pr_t(pr_t * oldpr);
		static bool attrcomp(pr_t* comp1, pr_t* comp2); // compare if two prs are the same.
	};

	struct file_ra_state {
		enum ra_pages_t{
			POSIX_FADV_NORMAL = 32, // set read-ahead maximum size to default, usually 32 pages
			POSIX_FADV_SEQUENTIAL = 64, // set read-ahead maximum size to two times the default
			POSIX_FADV_RANDOM = 0 // set read-ahead maximum size to zero, thus permanently disabling read-ahead
		};
		int ra_pages; // Maximum size in pages of a read-ahead window (0 for read-ahead permanently disabled)
		bool RA_FLAG_MISS; // when a page that has been read in advance is not found in the page cache
		// (likely because it has been reclaimed by the kernel in order to free memory)
		// in this case, the size of = false the next ahead window to be created is somewhat reduced
		bool RA_FLAG_INCACHE; // when the last 256 pages requested by the process have all been found in the page cache
		// the value of consecutive cache hits is stored in the ra->cache_hit field
		long cache_hit; // Number of consecutive cache hits (pages requested by the process and found in the page cache)
		long prev_page; // Index of the last page requested by the process
		long start; // Index of first page in the current window
		long size; // Number of pages included in the current window (-1 for read-ahead temporarily disabled, 0 for empty current window)
		long ahead_start; // Index of the first page in the ahead window
		long ahead_size; // Number of pages in the ahead window (0 for an empty ahead window)
		file_ra_state();
	};

	struct file_t {
		int id;
		bool o_sync; // Only write: calling process is blocked until data is written to disk.
		bool o_direct; // No caching for data, all read/write directly between User Mode process and disk.
		struct file_ra_state * f_ra;
		struct pr_t * pr_head;
		int cached_pages;
		file_t(int id, bool o_s, bool o_d, pr_t * pr_h);
		~file_t();
	};
protected:
	long total_pages; // Number of pages.
	long free_pages; // Number of free pages.
	long dirty_pages; // Number of dirty pages.
	double dirty_ratio;
	double dirty_background_ratio;

	struct pr_t * active_list; // List of active pages.
	struct pr_t * inactive_list; // List of inactive pages.
	long nr_scan_active; // Number of active pages to be scanned when reclaiming memory.
	long nr_scan_inactive; // Number of inactive pages to be scanned when reclaiming memory.
	long nr_active; // Number of pages in the active list.
	long nr_inactive; // Number of pages in the inactive list.
	long pages_scanned; // Counter used when doing page frame reclaiming.

	PageRequest * cacheAccess;

	list<PageRequest *> * reqQ; // The queue of requests
	map<PageRequest *, pr_t *> * prs; //  The page-ranges which record the status of all queued requests.
	map<int, file_t *> * files; // list of file objects.

    double cache_r_speed;
    double cache_w_speed;

	virtual void initialize();
    virtual void handleMessage(cMessage *msg);
    void handlePageReq(PageRequest *);
    void handleDiskAccessResp(PageRequest *);
    void handleCacheAccessResp(PageRequest * resp);

    void accessCache();
    void readCache(PageRequest *);
    void writeCache(PageRequest *);
    void scheduleDiskAccess(long id, int fid, long start, long end, bool read);
    void scheduleCacheAccess(long id, int fid, long start, long end, bool read);

    void walkCache_update(const int fid, pr_t * addp, const bool preop, const bool disk);
    void trimCache(int fid);

    void cacheAccessPreop(int fid, long start, long end, bool read, bool disk);
    int getAccessSize(int fid, long start, long end, bool read, bool incache);

    void printCache(int fid); // For debug: print the cache information for a given file.
    int walkCache_countMissing(int fid, pr_t * addp);

    void finish();
};

#endif
