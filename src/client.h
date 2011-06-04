/* Test parameters.

   By fredfsh (fredfsh@gmail.com)
*/
#ifndef CLIENT_H_
#define CLIENT_H_

#define ms(begin, end) \
    (  ((end.tv_sec - begin.tv_sec) * 1000000 + \
        (end.tv_usec - begin.tv_usec)            ) / 1000  )

#endif
