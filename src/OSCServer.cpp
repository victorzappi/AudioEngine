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
 * OSCServer.cpp
 *
 *      Author: Giulio Moro
 *
 *      Taken from Bela source code
 *
 * Modified on: 20 Sept 2017
 *  	Author: Victor Zappi
 *
 *  	Bela: an embedded platform for ultra-low latency audio and sensor processing
 *		http://bela.io
 *
 *		A project of the Augmented Instruments Laboratory within the
 *		Centre for Digital Music at Queen Mary University of London.
 *		http://www.eecs.qmul.ac.uk/~andrewm
 *
 *		(c) 2016 Augmented Instruments Laboratory: Andrew McPherson,
 *       Astrid Bin, Liam Donovan, Christian Heinrichs, Robert Jack,
 *		 Giulio Moro, Laurel Pardue, Victor Zappi. All rights reserved.
 *
 *		The Bela software is distributed under the GNU Lesser General Public License
 *		(LGPL 3.0), available here: https://www.gnu.org/licenses/lgpl-3.0.txt
 *
 */

#include <OSCServer.h>

//VIC
#include "priority_utils.h"

bool OSCServer::shouldStop = false;
int OSCServer::prioReceiveThread = -1;

// constructor
OSCServer::OSCServer(){
	port = -1;
	receiveThread = -1;
	verbose = 0;
}

// static method for checking messages
// called by messageCheckTask with pointer to OSCServer instance as argument
void *OSCServer::checkMessages(void* ptr){
    OSCServer *instance = (OSCServer*)ptr;

    //VIC Set Priority
    set_priority(prioReceiveThread, instance->verbose);

    while(!shouldStop){
        instance->messageCheck();
        usleep(1000);
    }
    return (void *)0;
}

void OSCServer::setup(int _port, /*VIC added*/int prio){
    port = _port;
    if(!socket.init(port))
        printf("socket not initialised\n");
    //createAuxTasks(); //VIC

    prioReceiveThread = prio; //VIC
    pthread_create(&receiveThread, NULL, checkMessages, this);
}

/*VIC
void OSCServer::createAuxTasks(){
    char name [30];
    sprintf (name, "OSCReceiveTask %i", port);
    OSCReceiveTask = Bela_createAuxiliaryTask(OSCServer::checkMessages, BELA_AUDIO_PRIORITY-5, name, this);
    Bela_scheduleAuxiliaryTask(OSCReceiveTask);
}*/

void OSCServer::messageCheck(){
    if (socket.waitUntilReady(true, UDP_RECEIVE_TIMEOUT_MS)){
        int msgLength = socket.read(&inBuffer, UDP_RECEIVE_MAX_LENGTH, false);
        pr.init(inBuffer, msgLength);
        oscpkt::Message *inmsg;
        while (pr.isOk() && (inmsg = pr.popMessage()) != 0) {
            inQueue.push(*inmsg);
        }
    }
}

bool OSCServer::messageWaiting(){
    return !inQueue.empty();
}

oscpkt::Message OSCServer::popMessage(){
    if (!inQueue.empty()){
        poppedMessage = inQueue.front();
        inQueue.pop();
    } else {
        poppedMessage.init("/error");
    }
    return poppedMessage;
}

void OSCServer::receiveMessageNow(int timeout){
    if (socket.waitUntilReady(true, timeout)){
        int msgLength = socket.read(&inBuffer, UDP_RECEIVE_MAX_LENGTH, false);
        pr.init(inBuffer, msgLength);
        oscpkt::Message *inmsg;
        while (pr.isOk() && (inmsg = pr.popMessage()) != 0) {
            inQueue.push(*inmsg);
        }
    }
}



