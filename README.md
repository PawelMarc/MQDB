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

* get(db_name, key) -- returns value, or Null if failed

* del(db_name, key) -- returns 0 if failed, 1 if success

#Iterator API

* it_new(db_name) -- returns handle to it object for use in other functions, you do not need to dealocate the objects, it is automaticaly removed after script is executed

* it_first(it) -- seeks to first element returns 0 if failed, 1 if success

* it_next(it) -- moves to next element returns 0 if failed, 1 if success

* it_valid(it) -- returns 0 if iterator is no longer valid

* it_val(it) -- returns current value as string

* it_key(it) -- returns current key as string

* it_del(it) -- if you like you can remove iterator by hand (do not need to)

This code returns all values in database and removes them all while reading

        var ret = []; 
        var i = 0; 
        var it = it_new('db/testb'); 
        for(it_first(it); it_valid(it); it_next(it)){ 
            ret[i++] = {'key': it_key(it), 'val': it_val(it)}; 
            del('db/testb', it_key(it)) 
        }; 
        JSON.stringify(ret)
        
##Extending

MQDB can be very easily extended using JavaScript on server side

##Performance

It is difficult to compare with other systems, because you can make a lot of get/put on the server using one script.
On my laptop (4 cores, 8GB RAM) it runs client and server simultaneously with speed about 1000 scripts / second.

       



