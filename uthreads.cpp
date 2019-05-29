#include <iostream>
#include <deque>
#include <queue>
#include <signal.h>
#include <sys/time.h>
#include "uthreads.h"
#include "Thread.h"
#include "MyPriorityQueue.h"

typedef std::shared_ptr<Thread> threadPtr;
static int allThreadQuantum = 0;
static int threadCounter = 0;
threadPtr runningThread;
threadPtr mainThread;
MyPriorityQueue<threadPtr, std::vector<threadPtr>, Thread::sleepComparator> sleepingThreads;
std::unordered_map<int, threadPtr> blockedThreads;
std::deque<threadPtr> readyThreads;
std::priority_queue <int, std::vector<int>, std::greater<int>> availableTids;
threadPtr allThreadsArr[MAX_THREAD_NUM];
sigset_t signalSet;
struct sigaction sa;
struct itimerval timer;
struct timeval start;


/*
 * Switches between threads - runs the next thread in the ready threads queue.
 * @param terminateReason - The reason the threads need to be switched.
 */
void switchThreads(Terminate_Reason terminateReason);


/*
 * Blocks the SIGVTALRM signal
 */
void blockSignals()
{
    if ((sigemptyset(&signalSet) == FAIL))
    {
        cerr << "System Error: Signal action fail" << std::endl;
        exit(1);
    }
    if (sigaddset(&signalSet, SIGVTALRM) == FAIL)
    {
        cerr << "System Error: Signal action fail" << std::endl;
        exit(1);
    }
    if (sigprocmask(SIG_SETMASK, &signalSet, NULL) == FAIL)
    {
        cerr << "System Error: Signal action fail" << std::endl;
        exit(1);
    }
}


/*
 *  Unblocks the SIGVTALRM signal
 */
void unblockSignals()
{
    if ((sigemptyset(&signalSet) == FAIL))
    {
        cerr << "System Error: Signal action fail" << std::endl;
        exit(1);
    }
    if (sigaddset(&signalSet, SIGVTALRM) == FAIL)
    {
        cerr << "System Error: Signal action fail" << std::endl;
        exit(1);
    }
    if (sigprocmask(SIG_UNBLOCK, &signalSet, NULL) == FAIL)
    {
        cerr << "System Error: Signal action fail" << std::endl;
        exit(1);
    }
}

/*
 * Sets the virtual timer to start
 */
void setTimer()
{

    if (setitimer(ITIMER_VIRTUAL, &timer, NULL)) {
        cerr << "System Error: Timer error" << std::endl;
        unblockSignals();
        exit(1);
    }
}


/*
 * handles the timer signals
 */
void timer_handler(int sig)
{
    blockSignals();
    switchThreads(TIME);
    unblockSignals();
}


/*
 * Initializes the virtual timer with the given number of quantom micros seconds
 */
void initializeTimer(int quantom_usecs)
{
    sa.sa_handler = &timer_handler;

    if (sigaction(SIGVTALRM, &sa,NULL) < 0) {
        cerr << "System Error: Sigaction error." << std::endl;
        unblockSignals();
        exit(1);
    }
    timer.it_value.tv_sec = quantom_usecs / 1000000;		// first time interval, seconds part
    timer.it_value.tv_usec = quantom_usecs % 1000000;	// first time
    // interval,  microseconds part
    timer.it_interval.tv_sec = quantom_usecs / 1000000;	// following time intervals,
    // seconds part
    timer.it_interval.tv_usec = quantom_usecs % 1000000;	// following time
    // intervals, microseconds part
    setTimer();
}


/*
 * Description: This function initializes the thread library.
 * You may assume that this function is called before any other thread library
 * function, and that it is called exactly once. The input to the function is
 * the length of a quantum in micro-seconds. It is an error to call this
 * function with non-positive quantum_usecs.
 * Return value: On success, return 0. On failure, return -1.
*/
int uthread_init(int quantum_usecs)
{
    blockSignals();
    if (quantum_usecs <= 0)
    {
        std::cerr << "Thread library error: non-positive number" <<endl;
        unblockSignals();
        return FAIL;
    }
    ++allThreadQuantum;
    ++threadCounter;
    mainThread = make_shared<Thread>(0);
    mainThread->set_quantumCounter(1); // initializing the main with 1 quantum
    allThreadsArr[0]= mainThread;
    runningThread = mainThread;
    initializeTimer(quantum_usecs);
    unblockSignals();
    return SUCCESS;

}


/*
 * Description: This function creates a new thread, whose entry point is the
 * function f with the signature void f(void). The thread is added to the end
 * of the READY threads list. The uthread_spawn function should fail if it
 * would cause the number of concurrent threads to exceed the limit
 * (MAX_THREAD_NUM). Each thread should be allocated with a stack of size
 * STACK_SIZE bytes.
 * Return value: On success, return the ID of the created thread.
 * On failure, return -1.
*/
int uthread_spawn(void (*f)(void))
{
    blockSignals();
    if (threadCounter == MAX_THREAD_NUM)
    {
        cerr << "Thread library error: non-positive number" << endl;
        unblockSignals();
        return FAIL;
    }
    int currentTid = threadCounter;
    ++threadCounter;
    if (!availableTids.empty())
    {
        currentTid = availableTids.top();
        availableTids.pop();
    }
    threadPtr newThread = make_shared<Thread>(currentTid, f);
    readyThreads.push_back(newThread);
    allThreadsArr[currentTid] = newThread;
    unblockSignals();
    return currentTid;
}


/*
 * Checks if there are threads which need to be waken up. If yes - wakes them up.
 */
void check_for_wake_threads()
{
    while (!sleepingThreads.empty())
    {
        threadPtr threadToCheck = sleepingThreads.top();
        if (gettimeofday(&start, nullptr))
        {
            cerr << "System Error: There is a problem to get the real time" << endl;
            unblockSignals();
            exit(1);
        }
        long timeNow = start.tv_sec * 1000000 + start.tv_usec;
        if (timeNow >= threadToCheck->get_wakeTime())
        {
            threadToCheck->set_sleepStatus(false);
            if (threadToCheck->get_tState() == BLOCKED)
            {
                blockedThreads[threadToCheck->get_tid()] = threadToCheck;
            }
            else {
                readyThreads.push_back(threadToCheck);
            }
            sleepingThreads.pop();
        }
        else
        {
            return;
        }
    }

}


/*
 * Switches between threads - runs the next thread in the ready threads queue.
 * @param terminateReason - The reason the threads need to be switched.
 */
void switchThreads(Terminate_Reason terminateReason)
{
    ++allThreadQuantum;
    check_for_wake_threads();
    if (readyThreads.empty())
    {
        runningThread->set_quantumCounter(runningThread->get_quantumCounter() + 1);
        return;
    }
    int returnVal = 0;
    switch (terminateReason)
    {
        case TIME:
        {
            readyThreads.push_back(runningThread);
            runningThread->set_tState(READY);
            returnVal = sigsetjmp(runningThread->getEnv(), 1);
            break;
        }
        case BLOCK:
        {
            blockedThreads[runningThread->get_tid()] = runningThread;
            runningThread->set_tState(BLOCKED);
            returnVal = sigsetjmp(runningThread->getEnv(), 1);
            break;
        }
        case TERMINATE:
        {
            allThreadsArr[runningThread->get_tid()] = nullptr;
            break;
        }
        case SLEEP:
        {
            runningThread->set_tState(READY);
            returnVal = sigsetjmp(runningThread->getEnv(), 1);
            break;
        }
    }
    if (returnVal)
    {
        setTimer();
        return;
    }
    runningThread = readyThreads.front();
    readyThreads.pop_front();
    runningThread->set_tState(RUNNING);
    runningThread->set_quantumCounter(runningThread->get_quantumCounter() + 1);
    siglongjmp(runningThread->getEnv(), 1);
}


/*
 * Terminate the main thread and thus terminate the all other threads.
 */
void terminateMainThread()
{
    runningThread = nullptr;
    mainThread = nullptr;
    for (auto thread : allThreadsArr) thread = nullptr;
    while(!readyThreads.empty()) readyThreads.pop_front();
    blockedThreads.clear();
    while (!sleepingThreads.empty()) sleepingThreads.pop();
}


/*
 * Description: This function terminates the thread with ID tid and deletes
 * it from all relevant control structures. All the resources allocated by
 * the library for this thread should be released. If no thread with ID tid
 * exists it is considered an error. Terminating the main thread
 * (tid == 0) will result in the termination of the entire process using
 * exit(0) [after releasing the assigned library memory].
 * Return value: The function returns 0 if the thread was successfully
 * terminated and -1 otherwise. If a thread terminates itself or the main
 * thread is terminated, the function does not return.
*/
int uthread_terminate(int tid)
{
    bool wasSleeping = false;
    blockSignals();
    if ((!(0<= tid && tid <= MAX_THREAD_NUM) || allThreadsArr[tid] == nullptr))
    {
        cerr << "Thread Library Error: There is no thread with such id" << endl;
        unblockSignals();
        return FAIL;
    }
    if (tid == MAIN_THREAD_ID)
    {
        terminateMainThread();
        unblockSignals();
        exit(SUCCESS);
    }
    threadPtr threadToTerminate = allThreadsArr[tid];
    State threadState = threadToTerminate->get_tState();
    if (threadToTerminate->get_sleepStatus())
    {
        wasSleeping = true;
        sleepingThreads.remove(threadToTerminate);
        threadToTerminate = nullptr;
    }
    switch (threadState)
    {
        case RUNNING:
        {
            switchThreads(TERMINATE);
            break;
        }
        case BLOCKED:
        {
            blockedThreads.erase(tid);
            allThreadsArr[tid] = nullptr;
            break;
        }
        case READY:
        {
            if (!wasSleeping)
            {
                auto pos = std::find(readyThreads.cbegin(), readyThreads.cend(), threadToTerminate);
                readyThreads.erase(pos);
                allThreadsArr[tid] = nullptr;
            }
            break;
        }
    }
    availableTids.push(tid);
    --threadCounter;
    unblockSignals();
    return SUCCESS;
}


/*
 * Description: This function blocks the thread with ID tid. The thread may
 * be resumed later using uthread_resume. If no thread with ID tid exists it
 * is considered as an error. In addition, it is an error to try blocking the
 * main thread (tid == 0). If a thread blocks itself, a scheduling decision
 * should be made. Blocking a thread in BLOCKED state has no
 * effect and is not considered an error.
 * Return value: On success, return 0. On failure, return -1.
*/
int uthread_block(int tid)
{
    blockSignals();
    if ((!(0<= tid && tid <= MAX_THREAD_NUM) || allThreadsArr[tid] == nullptr))
    {
        cerr << "Thread Library Error: There is no thread with such id" << endl;
        unblockSignals();
        return FAIL;
    }
    if (!tid)
    {
        cerr << "Thread Library Error: Trying to block the main thread" << endl;
        unblockSignals();
        return FAIL;
    }
    threadPtr threadToBlock = allThreadsArr[tid];
    switch(threadToBlock->get_tState())
    {
        case RUNNING:
        {
            switchThreads(BLOCK);
            break;
        }
        case READY:
        {
            if (threadToBlock->get_sleepStatus())
            {
                threadToBlock->set_tState(BLOCKED);
            }
            else
            {
                auto pos = std::find(readyThreads.cbegin(), readyThreads.cend(), threadToBlock);
                (*pos)->set_tState(BLOCKED);
                blockedThreads[tid] = *pos;
                readyThreads.erase(pos);
            }
            break;
        }
        case BLOCKED:
            break;
    }
    unblockSignals();
    return SUCCESS;
}


/*
 * Description: This function resumes a blocked thread with ID tid and moves
 * it to the READY state. Resuming a thread in a RUNNING or READY state
 * has no effect and is not considered as an error. If no thread with
 * ID tid exists it is considered an error.
 * Return value: On success, return 0. On failure, return -1.
*/
int uthread_resume(int tid)
{
    blockSignals();
    if ((!(0<= tid && tid <= MAX_THREAD_NUM) || allThreadsArr[tid] == nullptr)) //Check what shared pointer default
    {
        cerr << "Thread Library Error: There is no thread with such id" << endl;
        unblockSignals();
        return FAIL;
    }
    threadPtr threadToResume = allThreadsArr[tid];
    if (threadToResume->get_tState() == BLOCKED)
    {
        threadToResume->set_tState(READY);
        if (!threadToResume->get_sleepStatus())
        {
            readyThreads.push_back(threadToResume);
            blockedThreads.erase(tid);
        }
    }
    unblockSignals();
    return SUCCESS;
}


/*
 * Description: This function blocks the RUNNING thread for usecs micro-seconds in real time (not virtual
 * time on the cpu). It is considered an error if the main thread (tid==0) calls this function. Immediately after
 * the RUNNING thread transitions to the BLOCKED state a scheduling decision should be made.
 * After the sleeping time is over, the thread should go back to the end of the READY threads list.
 * Return value: On success, return 0. On failure, return -1.
*/
int uthread_sleep(unsigned int usec)
{
    blockSignals();
    if (!runningThread->get_tid())
    {
        cerr << "Thread Library Error: The main thread can't sleep" << endl;
        unblockSignals();
        return FAIL;
    }
    if (gettimeofday(&start, nullptr))
    {
        cerr << "System Error: There is a problem to get the real time" << endl;
        unblockSignals();
        exit(1);
    }
    runningThread->set_sleepStatus(true);
    runningThread->set_wakeTime(start.tv_sec * 1000000 + start.tv_usec + usec);
    sleepingThreads.push(runningThread);
    switchThreads(SLEEP);
    unblockSignals();
    return SUCCESS;
}


/*
 * Description: This function returns the thread ID of the calling thread.
 * Return value: The ID of the calling thread.
*/
int uthread_get_tid()
{
    return runningThread->get_tid();
}


/*
 * Description: This function returns the total number of quantums since
 * the library was initialized, including the current quantum.
 * Right after the call to uthread_init, the value should be 1.
 * Each time a new quantum starts, regardless of the reason, this number
 * should be increased by 1.
 * Return value: The total number of quantums.
*/
int uthread_get_total_quantums()
{
    return allThreadQuantum;
}


/*
 * Description: This function returns the number of quantums the thread with
 * ID tid was in RUNNING state. On the first time a thread runs, the function
 * should return 1. Every additional quantum that the thread starts should
 * increase this value by 1 (so if the thread with ID tid is in RUNNING state
 * when this function is called, include also the current quantum). If no
 * thread with ID tid exists it is considered an error.
 * Return value: On success, return the number of quantums of the thread with ID tid.
 * 			     On failure, return -1.
*/
int uthread_get_quantums(int tid)
{
    blockSignals();
    if ((!(0<= tid && tid <= MAX_THREAD_NUM) || allThreadsArr[tid] == nullptr))
    {
        cerr << "Thread Library Error: There is no thread with such id" << endl;
        unblockSignals();
        return FAIL;
    }
    unblockSignals();
    return allThreadsArr[tid]->get_quantumCounter();
}


