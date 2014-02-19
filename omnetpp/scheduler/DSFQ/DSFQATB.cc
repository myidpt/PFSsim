// Yonggang Liu

#include "DSFQATB.h"

// Alg_param format: [id weight] [id weight] [id weight] ... | Time_threshold
DSFQATB::DSFQATB(int id, int deg, int totalapp, const char * alg_param) : DSFQA(id, deg, totalapp, alg_param) {
    processParameter(alg_param);
}

double DSFQATB::notify() {
    return SIMTIME_DBL(simTime()) + time_threshold;
}

void DSFQATB::processParameter(const char * alg_param) {
    const char * wtstr = strchr(alg_param, '|');
    if(wtstr == NULL){
        PrintError::print("DSFQALB", myID, "Can't find the time threshold from the input parameter.");
        return;
    }
    wtstr ++;
    sscanf(wtstr, "%lf", &time_threshold);
}

DSFQATB::~DSFQATB() {
}
