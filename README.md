#MQDB

This is very early alpha release DO NOT USE THIS!!!!

##What it does?

This is database server based on: 

* 0MQ for communication

* JavaScript V8 for scripting

* LevelDB for storage
* WAH bitmaps for indecies

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

All functions take name of the database as first argument. It is path to catalog when leveldb will put data. If database does not exist it will be created.
In case of some error exception will be raised. If you want to create database "/a/b/s/some_db" the path "/a/b/s/" must exist. 

* put(db_name, key, val) -- returns 1 if success, throws exception in case of error

* get(db_name, key) -- returns value, or Null if key does not exist, in case of other errors throws exception

* del(db_name, key) -- returns 1 if success, throws exception in case of error

###Iterator API

* it_new(db_name) -- returns handle to iterator object for use in other functions, you do not need to deallocate the objects, they are automatically removed after script is executed

* it_first(it) -- seeks to first element returns 1 if success, throws exception in case of error

* it_last(it) -- seeks to last element returns 1 if success, throws exception in case of error

* it_seek(it, val) -- seeks to element equal or greater then val returns 1 if success, throws exception in case of error

* it_next(it) -- moves to next element returns 1 if success, throws exception in case of error

* it_prev(it) -- moves to prev element returns 1 if success, throws exception in case of error

* it_valid(it) -- returns 0 if iterator is no longer valid

* it_val(it) -- returns current value as string or throws exception

* it_key(it) -- returns current key as string or throws exception

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

This code returns all values with keys between "aaa2" and "aaa4"

        var ret = []; 
        var i = 0; 
        var it = it_new('db/testb'); 
        for(it_seek(it, 'aaa2'); it_valid(it) && it_key(it)<'aaa4'; it_next(it)){ 
            ret[i++] = {'key': it_key(it), 'val': it_val(it)}; 
        }; 
        JSON.stringify(ret)
                
##Extending

MQDB can be very easily extended using JavaScript on server side

##Performance

It is difficult to compare with other systems, because you can make a lot of get/put on the server using one script.
On my laptop (4 cores, 8GB RAM) it runs client and server simultaneously with speed about 1000 scripts / second.

       



