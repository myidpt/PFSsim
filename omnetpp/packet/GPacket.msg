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

message gPacket extends cPacket
{
    long id = 0; // App Number is 100000 based.
    
    double risetime = 0; // The time the job is rised on the client.
    double submittime = 0; // The time the job is submitted from the client.
    double arrivaltime = 0; // The time the job arrives at the data server.
    double dispatchtime = 0; // The time the job is dispatched on the data server.
    double finishtime = 0; // The finish time of the job, at disk.
    double returntime = 0; // The time the result comes back to the client.
    
    double qos_delay = 0; // The permitted delay of this job.
    
    long highoffset = 0; // The higher range of the offset.
    long lowoffset = 0; // The offset of the access.
    int size = 0; // The number of bytes of the access.
    int read = 0; // write for 0 and read for 1.
    int decision = -1; // The data server no. that the job to be sent.
    int app = 0; // Application No. for this packet.
}