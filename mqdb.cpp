/********************************
/  MQDB 
/  very simple server using V8 0MQ and LevelDB
/  BSD License
/
/**********************************/


#include <ucontext.h>
#include <stdlib.h>
#include <v8.h>
#include <pthread.h>
#include <assert.h>
#include <iostream>
#include <string>
#include <sstream>
#include <zmq.hpp>
#include <unistd.h>
#include "leveldb/db.h"

struct dbcontext {
    void *context;
    leveldb::DB* db;
};

//global context
//TODO: change to map
struct dbcontext *gcontext;

using namespace std;
using namespace v8;

#define NUM_THREADS     5

#define JSVAL Handle<Value>
#define JSOBJ Handle<Object>
#define JSOBJT Handle<ObjectTemplate>
#define JSARRAY Handle<Array>
#define JSARGS const Arguments&

static JSVAL addnum(JSARGS args) {
    HandleScope scope;
    int x = args[0]->IntegerValue();
    int y = args[1]->IntegerValue();
    return scope.Close(Integer::New(x+y));
}


leveldb::DB* getDb(String::Utf8Value &name) {
    return gcontext->db;
}

static JSVAL put(JSARGS args) {
    HandleScope scope;
    String::Utf8Value dbname(args[0]->ToString());
    String::Utf8Value key(args[1]->ToString());
    String::Utf8Value val(args[1]->ToString());
    leveldb::DB* db = getDb(dbname);
    leveldb::Status s;
    s = db->Put(leveldb::WriteOptions(), *key, *val);
    if (!s.ok()) return scope.Close(Integer::New(0)); //std::cerr << "ERROR: " << s.ToString() << std::endl;
    return scope.Close(Integer::New(1));
}

static JSVAL get(JSARGS args) {
    HandleScope scope;
    String::Utf8Value dbname(args[0]->ToString());
    String::Utf8Value key(args[1]->ToString());
    std::string val;
    leveldb::DB* db = getDb(dbname);
    leveldb::Status s;
    s = db->Get(leveldb::ReadOptions(), *key, &val);
    if (!s.ok()) return scope.Close(Integer::New(0)); //std::cerr << "ERROR: " << s.ToString() << std::endl;
    Handle<String> res = String::New(val.c_str());
    return scope.Close(res);
}

        
void* worker_routine(void *vdbc) {

    struct dbcontext * dbc = (struct dbcontext *) vdbc;
    
    //  Socket to talk to dispatcher
    void *receiver = zmq_socket (dbc->context, ZMQ_REP);
    zmq_connect (receiver, "inproc://workers");

    //Initialize our Isoloate
    Isolate *isolate = Isolate::New();
    if(!isolate)
    {
        cout << "Failed to initialize Isolate, we can't work";
        return 0;
    }
    
    Locker lock(isolate);
    Isolate::Scope isolateScope(isolate);
    HandleScope scope;
    
    Local<ObjectTemplate> globals = ObjectTemplate::New();
    
    globals->Set(String::New("addnum"), FunctionTemplate::New(addnum));
    globals->Set(String::New("put"), FunctionTemplate::New(put));
    globals->Set(String::New("get"), FunctionTemplate::New(get));


    Handle<Context> context = Context::New(NULL, globals);
    context->Enter();
    
    while (1) {
        
        zmq_msg_t req;
        int rc = zmq_msg_init (&req);
        assert (rc == 0);
        rc = zmq_recv (receiver, &req, 0);
        assert (rc == 0);
        
        //std::cout << "thread: " << pthread_self() << " Received script: " << (char*) zmq_msg_data(&req) << std::endl;

        //leveldb::Status s;
 
 #if 1
        Context::Scope contextScope(context);
        
        // Compile a function which will keep using the stack until v8 cuts it off
        
        //Local<Script> script = Local<Script>::New(Script::Compile(String::New( (char*) zmq_msg_data(&req) )));
        
        Local<String> source = String::New((char*) zmq_msg_data(&req));
        Local<Script> script = Script::Compile(source);
        if(script.IsEmpty())
        {
            cerr<< "compile error \n";
            
        /*
            Local<Message> message = tryCatch.Message();
            ostringstream strstream;
            strstream<<"Exception encountered at line "<<message->GetLineNumber();
            String::Utf8Value error(tryCatch.Exception());
            strstream<<" "<<*error;
            pimpl->log.error(strstream.str());
            //TODO:
            return false;
         */
        }

        
        TryCatch try_catch;
        //TODO: add compile and check!
        
        // Run the function once and catch the error to generate machine code
        Local<Value> result = script->Run();
        if (try_catch.HasCaught()) {
                String::Utf8Value message(Handle<Object>::Cast(try_catch.Exception())->Get(String::NewSymbol("type")));
                std::cout <<"exception->" <<*message <<"\n";
        }

        //String::AsciiValue ascii(result);
        String::Utf8Value sres(result->ToString());
        //res += result->IntegerValue();
        

#endif
                
        //  Send reply back to client        
        zmq_msg_t reply;
        rc = zmq_msg_init_size (&reply, 200);
        assert (rc == 0);
        snprintf ((char*) zmq_msg_data (&reply), 200, "DONE %s result: %s", (char*) zmq_msg_data(&req), *sres);
        //snprintf ((char*) zmq_msg_data (&reply), 100, "DONE %s", (char*) zmq_msg_data(&req));
        
        /* Send the message to the socket */
        rc = zmq_send (receiver, &reply, 0); 
        assert (rc == 0);
        
        //s_sleep(1);
        
        zmq_msg_close (&req);
        zmq_msg_close (&reply);

    }
    
    context->Exit();    
    //printf("Iter: %d , Result: %ld ; from thread: %ld \n", i, res, tid);
        
    //context.Dispose();
    
    //script.Dispose();
    
    Unlocker unlock(isolate);
    
    //isolate->Exit();
    //isolate->Dispose();

    zmq_close (receiver);
    return NULL;
}

int main() {

    struct dbcontext dbc;
    gcontext = &dbc;
    
    dbc.context = zmq_init (1);

    //  Socket to talk to clients
    void *clients = zmq_socket (dbc.context, ZMQ_ROUTER);
    zmq_bind (clients, "tcp://*:5555");

    //  Socket to talk to workers
    void *workers = zmq_socket (dbc.context, ZMQ_DEALER);
    zmq_bind (workers, "inproc://workers");
   
    // creating DB
    //leveldb::DB* db;
    leveldb::Options options;
    options.create_if_missing = true;
    leveldb::Status s = leveldb::DB::Open(options, "/tmp/testdb", &dbc.db);    
    if (!s.ok()) std::cerr << s.ToString() << std::endl;

    //  Launch pool of worker threads
    int thread_nbr;
    for (thread_nbr = 0; thread_nbr < 2; thread_nbr++) {
        pthread_t worker;
        pthread_create (&worker, NULL, worker_routine, &dbc);
    }
    //  Connect work threads to client threads via a queue
    zmq_device (ZMQ_QUEUE, clients, workers);

    //  We never get here but clean up anyhow
    zmq_close (clients);
    zmq_close (workers);
    zmq_term (dbc.context);
    return 0;
}

