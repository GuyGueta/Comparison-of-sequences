#ifndef EX2_MYPRIORITYQUEUE_H
#define EX2_MYPRIORITYQUEUE_H

#include <vector>
#include <queue>

/*
 * A special class to use priority queue which have a remove method. In this way we can use a min heap and efficiently
 * remove elements from there.
 */
template<class T, class Container = std::vector<T>, class Compare = std::less<typename Container::value_type>>
class MyPriorityQueue : public std::priority_queue<T, Container, Compare>
{
public:
    typedef typename std::priority_queue<T, Container, Compare>::container_type::const_iterator const_iterator;

    bool remove(const T& value)
    {
        auto it = find(this->c.begin(), this->c.end(), value);
        if (it != this->c.end())
        {
            this->c.erase(it);
            std::make_heap(this->c.begin(), this->c.end(), this->comp);
            return true;
        }
        else
        {
            return false;
        }
    }
};
#endif //EX2_MYPRIORITYQUEUE_H
