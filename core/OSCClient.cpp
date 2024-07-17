/*
 * OSCClient.cpp
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

#include <OSCClient.h>

//VIC
#include "priority_utils.h"

bool OSCClient::shouldStop = false;
int OSCClient::prioSendThread = -1;


OSCClient::OSCClient(){
	//VIC
	outBuffer = NULL;
	port = -1;
	address = NULL;
	sendThread = -1;
	verbose = 0;
}

//VIC
OSCClient::~OSCClient() {
	shouldStop = true;
}

void *OSCClient::sendQueue(void* ptr){
	OSCClient *instance = (OSCClient*)ptr;

	//VIC Set Priority
	set_priority(prioSendThread, instance->verbose);


    while(!shouldStop){
        instance->queueSend();
        usleep(1000);
    }
    return (void *)0;
}

void OSCClient::setup(int _port, const char* _address/*, bool scheduleTask VIC*/, int prio){
    address = _address;
    port = _port;

    socket.setServer(address);
	socket.setPort(port);

	/*VIC
	 if (scheduleTask)
    	createAuxTasks();*/
	prioSendThread = prio; //VIC
	pthread_create(&sendThread, NULL, sendQueue, this);

}

/*VIC
void OSCClient::createAuxTasks(){
    char name [30];
    sprintf (name, "OSCSendTask %i", port);
    OSCSendTask = Bela_createAuxiliaryTask(sendQueue, BELA_AUDIO_PRIORITY-5, name, this);
    Bela_scheduleAuxiliaryTask(OSCSendTask);
}*/

void OSCClient::queueMessage(oscpkt::Message msg){
    outQueue.push(msg);
}

void OSCClient::queueSend(){
    if (!outQueue.empty()){
        pw.init().startBundle();
        while(!outQueue.empty()){
            pw.addMessage(outQueue.front());
            outQueue.pop();
        }
        pw.endBundle();
        outBuffer = pw.packetData();
        socket.send(outBuffer, pw.packetSize());
    }
}

void OSCClient::sendMessageNow(oscpkt::Message msg){
    pw.init().addMessage(msg);
    outBuffer = pw.packetData();
    socket.send(outBuffer, pw.packetSize());
}

// OSCMessageFactory
OSCMessageFactory& OSCMessageFactory::to(std::string addr){
    msg.init(addr);
    return *this;
}

OSCMessageFactory& OSCMessageFactory::add(std::string in){
    msg.pushStr(in);
    return *this;
}
OSCMessageFactory& OSCMessageFactory::add(int in){
    msg.pushInt32(in);
    return *this;
}
OSCMessageFactory& OSCMessageFactory::add(float in){
    msg.pushFloat(in);
    return *this;
}
OSCMessageFactory& OSCMessageFactory::add(bool in){
    msg.pushBool(in);
    return *this;
}
OSCMessageFactory& OSCMessageFactory::add(void *ptr, int size){
    msg.pushBlob(ptr, size);
    return *this;
}
oscpkt::Message OSCMessageFactory::end(){
    return msg;
}
