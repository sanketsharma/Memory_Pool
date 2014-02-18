#include <list>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <iostream>

class A
{
    public: 
        A():size(5), array(new char[size])
        {}

        ~A()
        {
            delete [] array;
        }

    private:
        unsigned size;
        char* array;
};

class Memory_Pool
{
    private:
    std::list<boost::shared_ptr<A> > ptr_list;
    boost::mutex mut;
    boost::condition_variable cond;
    void run();
    unsigned max_res;
    boost::thread th;

    public:
    Memory_Pool();
    ~Memory_Pool();
    boost::shared_ptr<A> get_from_pool();
    void return_to_pool(boost::shared_ptr<A> );
};

boost::shared_ptr<A> Memory_Pool::get_from_pool()
{
    boost::lock_guard<boost::mutex> lock(mut);

    if(!ptr_list.empty())
    {
        boost::shared_ptr<A> ptr = ptr_list.front();
        ptr_list.pop_front();
        cond.notify_one();
        return ptr;
    }
    else
    {
        A* new_ptr = new A();
        boost::shared_ptr<A> new_sp = boost::shared_ptr<A>(new_ptr);
        return new_sp;
    }

}

void Memory_Pool::return_to_pool(boost::shared_ptr<A> sp)
{
    boost::lock_guard<boost::mutex> lock(mut);
    ptr_list.push_back(sp);
}

Memory_Pool::Memory_Pool():max_res(10), th(boost::bind(&Memory_Pool::run, this))
{
}

Memory_Pool::~Memory_Pool()
{
    th.interrupt();
    th.join();
}

void Memory_Pool::run() 
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
                A* new_ptr = new A();
                boost::shared_ptr<A> new_sp = boost::shared_ptr<A>(new_ptr);
                ptr_list.push_back(new_sp);
            }
        }
    }
    catch(boost::thread_interrupted & e)
    {
        std::cout << "Encountered Thread Interrupt Exception "<< std::endl;
        //std::cout << "Encountered Exception " << e  << std::endl;
        return;
    }
}
