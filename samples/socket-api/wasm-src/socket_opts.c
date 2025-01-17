#include <arpa/inet.h>
#include <errno.h>
#include <netinet/tcp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#ifdef __wasi__
#include <wasi_socket_ext.h>
#endif

#define OPTION_ASSERT(A, B, OPTION)           \
    if (A == B) {                             \
        printf("%s is expected\n", OPTION);   \
    }                                         \
    else {                                    \
        printf("%s is unexpected\n", OPTION); \
        perror("assertion failed");           \
        return EXIT_FAILURE;                  \
    }

struct timeval
to_timeval(time_t tv_sec, suseconds_t tv_usec)
{
    struct timeval tv = { tv_sec, tv_usec };
    return tv;
}

int
set_and_get_bool_opt(int socket_fd, int level, int optname, int val)
{
    int bool_opt = val;
    socklen_t opt_len = sizeof(bool_opt);
    setsockopt(socket_fd, level, optname, &bool_opt, sizeof(bool_opt));
    bool_opt = !bool_opt;
    getsockopt(socket_fd, level, optname, &bool_opt, &opt_len);
    return bool_opt;
}

int
main(int argc, char *argv[])
{
    int tcp_socket_fd = 0;
    int udp_socket_fd = 0;
    int udp_ipv6_socket_fd = 0;
    struct timeval tv;
    socklen_t opt_len;
    int buf_len;
    int result;
    struct linger linger_opt;
    uint32_t time_s;
    struct ip_mreq mcast;
    struct ipv6_mreq mcast_ipv6;
    unsigned char ttl;

    printf("[Client] Create TCP socket\n");
    tcp_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (tcp_socket_fd == -1) {
        perror("Create socket failed");
        return EXIT_FAILURE;
    }

    printf("[Client] Create UDP socket\n");
    udp_socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_socket_fd == -1) {
        perror("Create socket failed");
        return EXIT_FAILURE;
    }

    printf("[Client] Create UDP IPv6 socket\n");
    udp_ipv6_socket_fd = socket(AF_INET6, SOCK_DGRAM, 0);
    if (udp_ipv6_socket_fd == -1) {
        perror("Create socket failed");
        return EXIT_FAILURE;
    }

    // SO_RCVTIMEO
    tv = to_timeval(123, 1000);
    setsockopt(tcp_socket_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    tv = to_timeval(0, 0);
    opt_len = sizeof(tv);
    getsockopt(tcp_socket_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, &opt_len);
    OPTION_ASSERT(tv.tv_sec, 123, "SO_RCVTIMEO tv_sec");
    OPTION_ASSERT(tv.tv_usec, 1000, "SO_RCVTIMEO tv_usec");

    // SO_SNDTIMEO
    tv = to_timeval(456, 2000);
    setsockopt(tcp_socket_fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
    tv = to_timeval(0, 0);
    opt_len = sizeof(tv);
    getsockopt(tcp_socket_fd, SOL_SOCKET, SO_SNDTIMEO, &tv, &opt_len);
    OPTION_ASSERT(tv.tv_sec, 456, "SO_SNDTIMEO tv_sec");
    OPTION_ASSERT(tv.tv_usec, 2000, "SO_SNDTIMEO tv_usec");

    // SO_SNDBUF
    buf_len = 8192;
    setsockopt(tcp_socket_fd, SOL_SOCKET, SO_SNDBUF, &buf_len, sizeof(buf_len));
    buf_len = 0;
    opt_len = sizeof(buf_len);
    getsockopt(tcp_socket_fd, SOL_SOCKET, SO_SNDBUF, &buf_len, &opt_len);
    OPTION_ASSERT(buf_len, 16384, "SO_SNDBUF buf_len");

    // SO_RCVBUF
    buf_len = 4096;
    setsockopt(tcp_socket_fd, SOL_SOCKET, SO_RCVBUF, &buf_len, sizeof(buf_len));
    buf_len = 0;
    opt_len = sizeof(buf_len);
    getsockopt(tcp_socket_fd, SOL_SOCKET, SO_RCVBUF, &buf_len, &opt_len);
    OPTION_ASSERT(buf_len, 8192, "SO_RCVBUF buf_len");

    // SO_KEEPALIVE
    OPTION_ASSERT(
        set_and_get_bool_opt(tcp_socket_fd, SOL_SOCKET, SO_KEEPALIVE, 1), 1,
        "SO_KEEPALIVE enabled");
    OPTION_ASSERT(
        set_and_get_bool_opt(tcp_socket_fd, SOL_SOCKET, SO_KEEPALIVE, 0), 0,
        "SO_KEEPALIVE disabled");

    // SO_REUSEADDR
    OPTION_ASSERT(
        set_and_get_bool_opt(tcp_socket_fd, SOL_SOCKET, SO_REUSEADDR, 1), 1,
        "SO_REUSEADDR enabled");
    OPTION_ASSERT(
        set_and_get_bool_opt(tcp_socket_fd, SOL_SOCKET, SO_REUSEADDR, 0), 0,
        "SO_REUSEADDR disabled");

    // SO_REUSEPORT
    OPTION_ASSERT(
        set_and_get_bool_opt(tcp_socket_fd, SOL_SOCKET, SO_REUSEPORT, 1), 1,
        "SO_REUSEPORT enabled");
    OPTION_ASSERT(
        set_and_get_bool_opt(tcp_socket_fd, SOL_SOCKET, SO_REUSEPORT, 0), 0,
        "SO_REUSEPORT disabled");

    // SO_LINGER
    linger_opt.l_onoff = 1;
    linger_opt.l_linger = 10;
    setsockopt(tcp_socket_fd, SOL_SOCKET, SO_LINGER, &linger_opt,
               sizeof(linger_opt));
    linger_opt.l_onoff = 0;
    linger_opt.l_linger = 0;
    opt_len = sizeof(linger_opt);
    getsockopt(tcp_socket_fd, SOL_SOCKET, SO_LINGER, &linger_opt, &opt_len);
    OPTION_ASSERT(linger_opt.l_onoff, 1, "SO_LINGER l_onoff");
    OPTION_ASSERT(linger_opt.l_linger, 10, "SO_LINGER l_linger");

    // SO_BROADCAST
    OPTION_ASSERT(
        set_and_get_bool_opt(udp_socket_fd, SOL_SOCKET, SO_BROADCAST, 1), 1,
        "SO_BROADCAST enabled");
    OPTION_ASSERT(
        set_and_get_bool_opt(udp_socket_fd, SOL_SOCKET, SO_BROADCAST, 0), 0,
        "SO_BROADCAST disabled");

    // TCP_KEEPIDLE
    time_s = 16;
    setsockopt(tcp_socket_fd, IPPROTO_TCP, TCP_KEEPIDLE, &time_s,
               sizeof(time_s));
    time_s = 0;
    opt_len = sizeof(time_s);
    getsockopt(tcp_socket_fd, IPPROTO_TCP, TCP_KEEPIDLE, &time_s, &opt_len);
    OPTION_ASSERT(time_s, 16, "TCP_KEEPIDLE");

    // TCP_KEEPINTVL
    time_s = 8;
    setsockopt(tcp_socket_fd, IPPROTO_TCP, TCP_KEEPINTVL, &time_s,
               sizeof(time_s));
    time_s = 0;
    opt_len = sizeof(time_s);
    getsockopt(tcp_socket_fd, IPPROTO_TCP, TCP_KEEPINTVL, &time_s, &opt_len);
    OPTION_ASSERT(time_s, 8, "TCP_KEEPINTVL");

    // TCP_FASTOPEN_CONNECT
    OPTION_ASSERT(set_and_get_bool_opt(tcp_socket_fd, IPPROTO_TCP,
                                       TCP_FASTOPEN_CONNECT, 1),
                  1, "TCP_FASTOPEN_CONNECT enabled");
    OPTION_ASSERT(set_and_get_bool_opt(tcp_socket_fd, IPPROTO_TCP,
                                       TCP_FASTOPEN_CONNECT, 0),
                  0, "TCP_FASTOPEN_CONNECT disabled");

    // TCP_NODELAY
    OPTION_ASSERT(
        set_and_get_bool_opt(tcp_socket_fd, IPPROTO_TCP, TCP_NODELAY, 1), 1,
        "TCP_NODELAY enabled");
    OPTION_ASSERT(
        set_and_get_bool_opt(tcp_socket_fd, IPPROTO_TCP, TCP_NODELAY, 0), 0,
        "TCP_NODELAY disabled");

    // TCP_QUICKACK
    OPTION_ASSERT(
        set_and_get_bool_opt(tcp_socket_fd, IPPROTO_TCP, TCP_QUICKACK, 1), 1,
        "TCP_QUICKACK enabled");
    OPTION_ASSERT(
        set_and_get_bool_opt(tcp_socket_fd, IPPROTO_TCP, TCP_QUICKACK, 0), 0,
        "TCP_QUICKACK disabled");

    // IP_TTL
    ttl = 8;
    setsockopt(tcp_socket_fd, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl));
    ttl = 0;
    opt_len = sizeof(ttl);
    getsockopt(tcp_socket_fd, IPPROTO_IP, IP_TTL, &ttl, &opt_len);
    OPTION_ASSERT(ttl, 8, "IP_TTL");

    // IPV6_V6ONLY
    OPTION_ASSERT(
        set_and_get_bool_opt(udp_ipv6_socket_fd, IPPROTO_IPV6, IPV6_V6ONLY, 1),
        1, "IPV6_V6ONLY enabled");
    OPTION_ASSERT(
        set_and_get_bool_opt(udp_ipv6_socket_fd, IPPROTO_IPV6, IPV6_V6ONLY, 0),
        0, "IPV6_V6ONLY disabled");

    // IP_MULTICAST_LOOP
    OPTION_ASSERT(
        set_and_get_bool_opt(udp_socket_fd, IPPROTO_IP, IP_MULTICAST_LOOP, 1),
        1, "IP_MULTICAST_LOOP enabled");
    OPTION_ASSERT(
        set_and_get_bool_opt(udp_socket_fd, IPPROTO_IP, IP_MULTICAST_LOOP, 0),
        0, "IP_MULTICAST_LOOP disabled");

    // IP_ADD_MEMBERSHIP
    mcast.imr_multiaddr.s_addr = 16777440;
    mcast.imr_interface.s_addr = htonl(INADDR_ANY);
    result = setsockopt(udp_socket_fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mcast,
                        sizeof(mcast));
    OPTION_ASSERT(result, 0, "IP_ADD_MEMBERSHIP");

    // IP_DROP_MEMBERSHIP
    result = setsockopt(udp_socket_fd, IPPROTO_IP, IP_DROP_MEMBERSHIP, &mcast,
                        sizeof(mcast));
    OPTION_ASSERT(result, 0, "IP_DROP_MEMBERSHIP");

    // IP_MULTICAST_TTL
    ttl = 8;
    setsockopt(udp_socket_fd, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl));
    ttl = 0;
    opt_len = sizeof(ttl);
    getsockopt(udp_socket_fd, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, &opt_len);
    OPTION_ASSERT(ttl, 8, "IP_MULTICAST_TTL");

    // IPV6_MULTICAST_LOOP
    OPTION_ASSERT(set_and_get_bool_opt(udp_ipv6_socket_fd, IPPROTO_IPV6,
                                       IPV6_MULTICAST_LOOP, 1),
                  1, "IPV6_MULTICAST_LOOP enabled");
    OPTION_ASSERT(set_and_get_bool_opt(udp_ipv6_socket_fd, IPPROTO_IPV6,
                                       IPV6_MULTICAST_LOOP, 0),
                  0, "IPV6_MULTICAST_LOOP disabled");

    // IPV6_JOIN_GROUP
    setsockopt(udp_ipv6_socket_fd, IPPROTO_IPV6, IPV6_JOIN_GROUP, &mcast_ipv6,
               sizeof(mcast_ipv6));

    // IPV6_LEAVE_GROUP
    setsockopt(udp_ipv6_socket_fd, IPPROTO_IPV6, IPV6_LEAVE_GROUP, &mcast_ipv6,
               sizeof(mcast_ipv6));

    printf("[Client] Close sockets\n");
    close(tcp_socket_fd);
    close(udp_socket_fd);
    return EXIT_SUCCESS;
}
