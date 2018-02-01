#ifndef CONCURRENTQUEUE_H
#define CONCURRENTQUEUE_H

#include <queue>
#include <mutex>
#include <condition_variable>
#include <utility>

template<typename T>
class ConcurrentQueue
{
public:
    bool Push(T val); // blocking call; returns false if queue shutdown
    bool Pop(T & val); // blocking; returns false if queue is shutdown
    void Shutdown(); // push fails; pop fails once queue empty
private:
    std::queue<T> stdQueue;
    mutable std::mutex queueMtx;
    mutable std::condition_variable queueCV;
    bool isShutDown = false; // This can't be atomic! See notes regarding cond vars
};

template <typename T>
void ConcurrentQueue<T>::Shutdown()
{
    std::lock_guard<std::mutex> lk(queueMtx);
    isShutDown = true;
    queueCV.notify_all();
}

template <typename T>
bool ConcurrentQueue<T>::Push(T val)
{
    std::lock_guard<std::mutex> lk(queueMtx);

    if (isShutDown)
        return false;

    stdQueue.push(std::move(val));
    queueCV.notify_one();

    return true;
}

template <typename T>
bool ConcurrentQueue<T>::Pop(T & val)
{
    std::unique_lock<std::mutex> lk(queueMtx);
    queueCV.wait(lk, [this] () { return (!stdQueue.empty() || isShutDown); });

    if (!stdQueue.empty())
    {
        val = std::move(stdQueue.front());
        stdQueue.pop();
        return true;
    }

    return false; // queue is empty and condVar was notified; so shutdown
}

#endif // CONCURRENTQUEUE_H
