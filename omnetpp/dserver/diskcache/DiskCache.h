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

#define BG_WRITEOUT_ID			-1 // This is the specific ID only internally used for the background write out request.
#define RA_SUBID				100000 // This is the special subID used internally for identifying the pure RA request.

#include <omnetpp.h>
#include <list>
#include <map>
#include "General.h"

using namespace std;

class DiskCache : public cSimpleModule
{
protected:
	int myID;
	static int idInit;
	static int numDiskCache;
	struct pr_t { // page range type. unit: page.
		int fileid;
		long start;
		long end; // actually "end" page is not in cache. So only pages in the range [start, end-1] are in cache.

		int _count; // the count of processes accessing the pages.
		bool PG_dirty; // when the pages are dirty, and not written back.
		bool PG_writeback; // when writing the pages from cache to disk.
		bool PG_locked; // Only when writing to the pages on cache.
		bool PG_referenced; // used by reclaim algorithm
		bool modified; // used by reclaim algorithm
		bool PG_lru; // set if the range of pages are in the LRU list.

		struct pr_t * next;
		struct pr_t * prev;

		struct pr_t * lru_next; // the next range of pages in the LRU list.
		struct pr_t * lru_prev; // the previous range of pages in the LRU list.

		struct pr_t * writeout_list; // The writeout pr list it triggers.
		struct pr_t * readahead_list; // The read-ahead pr list it triggers.

		pr_t(int fid, long st, long e, bool dirty, struct pr_t * n, struct pr_t * p);
		pr_t(pr_t * oldpr);
		static bool attrcomp(pr_t* comp1, pr_t* comp2); // compare if two prs are the same.
	};

#define RA_THRESHOLD 32
	// Not very accurate here. The real threshold is based on the real requested data size "32Blocks".
	// Here the requested data does not necessarily start from 0 offset of the block.
	struct file_ra_state {
		enum ra_pages_t{
			POSIX_FADV_NORMAL = 32, // set read-ahead maximum size to default, usually 32 pages
			POSIX_FADV_SEQUENTIAL = 64, // set read-ahead maximum size to two times the default
			POSIX_FADV_RANDOM = 0 // set read-ahead maximum size to zero, thus permanently disabling read-ahead
		};
		int ra_pages; // Maximum size in pages of a read-ahead window (0 for read-ahead permanently disabled)
		bool RA_FLAG_MISS; // when a page that has been read in advance is not found in the page cache
		// (likely because it has been reclaimed by the kernel in order to free memory)
		// in this case, the size of the next ahead window to be created is somewhat reduced
		// Currently RA_FLAG_MISS is not implemented.
		bool RA_FLAG_INCACHE; // when the last 256 pages requested by the process have all been found in the page cache
		long cache_hit; // Number of consecutive cache hits (pages requested by the process and found in the page cache)
		long prev_page; // Index of the last page requested by the process
		long start; // Index of first page in the current window
		long size; // Number of pages included in the current window (-1 for read-ahead temporarily disabled, 0 for empty current window)
		long ahead_start; // Index of the first page in the ahead window
		long ahead_size; // Number of pages in the ahead window (0 for an empty ahead window)

		bool start_IO_ahead_window; // Flag for starting the IO on the ahead window.
		long started_ra_range; // Mark the range for the upper bound of the read ahead requests already started.
		file_ra_state();
		inline void page_cache_readahead(long start, int length);
		inline void setReadahead(pr_t * pr);
		inline void update_flag_incache(long start, int length, bool hit);
	};

	struct file_t {
		int id;
		bool o_sync; // Only write: calling process is blocked until data is written to disk.
		bool o_direct; // No caching for data, all read/write directly between User Mode process and disk.
		struct file_ra_state * f_ra;
		struct pr_t * pr_head;
		struct pr_t * pr_last;
		int cached_pages;
		file_t(int id, bool o_s, bool o_d, pr_t * pr_h);
		~file_t();
	};
protected:
	long total_pages; // Number of pages.
	long total_usable_pages; // Number of pages can be used by this process.
	long free_pages; // Number of free pages.
	long dirty_pages; // Number of dirty pages.
	double dirty_ratio;
	long dirty_threshold; // dirty_ratio * total_pages
	double dirty_background_ratio;
	long dirty_background_threshold; // dirty_background_ratio * total_pages
	double dirty_expire_secs;
	int bg_writeout_active;
	bool disable_ra; // If you want to disable read ahead, set it to be true.

	int writeout_batch; // The number of pages to write back in batch when the dirty_ratio is met.
	int diskread_batch; // The number of pages to read in batch.

	// These variables work for trimCache(), which insure the function is called periodically.
	int trimCacheCounter;
	int trimCacheInt;
	// These variables work for checkHealth(), which insure the function is called periodically.
	int checkHealthCounter;
	int checkHealthInt;

#define DEFAULT_PAGE_FREE_BATCH 32

	struct pr_t * writeout_list; // The list of pr to be written out.

	struct pr_t * lru_list; // A simplified LRU list. Only one doubly-linked FIFO queue. This points to the head of the list.
	struct pr_t * lru_list_end; // The end of th LRU list, the one to be removed soon (earliest).

	struct pr_t * active_list; // List of active pages.
	struct pr_t * inactive_list; // List of inactive pages.
	long nr_scan_active; // Number of active pages to be scanned when reclaiming memory.
	long nr_scan_inactive; // Number of inactive pages to be scanned when reclaiming memory.
	long nr_active; // Number of pages in the active list.
	long nr_inactive; // Number of pages in the inactive list.
	long pages_scanned; // Counter used when doing page frame reclaiming.

	PageRequest * cacheAccess;
	PageRequest * bg_writeout;

	list<PageRequest *> * reqQ; // The queue of requests
	map<PageRequest *, pr_t *> * prs; //  The page-ranges which record the status of all queued requests.
	map<int, file_t *> * files; // list of file objects.
	int fileIDs[MAX_FILE]; // Stores all the file IDs.
	int num_files; // Number of files.

    double cache_r_speed;
    double cache_w_speed;

	virtual void initialize();
    virtual void handleMessage(cMessage *msg);
    inline void handlePageReq(PageRequest *);
    inline void handleDiskAccessResp(PageRequest *);
    inline void handleCacheAccessResp(PageRequest * resp);
    inline void handleDirtyPageExpiration(); // Called when a dirty page is expired.

    void accessCache();
    inline int readCache(PageRequest *);
    inline void writeCache(PageRequest *);
    void scheduleDiskAccess(long id, long subid, int fid, long start, long end, bool read);
    void scheduleCacheAccess(long id, long subid, int fid, long start, long end, bool read);

    void walkCache_update(const int fid, pr_t * addp, const bool preop, const bool disk);
    inline void trimAllCache();
    inline void trimCache(file_t * file);

    inline void preOpOnCache(int fid, long start, long end, bool read, bool disk);
    inline int getAccessSize(int fid, long start, long end, bool read, bool incache);

    inline pr_t * setWriteout(int fid, int n); // Continuously write out n pages.
    inline void setBGWriteoutPR(pr_t * pr); // Set the page ranges for the background write out request.

    int printCache(int fid); // For debug: print the cache information for a given file.
    inline int walkCache_countMissing(int fid, long start, long end);

    bool pagetable_delete(pr_t * pr); // Delete a page range from the page table.

    inline bool lru_delete(pr_t * pr); // Delete a page range from the lru list.
    inline void lru_push(pr_t * pr); // Push a page range to the top of the lru list.
    inline void lru_insert(pr_t * pr1, pr_t * pr2); // Insert pr1 before or after pr2, depending on the page ranges.
    inline void lru_substitute(pr_t * pr1, pr_t * pr2); // substitute pr2 with pr1.
    inline bool lru_free_pages(int num); // Reclaim a specified number of pages.
    inline bool lru_free_pages(); // Reclaim a default number of pages: 32.
    int printLRU();

    inline bool checkListHealth(); // Check if the doubly linked list for files and LRU is good.

    void finish();
    ~DiskCache();
};

#endif
