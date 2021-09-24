#pragma once 

#include<fcntl.h>  //提供fcntl函数
//#include<type.h>
#include<unistd.h>  //提供sleep函数
#include<stdint.h>  //提供uint64_t的定义
#include <sys/ioctl.h>  //提供ioctl函数
#include<sys/socket.h>
#include<time.h> //nanosleep函数,nanosleep可以很好的保留中断时剩余时间,是比sleep()函数更高精度的时间函数


namespace server{
    bool is_hook_enable();
    void set_hook_enable(bool flag);
}

extern "C"{
    //sleep
    
    typedef unsigned int (*sleep_fun)(unsigned int seconds);
    extern sleep_fun sleep_f;

    typedef int (*usleep_fun)(useconds_t usec);
    extern usleep_fun usleep_f;

    typedef int (*nanosleep_fun)(const struct timespec *req, struct timespec *rem);
    extern nanosleep_fun nanosleep_f;

    //socket

    //socket函数
    typedef int (*socket_fun)(int domain, int type, int protocol);
    extern socket_fun socket_f;

    extern int connect_with_timeout(int fd, const struct sockaddr* addr,socklen_t addrlen, uint64_t timeout_ms);
    //connect函数
    typedef int (*connect_fun)(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
    extern connect_fun connect_f;

    //accept函数
    typedef int (*accept_fun)(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
    extern accept_fun accept_f;


    //read

    //read函数
    typedef ssize_t (*read_fun)(int fd, void *buf, size_t count);
    extern read_fun read_f;

    //readv函数
    typedef ssize_t (*readv_fun)(int fd, const struct iovec *iov, int iovcnt);
    extern readv_fun readv_f;

    //recv函数
    typedef ssize_t (*recv_fun)(int sockfd, void *buf, size_t len, int flags);
    extern recv_fun recv_f;

    //recvfrom函数
    typedef ssize_t (*recvfrom_fun)(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen);
    extern recvfrom_fun recvfrom_f;

    //recvmsg函数
    typedef ssize_t (*recvmsg_fun)(int sockfd, struct msghdr *msg, int flags);
    extern recvmsg_fun recvmsg_f;


    //wtite

    //write函数
    typedef ssize_t (*write_fun)(int fd, const void *buf, size_t count);
    extern write_fun write_f;

    //writev函数
    typedef ssize_t (*writev_fun)(int fd, const struct iovec *iov, int iovcnt);
    extern writev_fun writev_f;

    //send函数
    typedef ssize_t (*send_fun)(int sockfd, const void *buf, size_t len, int flags);
    extern send_fun send_f;

    //sendto函数
    typedef ssize_t (*sendto_fun)(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen);
    extern sendto_fun sendto_f;

    //sendmsg函数
    typedef ssize_t (*sendmsg_fun)(int sockfd, const struct msghdr *msg, int flags);
    extern sendmsg_fun sendmsg_f;

    //control

    //close函数
    typedef int (*close_fun)(int fd);
    extern close_fun close_f;

    //fctnl函数
    typedef int (*fcntl_fun)(int fd, int cmd, ... /* arg */ );
    extern fcntl_fun fcntl_f;

    //ioctl函数
    typedef int (*ioctl_fun)(int fd, unsigned long request, ...);
    extern ioctl_fun ioctl_f;

    //socketopt

    //getsockopt函数
    typedef int (*getsockopt_fun)(int sockfd, int level, int optname, void *optval, socklen_t *optlen);
    extern getsockopt_fun getsockopt_f;

    //setsockopt函数
    typedef int (*setsockopt_fun)(int sockfd, int level, int optname, const void *optval, socklen_t optlen);
    extern setsockopt_fun setsockopt_f;

}
