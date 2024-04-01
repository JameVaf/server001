#ifndef __CELLTIMESTAMP_HPP__
#define __CELLTIMESTAMP_HPP__

#include<iostream>
#include<chrono>

class CELLTimestamp
{
public:
    CELLTimestamp()
    {
        updata();
    }
    ~CELLTimestamp()
    {

    }

    //更新时间
    void updata()
    {
        begin_ = std::chrono::high_resolution_clock::now ();
    }

    //获取当前微秒
    long long getElapsedSecond()
    {
        return getElapsedTimeInMicroSec() * 0.000001;
    }

    //获取毫秒
    long long getElapsedTimeInMilliSec()
    {
        return getElapsedTimeInMilliSec() * 0.001;
    }

    //获取微秒
    long long getElapsedTimeInMicroSec()
    {
        return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - begin_).count();
    }

private:
    std::chrono::time_point<std::chrono::high_resolution_clock> begin_;
};

#endif //__CELLTIMESTAMP_HPP__