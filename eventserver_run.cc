#include <cstdio>
#include <cstring>
#include <iostream>

#include <error.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/epoll.h>

#include "eventserver.h"

namespace net {
  namespace prudhvi {
    bool EventServer::run(event_handle_t handler) noexcept
    {
      for(;;)
      {
        int n = epoll_wait(epollfd_, events_, maxevents_, -1);
        for(int i = 0; i < n; i++)
        {
          if((events_[i].events & EPOLLERR) ||
             (events_[i].events & EPOLLHUP))
          {
            std::cerr << "epoll error" << std::endl;
            close(events_[i].data.fd);
            continue;
          }
          else if(serverfd_ == events_[i].data.fd)
          {
            for(;;)
            {
              struct sockaddr in_addr;
              socklen_t in_len = sizeof(in_addr);
              char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];

              int infd = accept(serverfd_, &in_addr, &in_len);
              if(infd == -1)
              {
                if((errno == EAGAIN) || (errno == EWOULDBLOCK))
                  break;
                else
                {
                  std::cerr << "accept: " << std::strerror(errno) << std::endl;
                  break;
                }
              }

              int s = getnameinfo(&in_addr, in_len, hbuf, sizeof(hbuf), sbuf, sizeof(sbuf),
                  NI_NAMEREQD | NI_NUMERICSERV);
              if(!s)
              {
                std::fprintf(::stderr, "Accepted connection on fd %d (host=%s, port=%s)", infd, hbuf, sbuf);
                std::cerr << std::endl;
              }

              /*
               * Make client socket non-blocking
               */

              s = set_non_blocking(infd);
              if(!s)
              {
                std::fprintf(::stderr, "Unable to mark socket %d non-blocking (host=%s, port=%s)", infd, hbuf, sbuf);
                std::cerr << std::endl;
                continue;
              }

              event_.data.fd = infd;
              event_.events = EPOLLIN | EPOLLOUT | EPOLLET;
              s = epoll_ctl(epollfd_, EPOLL_CTL_ADD, infd, &event_);
              if(s == -1)
              {
                std::cerr << "epoll_ctl: " << std::strerror(errno) << std::endl;
                continue;
              }
            }
            continue;
          } /* EndIf socker = server socket */
          else {
            event_ctx_t ctx = {};
            ctx.epoll_fd = epollfd_;
            ctx.server_fd = serverfd_;
            ctx.client_fd = events_[i].data.fd;
            ctx.events = events_[i].events;
            handler(&ctx);
          } /* Client Socket are ready for Input & Output */
        } /* End Processing Events */
      } /* End For(;;) */

      /* This should never happen. If it did return false */
      return false;
    }
  }
}
