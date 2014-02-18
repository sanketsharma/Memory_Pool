#include <list>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <iostream>

namespace sanket
{
    class TestType
    {
        public: 
            TestType():size(5), array(new char[size])
        {}

            ~TestType()
            {
                delete [] array;
            }

        private:
            unsigned size;
            char* array;
    };

    template <typename T>
        class Memory_Pool
        {
            private:
                std::list<boost::shared_ptr<T> > ptr_list;
                boost::mutex mut;
                boost::condition_variable cond;
                void run();
                unsigned max_res;
                boost::thread th;

            public:
                Memory_Pool();
                ~Memory_Pool();
                boost::shared_ptr<T> get_from_pool();
                void return_to_pool(boost::shared_ptr<T> );
        };

    template <typename T>
        boost::shared_ptr<T> Memory_Pool<T>::get_from_pool()
        {
            boost::lock_guard<boost::mutex> lock(mut);

            if(!ptr_list.empty())
            {
                boost::shared_ptr<T> ptr = ptr_list.front();
                ptr_list.pop_front();
                cond.notify_one();
                return ptr;
            }
            else
            {
                T* new_ptr = new T();
                boost::shared_ptr<T> new_sp = boost::shared_ptr<T>(new_ptr);
                return new_sp;
            }

        }

    template <typename T>
        void Memory_Pool<T>::return_to_pool(boost::shared_ptr<T> sp)
        {
            boost::lock_guard<boost::mutex> lock(mut);
            ptr_list.push_back(sp);
        }

    template <typename T>
        Memory_Pool<T>::Memory_Pool()
        :max_res(10), 
        th(boost::bind(&Memory_Pool<T>::run, this))
    {
    }

    template <typename T>
        Memory_Pool<T>::~Memory_Pool()
        {
            th.interrupt();
            th.join();
        }

    template <typename T>
        void Memory_Pool<T>::run() 
        {
            try
            {
                while (true)
                {

                    {
                        boost::unique_lock<boost::mutex> lock;
                        while(ptr_list.size() >= max_res)
                        {
                            cond.wait(lock);
                        }     
                        T* new_ptr = new T();
                        boost::shared_ptr<T> new_sp = boost::shared_ptr<T>(new_ptr);
                        ptr_list.push_back(new_sp);
                    }
                }
            }
            catch(boost::thread_interrupted & e)
            {
                std::cout << "Encountered Thread Interrupt Exception "<< std::endl;
                return;
            }
        }
}
