/*======================================================
    > File Name: buffer.h
    > Author: wwh
    > E-mail:  
    > Other :  
    > Created Time: 2015年12月26日 星期六 12时23分10秒
 =======================================================*/

#ifndef _BUFFER_H_
#define _BUFFER_H_

#include <iostream>
#include <vector>

class Buffer
{
    public:
        Buffer():
            readable(0), writeable(0)
        {
            buffer.resize(4096);
        }
        Buffer(size_t BufferSize):
            readable(0), writeable(0)
        {
            buffer.resize(BufferSize);
        }

        /* return buffer capacity */
        size_t Capacity()
        {
            return buffer.capacity();
        }

        /* return buffer used size */
        size_t UseSize()
        {
            return writeable;
        }

        /* return buffer unused size */
        size_t UnUseSize()
        {
            return buffer.capacity() - writeable;
        }

        /* reduce buffer */
        void ReduceBuffer()
        {
            readable = 0;
            writeable = 0;
        }

    private:
        /* buffer */
        std::vector<char> buffer;
        /* readable position and writeable position*/
        size_t readable;
        size_t writeable;
};



#endif
