#ifndef SERVER_IOMAMAGER_H
#define SERVER_IOMAMAGER_H

#if __cplusplus >= 202002L
#include "iomanager_cpp20.h"
namespace server {
typedef IOManager_ IOManager;
}

#else
#include "iomanager_cpp17.h"
namespace server {
}

#endif


#endif
