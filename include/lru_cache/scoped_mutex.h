//The scoped lock class

#include <pthread.h>

class scoped_lock
{
   private:
   //Non-copyable
   scoped_lock(scoped_lock const&);
   //Non-assignable
   scoped_lock& operator=  (scoped_lock const&);

   public:

   //Constructs the scoped_lock and locks the mutex using lock()
   explicit scoped_lock(pthread_mutex_t* m)
      : mp_mutex(m), m_locked(false) 
   {  
        pthread_mutex_lock(mp_mutex);   
        m_locked = true;  
   }

   //Destroys the scoped_lock. If the mutex was not released and
   //the mutex is locked, unlocks it using unlock() function
   ~scoped_lock()
   {
      try{  if(m_locked && mp_mutex)   pthread_mutex_unlock(mp_mutex);  }
      catch(...){}
   }

   //Return true if the mutex is locked
   bool locked() const
   {  return m_locked;  }

   private:
   pthread_mutex_t *mp_mutex; 
   bool        m_locked;
};

