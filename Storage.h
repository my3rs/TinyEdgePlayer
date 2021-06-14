#ifndef EDGEPLAYER_STORAGE_H
#define EDGEPLAYER_STORAGE_H

#include <thread>
#include <atomic>
#include <condition_variable>

#include "config.h"

class Storage
{
public:
    explicit Storage(unsigned size = 512);

    void Free(unsigned size);
    bool Malloc(unsigned size);

    void        SetSize(unsigned size) { size_ = size; }
    unsigned    GetSize() const       { return size_; }

    unsigned    FreeSize() const     { return size_ - used_size_; }

    double      load() const
    {
        double load = used_size_ * 1.0 / size_;
        if (load <= 0.0)
            return 0.01;
        else
            return load;
    }

    void        Shutdown();

private:
    void _GcFunc();

private:
    unsigned        size_;          // MB
    std::atomic<unsigned>        used_size_;
};


#endif //EDGEPLAYER_STORAGE_H
