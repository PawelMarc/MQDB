#MQDB

This is very early alpha release DO NOT USE THIS!!!!

##What it does?

This is database server based on: 

* 0MQ for communication

* JavaScript V8 for scripting

* LevelDB for storage

you simply send JS source code of commands to this server and it does all the magic


##Install

To install you need to install 0MQ first

After this run make in leveldb dir

and after make in main dir

After compilation you will have 2 programs:

* mqdb -- this is server

* client -- this is sample client to test speed of server

##Using

You can use any language and platform that support [0MQ](http://http://www.zeromq.org/)

Example using C:

        zmq::message_t request (200);
        
		snprintf ((char *) request.data(), 200 ,
            "for(i=1; i<10; i++){ put('db/test','aaa','dane'+i) }");
		
        socket.send (request);

        //  Get the reply
        zmq::message_t reply;
        socket.recv (&reply);
        std::cout << "Received: " << static_cast<char*>(reply.data()) << std::endl;
        
As you can see we are simply sending JavaScript code to server using 0MQ message.
Server returns whatever our script returns or information about error.

##DB API

For now we have only two commands

* put(db_name, key, val) -- returns 0 if failed, 1 if success

* get(db_name, key) -- returns value, or 0 if failed

I will try to keep API very minimal (will add cursors, remove etc)

##Extending

MQDB can be very easily extended using JavaScript on server side

##Performance

It is difficult to compare with other systems, because you can make a lot of get/put on the server using one script.
On my laptop (4 cores, 8GB RAM) it runs client and server simultaneously with speed about 1000 scripts / second.

       



