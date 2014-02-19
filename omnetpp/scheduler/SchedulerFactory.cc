/*
 * schedulerFactory.cc
 *
 *  Created on: Apr 23, 2013
 *      Author: Yonggang Liu
 */

#include "SchedulerFactory.h"

const char * SchedulerFactory::FIFO_ALG = "FIFO";
const char * SchedulerFactory::SFQ_ALG = "SFQ";
const char * SchedulerFactory::DSFQA_ALG = "DSFQA";
const char * SchedulerFactory::DSFQD_ALG = "DSFQD";
const char * SchedulerFactory::DSFQF_ALG = "DSFQF";
const char * SchedulerFactory::SSFQ_ALG = "SSFQ";
const char * SchedulerFactory::DSFQATB_ALG = "DSFQATB";
const char * SchedulerFactory::DSFQALB_ALG = "DSFQALB";
const char * SchedulerFactory::SFQRC_ALG = "SFQRC";
const char * SchedulerFactory::I2L_ALG = "I2L";
const char * SchedulerFactory::EDF_ALG = "EDF";
const char * SchedulerFactory::DTA_ALG = "DTA";

SchedulerFactory::SchedulerFactory() {
}

IQueue * SchedulerFactory::createScheduler(const char * algorithm, int id, int degree) {
    IQueue * queue = NULL;
    if (!strcmp(algorithm, FIFO_ALG)) {
        cout << "FIFO(" << degree << ")." << endl;
        queue = new FIFO(id, degree);
    }
    else {
        PrintError::print("Proxy",
                "Undefined algorithm for createScheduler(const char *, int, int).");
    }
    return queue;
}

IQueue * SchedulerFactory::createScheduler(const char * algorithm, int id, int degree,
        int numapps, const char * alg_param) {
    IQueue * queue = NULL;
    if (!strcmp(algorithm, FIFO_ALG)) {
        cout << "FIFO(" << degree << ")." << endl;
        queue = new FIFO(id, degree);
    }
    else if (!strcmp(algorithm, SFQ_ALG)) {
        cout << "SFQ(" << degree << ")";
        queue = new SFQ(id, degree, numapps, alg_param);
    }
    else if (!strcmp(algorithm, DSFQA_ALG)) {
        cout << "DSFQA(" << degree << ")";
        queue = new DSFQA(id, degree, numapps, alg_param);
    }
    else if (!strcmp(algorithm, DSFQD_ALG)) {
        cout << "DSFQD(" << degree << ")";
        queue = new DSFQD(id, degree, numapps, alg_param);
    }
    else if (!strcmp(algorithm, DSFQF_ALG)) {
        cout << "DSFQF(" << degree << ")";
        queue = new DSFQF(id, degree, numapps, alg_param);
    }
    else if (!strcmp(algorithm, DSFQATB_ALG)) {
        cout << "DSFQA(" << degree << ")-TimeBased";
        queue = new DSFQATB(id, degree, numapps, alg_param);
    }
    else if (!strcmp(algorithm, DSFQALB_ALG)) {
        cout << "DSFQA(" << degree << ")-LoadBased";
        queue = new DSFQALB(id, degree, numapps, alg_param);
    }
    else if (!strcmp(algorithm, SFQRC_ALG)) {
        cout << "SFQRC(" << degree << ")";
        queue = new SFQRC(id, degree, numapps, alg_param);
    }
    else if (!strcmp(algorithm, I2L_ALG)) {
        cout << "I2L(" << degree << ")";
        queue = new I2L(id, degree, numapps, alg_param);
    }
    else if (!strcmp(algorithm, EDF_ALG)) {
        cout << "EDF(" << degree << ")";
        queue = new EDF(id, degree, numapps, alg_param);
    }
    else if (!strcmp(algorithm, DTA_ALG)) {
        cout << "DTA(" << degree << ")";
        queue = new DTA(id, degree, numapps, alg_param);
    }
    else {
        PrintError::print("Proxy", "Undefined algorithm.");
    }
    return queue;
}

SchedulerFactory::~SchedulerFactory() {
}

