#if __cplusplus >= 202002L

#include "fiber_cpp20.h"
namespace server {
typedef Fiber_ Fiber;
}

#else

#include "fiber_cpp17.h"
namespace server{
typedef Fiber_1 Fiber;
}
#endif
