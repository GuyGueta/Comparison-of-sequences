#ifndef EX2_THREAD_H
#define EX2_THREAD_H

#include <iostream>
#include <setjmp.h>
#include <memory>
#include <unordered_map>
#include <algorithm>
#include <signal.h>

#ifdef __x86_64__
/* code for 64 bit Intel arch */

typedef unsigned long address_t;
#define JB_SP 6
#define JB_PC 7

/* A translation is required when using an address of a variable.
   Use this as a black box in your code. */
address_t translate_address(address_t addr)
{
    address_t ret;
    asm volatile("xor    %%fs:0x30,%0\n"
                 "rol    $0x11,%0\n"
    : "=g" (ret)
    : "0" (addr));
    return ret;
}

#else
/* code for 32 bit Intel arch */

typedef unsigned int address_t;
#define JB_SP 4
#define JB_PC 5

/* A translation is required when using an address of a variable.
   Use this as a black box in your code. */
address_t translate_address(address_t addr)
{
    address_t ret;
    asm volatile("xor    %%gs:0x18,%0\n"
		"rol    $0x9,%0\n"
                 : "=g" (ret)
                 : "0" (addr));
    return ret;
}
#endif

#define FAIL -1
#define MAIN_THREAD_ID 0
#define SUCCESS 0
#define STACK_SIZE 4096


enum State {RUNNING, READY, BLOCKED};
enum Terminate_Reason {TIME, BLOCK, TERMINATE, SLEEP};
using std::shared_ptr;
using std::make_shared;
using std::cerr;
using std::endl;


class Thread
{
private:
    int _tid;
    int _quantumCounter = 0;
    State _tState = READY;
    bool _sleepStatus = false;
    sigjmp_buf _env;
    long _wakeTime;
    char* _stack = new char[STACK_SIZE];

public:
    Thread(int tid, void (*f)() = nullptr)
    {
        _tid = tid;
        if (!tid)
        {
            _tState = RUNNING;
            _stack = nullptr;
        }
        address_t pc, sp;
        sp = (address_t)_stack + STACK_SIZE - sizeof(address_t);
        pc = (address_t)f;
        sigsetjmp(_env, 1);

        (_env->__jmpbuf)[JB_SP] = translate_address(sp);
        (_env->__jmpbuf)[JB_PC] = translate_address(pc);
        sigemptyset(&_env->__saved_mask);
        if (sigemptyset(&_env->__saved_mask) == FAIL)
        {
            cerr << "system error: sig emptyset failed" << std::endl;
            exit(1);
        }
    }


    /*
     * Returns the thread id.
     */
    int get_tid() const { return _tid; }


    /*
     * Returns the thread number of quantums.
     */
    int get_quantumCounter() const { return _quantumCounter; }


    /*
     * Returns the thread state.
     */
    State get_tState() const { return _tState; }


    /*
     * Returns true if the thread is sleeping, false otherwise.
     */
    bool get_sleepStatus() const { return _sleepStatus; }


    /*
     * Sets the thread quantum number to quantum.
     */
    void set_quantumCounter(int quantum) { _quantumCounter = quantum; }


    /*
     * Sets the thread state to tState.
     */
    void set_tState(State tState) { _tState = tState; }


    /*
     * Sets the thread sleep status to sleepStatus.
     */
    void set_sleepStatus(bool sleepStatus) { _sleepStatus = sleepStatus; }


    /*
     * Returns the thread Env reference.
     */
    sigjmp_buf &getEnv() { return _env; }


    /*
     * Returns the thread wake time - when the thread need to wake up.
     */
    long get_wakeTime() const { return _wakeTime; }


    /*
     * Sets the thread wake time to time.
     */
    void set_wakeTime(long time) { _wakeTime = time; }


    /*
     * A comparator for compare between 2 threads by their wake time. For use in the priority queue.
     */
    struct sleepComparator
    {
        bool operator()(const shared_ptr<Thread> &thread1, const shared_ptr<Thread> &thread2) const
        {
            return thread1->get_wakeTime() > thread2->get_wakeTime();
        }
    };
};



#endif //EX2_THREAD_H

