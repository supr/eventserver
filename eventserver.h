#ifndef _EVENTSERVER_H__
#define _EVENTSERVER_H__

#include <sys/epoll.h>

namespace net
{
  namespace prudhvi
  {
    struct event_ctx
    {
      uint64_t epoll_fd;
      uint64_t events;
      uint64_t server_fd;
      uint64_t client_fd;
    };

    typedef struct event_ctx event_ctx_t;
    typedef void (*event_handle_t)(const event_ctx_t* ctx);

    class EventServer
    {
      public:
        explicit EventServer(uint16_t port, uint16_t maxevents = 64);
        ~EventServer();
        bool run(event_handle_t handler) noexcept;

      private:
        bool bind() noexcept;
        bool set_non_blocking(int64_t fd) noexcept;

        uint16_t maxevents_;
        uint16_t port_;
        int64_t serverfd_;
        int64_t epollfd_;
        struct epoll_event event_, *events_ = nullptr;
    };
  }
}
#endif
