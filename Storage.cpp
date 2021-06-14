#include <iostream>
#include <chrono>
#include <mutex>

#include "Storage.h"

Storage::Storage(unsigned int size)
    : size_(size),
    used_size_(10)
{

}

bool Storage::Malloc(unsigned int size)
{
    if (size < FreeSize()) {
        used_size_ = used_size_ + size;
        return true;
    } else {
        return false;
    }
}

void Storage::Free(unsigned int size)
{
    if (size < used_size_) {
        used_size_ = used_size_ - size;
    } else {
        used_size_ = 10;
    }
}
