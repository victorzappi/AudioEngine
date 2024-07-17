/*
 * [2-Clause BSD License]
 *
 * Copyright 2017 Victor Zappi
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * priority_utils.cpp
 *
 *  Created on: 2015-10-30
 *      Author: Victor Zappi
 */

#include "priority_utils.h"

#include <stdio.h>
#include <iostream>

#include <sys/resource.h>
#include <unistd.h> // getpid

using namespace std;



//-----------------------------------------------------------------------------------------------------------
// set priority to this thread
//-----------------------------------------------------------------------------------------------------------
void set_priority(int order, int verbose) {
	if(verbose==1)
		printf("\n*\n");

    // We'll operate on the currently running thread.
    pthread_t this_thread = pthread_self();
    // struct sched_param is used to store the scheduling priority
	struct sched_param params;
	// We'll set the priority to the maximum.
	params.sched_priority = sched_get_priority_max(SCHED_FIFO) - order;

	if(verbose==1)
		cout << "Trying to set thread realtime prio = " << params.sched_priority << endl;

	// Attempt to set thread real-time priority to the SCHED_FIFO policy
	if (pthread_setschedparam(this_thread, SCHED_FIFO, &params) != 0) {
		// Print the error
		cout << "Unsuccessful in setting thread realtime prio" << endl;
		if(verbose==1)
			printf("*\n");
		return;
	}

	// Now verify the change in thread priority
    int policy = 0;
    if (pthread_getschedparam(this_thread, &policy, &params) != 0) {
        cout << "Couldn't retrieve real-time scheduling parameters" << endl;
        if(verbose==1)
        	printf("*\n");
        return;
    }

    // Check the correct policy was applied
    if(policy != SCHED_FIFO) {
        cout << "Scheduling is NOT SCHED_FIFO!" << endl;
    } else {
    	if(verbose==1)
    		cout << "SCHED_FIFO OK" << endl;
    }

    // Print thread scheduling priority
    if(verbose==1)
    	cout << "Thread priority is " << params.sched_priority << endl;

    if(verbose==1)
    	printf("*\n");
}

void set_niceness(int niceness, int verbose) {
	if(verbose==1)
		printf("\n*\n");

	if(verbose==1)
		cout << "Trying to set thread niceness = " << niceness << endl;

	 int which = PRIO_PROCESS;
	 id_t pid = getpid();

	 if(setpriority(which, pid, -20)!=0) {
		 cout << "Unsuccessful in setting thread niceness " << endl;
		 if(verbose==1)
			 printf("*\n");
		 return;
	 }

	if(verbose==1) {
		cout << "Thread niceness is " << getpriority(which, pid) << endl;
	}

	if(verbose==1)
		printf("*\n");
}

//-----------------------------------------------------------------------------------------------------------
