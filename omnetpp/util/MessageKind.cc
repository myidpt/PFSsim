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

#include "MessageKind.h"

#define CASE(X) \
	case X: \
	return #X;

string MessageKind::getMessageKindString(short kind) {
	switch (kind) {
	CASE(SELF_EVENT)
	CASE(LAYOUT_REQ)
	CASE(LAYOUT_RESP)
	CASE(LFILE_REQ)
	CASE(BLK_REQ)
	CASE(BLK_RESP)
	CASE(LFILE_RESP)
	CASE(SCH_JOB)
	CASE(TRC_SYN)
	CASE(PAGE_REQ)
	CASE(PAGE_RESP)
	CASE(PROP_SCH)
	CASE(DISK_REQ)
	CASE(DISK_RESP)
	CASE(TRACE_REQ)
	CASE(TRACE_RESP)
	CASE(PFS_W_REQ)
	CASE(PFS_R_REQ)
	CASE(PFS_W_RESP)
	CASE(PFS_W_DATA)
	CASE(PFS_W_DATA_LAST)
	CASE(PFS_W_FIN)
	CASE(PFS_R_DATA)
	CASE(PFS_R_DATA_LAST)
	CASE(SELF_PFS_W_REQ)
	CASE(SELF_PFS_R_REQ)
	CASE(SELF_PFS_W_RESP)
	CASE(SELF_PFS_W_DATA)
	CASE(SELF_PFS_W_DATA_LAST)
	CASE(SELF_PFS_W_FIN)
	CASE(SELF_PFS_R_DATA)
	CASE(SELF_PFS_R_DATA_LAST)
	default:
		cout << "Not recorded Message kind) " << kind << endl;
	}
	return NULL;
}

MessageKind::MessageKind() {

}

MessageKind::~MessageKind() {
}
