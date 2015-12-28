/*======================================================
    > File Name: Logger.h
    > Author: wwh
    > E-mail:  
    > Other :  
    > Created Time: 2015年12月26日 星期六 12时48分41秒
 =======================================================*/

#ifndef _LOGGER_H_
#define _LOGGER_H_

#include <list>
#include <memory>
#include <mutex>
#include <thread>

class Logger
{
    public:


    private:
        std::shared_ptr<Buffer> currentBuffer;
        std::list<std::shared_ptr<Buffer>> usedBuffer;
        std::list<std::shared_ptr<Buffer>> unusedBuffer;

        int filefd;
};


#endif
