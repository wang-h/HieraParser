#ifndef  GET_WALL_H
#define GET_WALL_H
#include <sys/time.h>
#include <time.h>
static double get_wall_time()
{
    struct timeval time;
    if (gettimeofday(&time, NULL))
    {
      //  Handle error
      return 0;
    }
    return (double)time.tv_sec + (double)time.tv_usec * .000001;
}
#endif // GET_WALL_H