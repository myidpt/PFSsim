/*
 * schedulerFactory.h
 *
 *  Created on: Apr 23, 2013
 *      Author: Yonggang Liu
 */

#ifndef SCHEDULERFACTORY_H_
#define SCHEDULERFACTORY_H_

#include "General.h"
#include "scheduler/IQueue/IQueue.h"
#include "scheduler/FIFO/FIFO.h"
#include "scheduler/SFQ/SFQ.h"
#include "scheduler/SFQ/SSFQ.h"
#include "scheduler/DSFQ/DSFQA.h"
#include "scheduler/DSFQ/DSFQD.h"
#include "scheduler/DSFQ/DSFQF.h"
#include "scheduler/DSFQ/DSFQALB.h"
#include "scheduler/DSFQ/DSFQATB.h"
#include "scheduler/SFQRC/SFQRC.h"
#include "scheduler/I2L/I2L.h"
#include "scheduler/EDF/EDF.h"
#include "scheduler/DTA/DTA.h"

class SchedulerFactory {
public:
    static const char * FIFO_ALG;
    static const char * SFQ_ALG;
    static const char * DSFQA_ALG;
    static const char * DSFQD_ALG;
    static const char * DSFQF_ALG;
    static const char * SSFQ_ALG;
    static const char * DSFQATB_ALG;
    static const char * DSFQALB_ALG;
    static const char * SFQRC_ALG;
    static const char * I2L_ALG;
    static const char * EDF_ALG;
    static const char * DTA_ALG;

    SchedulerFactory();
    static IQueue * createScheduler(const char * algorithm, int id, int degree);
    static IQueue * createScheduler(const char * algorithm, int id, int degree,
            int numapps, const char * alg_param);
    virtual ~SchedulerFactory();
};

#endif /* SCHEDULERFACTORY_H_ */
