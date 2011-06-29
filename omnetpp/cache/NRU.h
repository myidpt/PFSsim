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

#ifndef NRU_H_
#define NRU_H_

#include "ICache.h"

class NRU: public ICache {
private:
	struct ICache::pr_type * rwCache(struct pr_type * pr);
	ICache::pr_type * prePageTable; // Only useful internally. No meaning.
public:
	NRU(int);
	virtual struct ICache::pr_type * readCache(struct pr_type * pr);
	virtual void writeCache(struct pr_type * pr);
	virtual struct pr_type * flushCache();
	virtual long mergePTandGetSize();
	virtual void resetRefFlag();
	void printPageTable();
	virtual ~NRU();
};

#endif /* NRU_H_ */
