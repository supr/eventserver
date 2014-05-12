#include <cstdint>
#include <iostream>
#include <cstring>

#include <error.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netdb.h>

#include "eventserver.h"

namespace net
{
  namespace prudhvi
  {
    EventServer::EventServer(uint16_t port, uint16_t maxevents)
      : port_(port), maxevents_(maxevents)
    {
      bool rv = bind();
      if(!rv)
        throw "Unable to bind";
      
      rv = set_non_blocking(serverfd_);
      if(!rv)
        throw "Unable to set socket to non-blocking";

      int s = listen(serverfd_, SOMAXCONN);
      if(s == -1)
      {
        std::cerr << "listen: " << std::strerror(errno) << std::endl;
        throw "Unable to listen";
      }

      epollfd_ = epoll_create1(0);
      if(epollfd_ == -1)
      {
        std::cerr << "epoll_create1: " << std::strerror(errno) << std::endl;
        throw "Unable to create epoll descriptor";
      }

      event_.data.fd = serverfd_;
      event_.events  = EPOLLIN | EPOLLET;

      s = epoll_ctl(epollfd_, EPOLL_CTL_ADD, serverfd_, &event_);
      if(s == -1)
      {
        std::cerr << "epoll_ctl: " << std::strerror(errno) << std::endl;
        throw "Unable to add server socket to epoll descriptor";
      }

      events_ = (struct epoll_event*)calloc(maxevents_, sizeof(event_));
      if(events_ == nullptr)
      {
        std::cerr << "calloc: unable to allocate events" << std::endl;
        throw "Unable to allocate memory for events";
      }
    }

    bool EventServer::bind() noexcept
    {
      struct addrinfo hints, *results = nullptr, *rp = nullptr;

      std::memset(&hints, 0, sizeof(struct addrinfo));
      hints.ai_family   = AF_INET;
      hints.ai_socktype = SOCK_STREAM;
      hints.ai_flags    = AI_PASSIVE;

      int sfd, s = getaddrinfo(NULL, std::to_string(port_).c_str(), &hints, &results);
      if(s != 0)
      {
        std::cerr << "getaddrinfo: " << gai_strerror(s) << std::endl;
        return false;
      }

      for(rp = results; rp; rp = rp->ai_next)
      {
        sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if(sfd == -1)
          continue;

        s = ::bind(sfd, rp->ai_addr, rp->ai_addrlen);
        if(s == 0)
          break;

        close(sfd);
      }

      if(rp == nullptr)
      {
        std::cerr << "bind: no suitable interfaces found" << std::endl;
        return false;
      }

      freeaddrinfo(results);
      serverfd_ = sfd;
      
      return true;
    }

    bool EventServer::set_non_blocking(int64_t fd) noexcept
    {
      int flags = fcntl(fd, F_GETFL, 0);
      if(flags == -1)
      {
        std::cerr << "fcntl: " << std::strerror(errno) << std::endl;
        return false;
      }

      flags |= O_NONBLOCK;
      int rv = fcntl(fd, F_SETFL, flags);
      if(rv == -1)
      {
        std::cerr << "fcntl: " << std::strerror(errno) << std::endl;
        return false;
      }

      return true;
    }

    EventServer::~EventServer()
    {
      close(serverfd_);
    }
  } /* End Namespace prudhvi */
} /* End Namespace net */
