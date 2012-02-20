//The scoped lock class

#include <pthread.h>

class scoped_lock
{

   public:

   //Constructs and locks
   explicit scoped_lock(pthread_mutex_t* m)
      : mutex(m), locked(false) 
   {  
        pthread_mutex_lock(mutex);   
        locked = true;  
   }

   //Destroys and unlocks
   ~scoped_lock()
   {
      if(locked && mutex)   
        pthread_mutex_unlock(mutex);
   }

   private:
   //Non-copyable
   scoped_lock(scoped_lock const&);
   //Non-assignable
   scoped_lock& operator=  (scoped_lock const&);

   pthread_mutex_t *mutex; 
   bool  locked;
};

