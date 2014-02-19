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

#ifndef DTAEDF_H_
#define DTAEDF_H_

#include <map>
#include "EDF.h"

class DTA_EDF : public EDF {
public:
    DTA_EDF(int id, int deg, int ttlapps, double delay[]);
    void refineWaitQ(map<long, bPacket *> * refinemap); // Delete the bPackets that are not in the parameter list.
    virtual ~DTA_EDF();
};

#endif /* DTAEDF_H_ */
