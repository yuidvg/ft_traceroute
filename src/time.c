#include "all.h"

struct timeval timeDifference(const struct timeval start, const struct timeval end)
{
    struct timeval result;
    timersub(&end, &start, &result);
    return result;
}

struct timeval timeSum(const struct timeval time1, const struct timeval time2)
{
    struct timeval result;
    timeradd(&time1, &time2, &result);
    return result;
}

double_t timeValInMiliseconds(const struct timeval timeVal)
{
    return timeVal.tv_sec * 1000.0 + timeVal.tv_usec / 1000.0;
}

struct timeval timeOfDay()
{
    struct timeval time;
    gettimeofday(&time, NULL);
    return time;
}

// static double get_timeout(Probe *pb)
// {
//     double value;
//     extern const double here_factor;
//     extern const double near_factor;
//     extern const double DEF_WAIT_PREC;
//     extern const double wait_secs;
//     extern const int probes_per_hop;
//     extern const int num_probes;
//     extern Probe probes[];

//     if (here_factor)
//     {
//         /*  check for already replied from the same hop   */
//         int i, idx = (pb - probes);
//         Probe *p = &probes[idx - (idx % probes_per_hop)];

//         for (i = 0; i < probes_per_hop; i++, p++)
//         {
//             // /*   `p == pb' skipped since  !pb->done   */

//             // // if (p->done && (value = p->recv_time - p->send_time) > 0)
//             // // {
//             // //     value += DEF_WAIT_PREC;
//             // //     value *= here_factor;
//             // //     return value < wait_secs ? value : wait_secs;
//             // // }
//         }
//     }

//     if (near_factor)
//     {
//         /*  check forward for already replied   */
//         Probe *p, *endp = probes + num_probes;

//         for (p = pb + 1; p < endp && p->send_time; p++)
//         {

//             if (p->done && (value = p->recv_time - p->send_time) > 0)
//             {
//                 value += DEF_WAIT_PREC;
//                 value *= near_factor;
//                 return value < wait_secs ? value : wait_secs;
//             }
//         }
//     }

//     return wait_secs;
// }
