/*
 * OSCServer.h
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

#ifndef __OSCServer_H_INCLUDED__
#define __OSCServer_H_INCLUDED__

#include "UdpServer.h"
#include "oscpkt.hh"
#include <queue>

#define UDP_RECEIVE_TIMEOUT_MS 20
#define UDP_RECEIVE_MAX_LENGTH 16384

/**
 * \brief OSCServer provides functions for receiving OSC messages in Bela.
 *
 * When an OSC message is received, the message is decoded by the OSCServer off the audio
 * thread and placed in an internal queue. This queue can be polled from the audio thread
 * using messageWaiting(), and if messages are present they can be accessed with popMessage().
 *
 * Parsing the OSC messages is left to the user
 *
 * Care must be taken to use the correct methods while running on the audio thread to
 * prevent Xenomai mode switches and audio glitches.
 *
 * Uses oscpkt (http://gruntthepeon.free.fr/oscpkt/) underneath
 */
class OSCServer{
    public:
        OSCServer();

        /**
		 * \brief Sets the port used to receive OSC messages
		 *
		 * Must be called once during setup()
		 *
		 * @param port the port used to send OSC messages
		 *
		 */
        void setup(int port, /*VIC added*/int prio=10);

        /**
		 * \brief Returns true if an OSC message has been received and queued
		 *
		 * This method is audio-thread safe, and can be used from render()
		 *
		 * Use this method to check if a message has been received before calling
		 * popMessage()
		 *
		 */
        bool messageWaiting();

        /**
		 * \brief Removes and returns the oldest message from the queue
		 *
		 * This method is audio-thread safe, and can be used from render()
		 *
		 * This function returns the oldest queued OSC message in the form of an oscpkt
		 * Message object, which must then be parsed. It also removes that message from
		 * the queue.
		 *
		 * \return oscpkt::Message an oscpkt Message object representing an OSC message
		 *
		 */
        oscpkt::Message popMessage();

        /**
		 * \brief Blocks execution until an OSC message is received
		 *
		 * This method is *not* audio-thread safe, and can *not* be used from render()
		 *
		 * This method blocks the thread's execution until an OSC message is received or
		 * timeout milliseconds have elapsed. This should never be called from render()
		 * but is useful for receiving messages during setup or an auxiliary task
		 *
		 * @param timeout the time in milliseconds to block for if no messages are received. Null will block indefinitely.
		 *
		 */
        void receiveMessageNow(int timeout);

        //VIC
        int verbose;

    private:
        int port;
        UdpServer socket;

        //AuxiliaryTask OSCReceiveTask; //VIC commented out and replaced with pthread
        pthread_t receiveThread;
        //VIC
        static int prioReceiveThread;
        static bool shouldStop;

        //void createAuxTasks(); //VIC
        void messageCheck();

        static void *checkMessages(void*);

        int inBuffer[UDP_RECEIVE_MAX_LENGTH];
        std::queue<oscpkt::Message> inQueue;
        oscpkt::Message poppedMessage;
        oscpkt::PacketReader pr;
};


#endif
