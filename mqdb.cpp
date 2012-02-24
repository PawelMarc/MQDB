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
#include "lru_cache/lru_cache.h"


#define NUM_THREADS     5

#define JSVAL Handle<Value>
#define JSOBJ Handle<Object>
#define JSOBJT Handle<ObjectTemplate>
#define JSARRAY Handle<Array>
#define JSARGS const Arguments&

using namespace std;
using namespace v8;

/// LRUCache for keeping open databases
typedef LRUCache<std::string, leveldb::DB*> db_lru_type;

struct dbcontext {
    void *context;
    db_lru_type* dbl;
};

//global context
struct dbcontext *gcontext;


//helper db functions

//opens or creates db for given name
leveldb::DB* getDb(db_lru_type* dbl, const String::Utf8Value &name, leveldb::Status& s) {
    leveldb::DB* ret = 0;
    //first check global lru cache
    if (dbl->exists(*name)) {
        //cout << "getDB exists for: " << *name << endl;
        ret = dbl->fetch(*name); //->db;
    }
    else {
        //cout << "getDB create for: " << *name << endl;
        leveldb::Options options;
        options.create_if_missing = true;
        s = leveldb::DB::Open(options, *name, &ret);    
        if (s.ok()) {
            dbl->insert(*name, ret);
        } else {
            //TODO: error handling!
            std::cerr << s.ToString() << std::endl;
            //v8::ThrowException(v8::String::New(s.ToString().c_str()));
            //return NULL;
            //std::cerr << s.ToString() << std::endl;
        }
    }
    return ret;
}

void closeDb(leveldb::DB* db) {
    cout << "closeDb for: " << db << endl;
    delete db;
}

//*********************************
//PER THREAD OBECT MANAGEMENT

pthread_key_t   tlsKey = 0;

//used on exit from thread 
void globalDestructor(void *value)
{
  printf("In the globalDestructor\n");
  //TODO: free vector
  //free(value);
  pthread_setspecific(tlsKey, NULL);
}

//struct when we keep pointer to allocated value and pointer to function to deallocate
struct deobj {
    void* val;
    void (*des)(void*);
};

void th_alloc_array(int size)
{
  vector<deobj> * SS = new vector<deobj>();
  SS->reserve(size);
  
  //printf("alloc_array Inside secondary thread\n");
  pthread_setspecific(tlsKey, (void*)SS);
}

void add_obj(void* val, void (*des)(void*)) {
    struct deobj dob;
 
    void* global  = pthread_getspecific(tlsKey);
    vector<deobj> * SS = static_cast< vector<deobj>* >(global);
      
    dob.val = val;
    dob.des = des;
   
    SS->push_back(dob);
}

void free_objects() {
    struct deobj dob;
 
    void* global  = pthread_getspecific(tlsKey);
    vector<deobj> * SS = static_cast< vector<deobj>* >(global);
    
    while (!SS->empty())
    {
        dob = SS->back();
        SS->pop_back();
        dob.des(dob.val);
    }
}



// Javascript API

static JSVAL addnum(JSARGS args) {
    HandleScope scope;
    int x = args[0]->IntegerValue();
    int y = args[1]->IntegerValue();
    return scope.Close(Integer::New(x+y));
}

static JSVAL put(JSARGS args) {
    HandleScope scope;
    String::Utf8Value dbname(args[0]->ToString());
    String::Utf8Value key(args[1]->ToString());
    String::Utf8Value val(args[1]->ToString());
    leveldb::Status s;
    leveldb::DB* db = getDb(gcontext->dbl, dbname, s);
    if (!db) {
        return v8::ThrowException(v8::String::New(s.ToString().c_str()));
    }    
    s = db->Put(leveldb::WriteOptions(), *key, *val);
    if (!s.ok()) {
        return v8::ThrowException(v8::String::New(s.ToString().c_str()));
    }
    return scope.Close(Integer::New(1));
}

static JSVAL get(JSARGS args) {
    HandleScope scope;
    String::Utf8Value dbname(args[0]->ToString());
    String::Utf8Value key(args[1]->ToString());
    std::string val;
    leveldb::Status s;
    leveldb::DB* db = getDb(gcontext->dbl, dbname,s);
    if (!db) {
        return v8::ThrowException(v8::String::New(s.ToString().c_str()));
    }
    s = db->Get(leveldb::ReadOptions(), *key, &val);
    if (!s.ok()) {
        //if not found we return Null otherwise we throw exception
        if (s.IsNotFound())
            return scope.Close(Null());
        else
            return v8::ThrowException(v8::String::New(s.ToString().c_str()));
    }
    Handle<String> res = String::New(val.c_str());
    return scope.Close(res);
}

static JSVAL del(JSARGS args) {
    HandleScope scope;
    String::Utf8Value dbname(args[0]->ToString());
    String::Utf8Value key(args[1]->ToString());
    leveldb::Status s;
    leveldb::DB* db = getDb(gcontext->dbl, dbname,s);
    if (!db) {
        return v8::ThrowException(v8::String::New(s.ToString().c_str()));
    }
    s = db->Delete(leveldb::WriteOptions(), *key);
    if (!s.ok()) {
        //if not found we return Null otherwise we throw exception
        if (s.IsNotFound())
            return scope.Close(Null());
        else
            return v8::ThrowException(v8::String::New(s.ToString().c_str()));
    }
    return scope.Close(Integer::New(1));
}

// ITERATOR API

void auto_del_it(void *val) {
    //cout << "Auto deleting Iterator" << endl;
    leveldb::Iterator* it = static_cast< leveldb::Iterator* >(val);
    delete it;
}

static JSVAL it_new(JSARGS args) {
    HandleScope scope;
    String::Utf8Value dbname(args[0]->ToString());
    //String::Utf8Value key(args[1]->ToString());
    leveldb::Status s;
    leveldb::DB* db = getDb(gcontext->dbl, dbname,s);
    if (!db) {
        return v8::ThrowException(v8::String::New(s.ToString().c_str()));
    }
        
    leveldb::Iterator* it = db->NewIterator(leveldb::ReadOptions());
    
    //add iterator for auto del
    add_obj((void*)it, auto_del_it);
    
    return scope.Close(External::Wrap(it));
}

static JSVAL it_first(JSARGS args) {
    HandleScope scope;
    leveldb::Iterator* it = (leveldb::Iterator*) External::Unwrap(args[0]);
    if (!it) {
        return v8::ThrowException(v8::String::New("Iterator is Null"));
    }
    it->SeekToFirst();
    return scope.Close(Integer::New(1));
}

static JSVAL it_last(JSARGS args) {
    HandleScope scope;
    leveldb::Iterator* it = (leveldb::Iterator*) External::Unwrap(args[0]);
    if (!it) {
        return v8::ThrowException(v8::String::New("Iterator is Null"));
    }
    it->SeekToLast();
    return scope.Close(Integer::New(1));
}

static JSVAL it_seek(JSARGS args) {
    HandleScope scope;
    leveldb::Iterator* it = (leveldb::Iterator*) External::Unwrap(args[0]);
    String::Utf8Value val(args[1]->ToString());
    if (!it) {
        return v8::ThrowException(v8::String::New("Iterator is Null"));
    }
    it->Seek(*val);
    return scope.Close(Integer::New(1));
}

static JSVAL it_next(JSARGS args) {
    HandleScope scope;
    leveldb::Iterator* it = (leveldb::Iterator*) External::Unwrap(args[0]);
    //leveldb::Iterator* it = (leveldb::Iterator*) args[0]->IntegerValue();
    if (!it) {
        return v8::ThrowException(v8::String::New("Iterator is Null"));
    }
    it->Next();
    return scope.Close(Integer::New(1));
}

static JSVAL it_prev(JSARGS args) {
    HandleScope scope;
    leveldb::Iterator* it = (leveldb::Iterator*) External::Unwrap(args[0]);
    //leveldb::Iterator* it = (leveldb::Iterator*) args[0]->IntegerValue();
    if (!it) {
        return v8::ThrowException(v8::String::New("Iterator is Null"));
    }
    it->Prev();
    return scope.Close(Integer::New(1));
}

static JSVAL it_valid(JSARGS args) {
    HandleScope scope;
    leveldb::Iterator* it = (leveldb::Iterator*) External::Unwrap(args[0]);
    //leveldb::Iterator* it = (leveldb::Iterator*) args[0]->IntegerValue();
    if (!it) {
        return v8::ThrowException(v8::String::New("Iterator is Null"));
    }
    return scope.Close(Integer::New(it->Valid()));
}

static JSVAL it_key(JSARGS args) {
    HandleScope scope;
    leveldb::Iterator* it = (leveldb::Iterator*) External::Unwrap(args[0]);
    //leveldb::Iterator* it = (leveldb::Iterator*) args[0]->IntegerValue();
    if (!it) {
        return v8::ThrowException(v8::String::New("Iterator is Null"));
    }
    Handle<String> res = String::New(it->key().ToString().c_str());
    return scope.Close(res);
}

static JSVAL it_val(JSARGS args) {
    HandleScope scope;
    leveldb::Iterator* it = (leveldb::Iterator*) External::Unwrap(args[0]);
    //leveldb::Iterator* it = (leveldb::Iterator*) args[0]->IntegerValue();
    if (!it) {
        return v8::ThrowException(v8::String::New("Iterator is Null"));
    }
    Handle<String> res = String::New(it->value().ToString().c_str());
    return scope.Close(res);
}

static JSVAL it_del(JSARGS args) {
    HandleScope scope;
    leveldb::Iterator* it = (leveldb::Iterator*) External::Unwrap(args[0]);
    //leveldb::Iterator* it = (leveldb::Iterator*) args[0]->IntegerValue();
    if (!it) {
        return v8::ThrowException(v8::String::New("Iterator is Null"));
    }
    delete it;
    return scope.Close(Integer::New(1));
}

//Taken from google shell example
// Extracts a C string from a V8 Utf8Value.
const char* ToCString(const v8::String::Utf8Value& value) {
  return *value ? *value : "<string conversion failed>";
}


void ReportException(v8::TryCatch* try_catch) {
  v8::HandleScope handle_scope;
  v8::String::Utf8Value exception(try_catch->Exception());
  const char* exception_string = ToCString(exception);
  v8::Handle<v8::Message> message = try_catch->Message();
  if (message.IsEmpty()) {
    // V8 didn't provide any extra information about this error; just
    // print the exception.
    printf("%s\n", exception_string);
  } else {
    // Print (filename):(line number): (message).
    v8::String::Utf8Value filename(message->GetScriptResourceName());
    const char* filename_string = ToCString(filename);
    int linenum = message->GetLineNumber();
    printf("%s:%i: %s\n", filename_string, linenum, exception_string);
    // Print line of source code.
    v8::String::Utf8Value sourceline(message->GetSourceLine());
    const char* sourceline_string = ToCString(sourceline);
    printf("%s\n", sourceline_string);
    // Print wavy underline (GetUnderline is deprecated).
    int start = message->GetStartColumn();
    for (int i = 0; i < start; i++) {
      printf(" ");
    }
    int end = message->GetEndColumn();
    for (int i = start; i < end; i++) {
      printf("^");
    }
    printf("\n");
    v8::String::Utf8Value stack_trace(try_catch->StackTrace());
    if (stack_trace.length() > 0) {
      const char* stack_trace_string = ToCString(stack_trace);
      printf("%s\n", stack_trace_string);
    }
  }
}
        
void* worker_routine(void *vdbc) {

    struct dbcontext * dbc = (struct dbcontext *) vdbc;
    
    //allocate array for objects that will be automatically removed after 
    th_alloc_array(1000);
    
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
    globals->Set(String::New("del"), FunctionTemplate::New(del));
    
    //iterator
    globals->Set(String::New("it_del"), FunctionTemplate::New(it_del));
    globals->Set(String::New("it_new"), FunctionTemplate::New(it_new));
    globals->Set(String::New("it_first"), FunctionTemplate::New(it_first));
    globals->Set(String::New("it_last"), FunctionTemplate::New(it_last));
    globals->Set(String::New("it_seek"), FunctionTemplate::New(it_seek));
    globals->Set(String::New("it_next"), FunctionTemplate::New(it_next));
    globals->Set(String::New("it_prev"), FunctionTemplate::New(it_prev));
    globals->Set(String::New("it_valid"), FunctionTemplate::New(it_valid));
    globals->Set(String::New("it_key"), FunctionTemplate::New(it_key));
    globals->Set(String::New("it_val"), FunctionTemplate::New(it_val));

    Handle<Context> context = Context::New(NULL, globals);
    context->Enter();
    
    while (1) {
        
        zmq_msg_t req;
        int rc = zmq_msg_init (&req);
        assert (rc == 0);
        rc = zmq_recv (receiver, &req, 0);
        assert (rc == 0);
        
        zmq_msg_t reply;
        rc = zmq_msg_init_size (&reply, 200);
        assert (rc == 0);

        
        //std::cout << "thread: " << pthread_self() << " Received script: " << (char*) zmq_msg_data(&req) << std::endl;

        //leveldb::Status s;
 
 #if 1
        Context::Scope contextScope(context);
        
        // Compile a function which will keep using the stack until v8 cuts it off
        
        //Local<Script> script = Local<Script>::New(Script::Compile(String::New( (char*) zmq_msg_data(&req) )));
        
        TryCatch try_catch;
        
        Local<String> source = String::New((char*) zmq_msg_data(&req));
        Local<Script> script = Script::Compile(source);
        
        if(script.IsEmpty())
        {
            ReportException(&try_catch);
            
            String::Utf8Value error(try_catch.Exception());
            v8::Handle<v8::Message> message = try_catch.Message();
            int line_num = 0;
            int col_num = 0;
            
            if (message.IsEmpty()) {
                
            }else {
                line_num = message->GetLineNumber();
                col_num = message->GetStartColumn();
            }
            
            int mess_size = error.length()+100;
            rc = zmq_msg_init_size (&reply, mess_size);
            assert (rc == 0);

            //cerr << "compile error line: " << " message: " << *error << endl;
            snprintf ((char*) zmq_msg_data (&reply), mess_size, "2; {res: 'COMPILE ERROR', line: %d, col: %d, message: '%s'}", line_num, col_num, *error);
        }
        else {        

            // Run the function once and catch the error to generate machine code
            Local<Value> result = script->Run();
                    
            if (try_catch.HasCaught()) {
                //TODO: escape string
                String::Utf8Value message(try_catch.Exception());
                int mess_size = message.length()+100;
                rc = zmq_msg_init_size (&reply, mess_size);
                assert (rc == 0);

                std::cout <<"exception->" <<*message <<"\n";
                snprintf ((char*) zmq_msg_data (&reply), mess_size, "1; {res: 'ERROR', message: '%s'}", *message);
            }
            else {
                //TODO: escape string
                String::Utf8Value message(result->ToString());
                int mess_size = message.length()+2;
                rc = zmq_msg_init_size (&reply, mess_size);
                assert (rc == 0);

                snprintf ((char*) zmq_msg_data (&reply), mess_size, " %s", *message);
            }
        }        

#endif
                
        
        /* Send the message to the socket */
        rc = zmq_send (receiver, &reply, 0); 
        assert (rc == 0);
        
        //s_sleep(1);
        
        zmq_msg_close (&req);
        zmq_msg_close (&reply);
        
        free_objects();
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

    //register key for vector when we keep per thread allocated objects to free them automatically
    int rc = pthread_key_create(&tlsKey, globalDestructor);
    assert(rc==0);
    
    struct dbcontext dbc;
    gcontext = &dbc;
    
    dbc.context = zmq_init (1);
    //max open 100 databases
    dbc.dbl = new db_lru_type(100);
    dbc.dbl->setOnRemove(&closeDb);

    //  Socket to talk to clients
    void *clients = zmq_socket (dbc.context, ZMQ_ROUTER);
    zmq_bind (clients, "tcp://*:5555");

    //  Socket to talk to workers
    void *workers = zmq_socket (dbc.context, ZMQ_DEALER);
    zmq_bind (workers, "inproc://workers");
   
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

