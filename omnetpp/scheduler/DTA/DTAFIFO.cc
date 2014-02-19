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

#include "DTAFIFO.h"

DTA_FIFO::DTA_FIFO(int id, int deg) : FIFO(id, deg) {
}

void DTA_FIFO::refineWaitQ(map<long, bPacket *> * refinemap) {
    bool d = true;
    while(d == true) {
        d = false;
        for (list<bPacket *>::iterator it = waitQ->begin();
                it != waitQ->end(); it ++) {
            if (refinemap->find((*it)->getID())
                    == refinemap->end()) { // It doesn't exist.
#ifdef SCH_DEBUG
                cout << (*it)->getID() << " does not exist in FIFO queue." << endl;
#endif
                waitQ->erase(it);
                d = true;
                break;
            }
        }
    }
}

DTA_FIFO::~DTA_FIFO() {
}

