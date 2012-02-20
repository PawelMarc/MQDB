//
//  MQDB test client in C++
//  Connects REQ socket to tcp://localhost:5555
//  Sends some test JS to server
//
#include <zmq.hpp>
#include <string>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>

#include <time.h>
#include <sys/time.h>


#define within(num) (int) ((float) num * random () / (RAND_MAX + 1.0))


unsigned long get_time_us() {
    struct timeval t;
    gettimeofday (&t, NULL);
    return t.tv_sec * 1000000 + t.tv_usec;
}

int main ()
{
    //  Prepare our context and socket
    zmq::context_t context (1);
    zmq::socket_t socket (context, ZMQ_REQ);
    
    std::cout << "Connecting to hello world server..." << std::endl;
    socket.connect ("tcp://localhost:5555");
    //socket.connect ("tcp://176.9.101.85:5555");

    //  Initialize random number generator
    //srandom ((unsigned) time (NULL));

    //  Start our clock now
    unsigned long total_msec = get_time_us();
    
    unsigned long resp_time_med = 0;
    unsigned long resp_time_min = 10000000;
    unsigned long resp_time_max = 0;
    
    
    int start = within(1000);
    int count = 10;
    
    //  Do 10 requests, waiting each time for a response
    for (int request_nbr = start; request_nbr != start+count; request_nbr++) {
        
        unsigned long resp_time = get_time_us();
        
        //FIX: 100 is max length
        zmq::message_t request (200);
        
        //std::istringstream iss(static_cast<char*>(request.data()));
		//iss << "SET key" << request_nbr << " val" << request_nbr;
		
		snprintf ((char *) request.data(), 200 ,
            "for(i=%d; i<%d+10; i++){ put('db/test%d','aaa%d','dane%d') }", request_nbr, request_nbr, request_nbr, request_nbr, request_nbr);
		
        //std::cout << "Sending: " << static_cast<char*>(request.data()) << "..." << std::endl;
        socket.send (request);

        //  Get the reply
        zmq::message_t reply;
        socket.recv (&reply);
        std::cout << "Received: " << static_cast<char*>(reply.data()) << std::endl;
        resp_time = get_time_us() - resp_time;
        resp_time_med += resp_time;
        resp_time_min = resp_time_min > resp_time ? resp_time : resp_time_min;
        resp_time_max = resp_time_max < resp_time ? resp_time : resp_time_max;
    }
    resp_time_med = resp_time_med/count;

    for (int request_nbr = start; request_nbr != start+count; request_nbr++) {
        //FIX: 100 is max length
        zmq::message_t request (100);
        //memcpy ((void *) request.data (), "Hello", 5);
        
        //std::istringstream iss(static_cast<char*>(request.data()));
		//iss << "SET key" << request_nbr << " val" << request_nbr;
		
		snprintf ((char *) request.data(), 200 ,
            "for(i=%d; i<%d+10; i++){ get('db/test%d','aaa%d') }", request_nbr, request_nbr, request_nbr, request_nbr);
				
        //std::cout << "Sending: " << static_cast<char*>(request.data()) << "..." << std::endl;
        socket.send (request);

        //  Get the reply
        zmq::message_t reply;
        socket.recv (&reply);
        std::cout << "Received: " << static_cast<char*>(reply.data()) << std::endl;
    }

    total_msec = (get_time_us() - total_msec)/1000;

    std::cout << "\nTotal elapsed time: " << total_msec << " msec\n" << " resp med: " << resp_time_med << " min:" << resp_time_min << " max:" << resp_time_max << std::endl;
 
    return 0;
}
