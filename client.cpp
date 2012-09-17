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

unsigned long get_time_us() {
    struct timeval t;
    gettimeofday (&t, NULL);
    return t.tv_sec * 1000000 + t.tv_usec;
}

#define within(num) (int) ((float) num * random () / (RAND_MAX + 1.0))


int main ()
{
    //  Prepare our context and socket
    zmq::context_t context (1);
    zmq::socket_t socket (context, ZMQ_REQ);
    
    int rc = 0;
    std::cout << "Connecting to hello world server..." << std::endl;
    socket.connect ("tcp://localhost:5555");
    //socket.connect ("epgm://eth0;127.0.0.1:5555");
    //socket.connect("ipc:///tmp/feeds_mq");
    //socket.connect ("tcp://176.9.101.85:5555");
    assert(rc == 0);
    
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
        zmq::message_t request (2000);
        
        //std::istringstream iss(static_cast<char*>(request.data()));
		//iss << "SET key" << request_nbr << " val" << request_nbr;
		
		snprintf ((char *) request.data(), 2000 ,
		    "put('db/testx','aaa','dane'); get('db/testx','aaa'); ");
            //"for(i=%d; i<%d+10; i++){ put('db/testb','aaa'+i,'dane'+i) }", request_nbr, request_nbr);
		
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

#if 0
    for (int request_nbr = start; request_nbr != start+1; request_nbr++) {
        //FIX: 100 is max length
        zmq::message_t request (2000);
        //memcpy ((void *) request.data (), "Hello", 5);
        
        //std::istringstream iss(static_cast<char*>(request.data()));
		//iss << "SET key" << request_nbr << " val" << request_nbr;
		
		snprintf ((char *) request.data(), 2000,
		"var ret = []; var i = 0; var it = it_new('db/testb'); for(it_seek(it, 'aaa2'); it_valid(it) && it_key(it)<'aaa4'; it_next(it)){ ret[i++] = {'key': it_key(it), 'val': it_val(it)}; }; JSON.stringify(ret)"
		//"var ret = []; var i = 0; var it = it_new('db/testb'); for(it_first(it); it_valid(it); it_next(it)){ ret[i++] = {'key': it_key(it), 'val': it_val(it)}; };"
		//"for(it_last(it); it_valid(it); it_prev(it)){ ret[i++] = {'key': it_key(it), 'val': it_val(it)}; del('db/testb', it_key(it)) }; JSON.stringify(ret)" 
		); 
		// var off = %d; for(i=0; i<10; i++){ var key = 'aaa'+(i+off); ret[i] = {'key': key, 'get': get('db/test', key)}; del('db/test', key) }; JSON.stringify(ret)", request_nbr);
		 //"var ret = []; var off = %d; for(i=0; i<10; i++){ var key = 'aaa'+(i+off); ret[i] = {'key': key, 'get': get('db/test', key)}; del('db/test', key) }; JSON.stringify(ret)", request_nbr);
				
        //std::cout << "Sending: " << static_cast<char*>(request.data()) << "..." << std::endl;
        socket.send (request);

        //  Get the reply
        zmq::message_t reply;
        socket.recv (&reply);
        std::cout << "Received: " << static_cast<char*>(reply.data()) << std::endl;
    }
#endif

    total_msec = (get_time_us() - total_msec)/1000;

    std::cout << "\nTotal elapsed time: " << total_msec << " msec\n" << " resp med: " << resp_time_med << " min:" << resp_time_min << " max:" << resp_time_max << std::endl;
 
    return 0;
}
