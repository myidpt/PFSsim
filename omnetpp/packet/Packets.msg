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

packet bPacket extends cPacket
{
//    @customize(true);
    long ID = 0;
    long SubID = 0; // ID is not enough, currently SubID is only used on and below the VFS layer.
    int app = 0;
    int size = 0; // The number of bytes of the access.
    int clientID = -1; // The ID of the sender client.
    int subReqNum = 0; // Record the number of subrequests. Only valid on a single module.
}

packet qPacket extends bPacket
{
    int fileId = -1;
    int dsNum = 0;
    int dsList[1024];
    long dsStripeSizes[1024];
}

packet sPacket extends bPacket
{
    double tags[256];
    int application = 0;
    int length = 0;
    bool forward;
}

packet gPacket extends bPacket
{
    double risetime = 0; // The time the job is rised on the client.
    double submittime = 0; // The time the job is submitted from the client.
    double interceptiontime = 0; // The time the job is intercepted by the scheduler.
    double scheduletime = 0; // The time the job is scheduled at the scheduler.
    double arrivaltime = 0; // The time the job arrives at the data server.
    double dispatchtime = 0; // The time the job is dispatched on the data server.
    double finishtime = 0; // The finish time of the job, at disk.
    double returntime = 0; // The time the result comes back to the client.
    
    double qos_delay = 0; // The permitted delay of this job.
    
    int fileId = 0; // The file id of this access.
    long highoffset = 0; // The higher range of the offset.
    long lowoffset = 0; // The offset of the access.
    int read = 0; // write for 0 and read for 1.
    int meta = 0; // There are meta packets that does not incur actual data.
    int dsID = -1; // The data server no. that the job to be sent.
    
    long aggregateSize = 0; // The aggregate size of the total request.
}

packet PageRequest extends bPacket
{
    int fileId;
    long pageStart;
    long pageEnd;
    bool read;
    bool DIRECT = false;
    bool SYNC = false;
    bool pending = false;
}

packet BlkRequest extends bPacket
{
    int fileId;
    long blkStart;
    long blkEnd;
    bool read;
}