# tcp-tunnel 设计思路
- 使用 libuv 网络库
- 使用多线程而非多进程
- 纯 TCP，不使用任何加密

# libuv 用法简介
**事件循环对象**
- 每个线程都有一个 uv_loop_t 事件循环对象，这个和 libevent 的 event_base 对象是一样的，本质都是 epoll_fd。uv_loop_t 的 data 字段可以用作 thread local 存储。
- 使用 `uv_loop_init()` 来初始化 loop 结构体。注意，在 libuv 中，libuv 不负责分配内存以及释放内存，只负责填充数据以及释放数据，内存分配与释放由我们自己管理。
- loop 对象初始化之后，就可以添加我们的监听事件，比如注册 tcp 监听套接字、udp 监听套接字等，最后调用 `uv_run()` 运行事件循环即可。uv_run() 后不应该有任何代码。

**事件对象 handle**
- uv_handle_t 和 libevent 的 `struct event` 是一样的概念，事件对象。其实就是 epoll 中的 `struct epoll_event`。
- 相关的 callback 为 `uv_alloc_cb`，该回调在 tcp/udp 的 read_cb 前执行，我们需要负责填充 `uv_buf_t`（base、len）。
- 所有的 handler 都有一个 `close_cb` 关闭回调。调用 `uv_close()` 关闭某个事件对象时，需要传入关闭回调，必须在关闭回调中释放内存。
- handle 对象也有一个 data 字段，存储我们的用户数据，另外还有一个 loop 字段，存储与之关联的 loop 对象，libuv 的 api 设计还是不错的。
- `uv_is_active()` 用来检测一个 handler 是否为 pending 状态，调用 handler 对象的 start() 方法后，就处于 pending 状态，常用于检测 timer。对于 tcp/udp/stream 涉及 IO 操作的 handle，只要它们正在执行 IO 操作，比如 connecting、accepting、reading、writing 等，都处于 pending 状态。
- 调用 uv_close() 关闭一个 handler 时，底层 fd 将立即释放，但 close_cb 会在下次事件循环中运行，如果该 handler 上还有正在进行的 request，那么这些 request 的回调将被调用，status 为 UV_ECANCELED。
- 对于 io 类型的 handler，可以使用 `uv_fileno()` 来获取底层的 fd。

**请求对象 request**
- libevent、libev 中没有概念，request 是 libuv 对一次性操作的抽象概念，比如发送一段数据，这就是一个请求，发送完毕后这个请求的生命周期就结束了。
- `uv_req_t` 也有 data 字段，存储我们的用户数据，对于与 handler 关联的 request，还有一个 `handle` 字段，通过它可以拿到与之关联的 uv_handle_t 对象。
- uv_req_t 的 callback 根据不同的类型而不同，比如 stream 类型的 write_req（写请求）、shutdown_req（关闭请求）、connect_req（连接请求），回调不一样。

**uv_timer_t 定时器**
- 定时器回调：`void (*uv_timer_cb)(uv_timer_t* handle)`，当定时器到期，回调将被调用。
- 初始化一个定时器对象：`uv_timer_init()`，时间单位为毫秒，而不是 libevent 的 struct timeval 结构体。
- 启动一个计时器对象：`uv_timer_start()`，对于已经 pending 的 timer，调用它会重置定时器，这个和 libevent 是一样的。
- 停止一个计时器对象：`uv_timer_stop()`，取消对应的定时器，这个函数是幂等的，可以多次调用，即使不是 pending 状态。
- 释放定时器结构：`uv_close()`，都是通用的 handler 关闭 api，内存释放以及其它资源释放都应该在 close_cb 回调中执行。

**uv_stream_t 流对象**
- uv_stream_t 是 tcp、unix_domain_socket、console_file（stdin、stdout、stderr）的抽象，我们只需关心 tcp 的相关东西。
- 连接请求：`uv_connect_t`，当连接成功或失败，则调用我们传递给它的回调。请求对象有一个 handle 字段，存储与之关联的 handler 对象。
- 写入请求：`uv_write_t`，当写入成功或失败，则调用我们的传递给它的回调。写入请求可能会排队，回调调用之后才能重用 write_req 对象。写入对象也有一个 handler 字段，指向执行此写入请求的 handler 对象。
- 关闭请求：`uv_shutdown_t`，关闭请求的工作原理其实是这样的，因为 stream_t 对象有一个写入队列，当这个队列上的所有写入请求都执行完毕后，libuv 会发送 FIN 给对端，然后调用我们的回调。handle 字段同上。
- `uv_accpet_cb` 用于监听套接字：`void (*uv_accept_cb)(uv_stream_t* server, int status)`，其中 server 是我们的监听套接字 handler 对象，status 如果为负数则表示发生错误。
- `uv_read_cb` 数据可读回调，注意，read_cb 是在 libuv 从套接字收到数据后，才调用的，因此当 read_cb 调用后，我们就可以从 uv_buf_t 中拿到数据，不用从 socket 中读取它。
  - `nread > 0`：表示成功读取到数据，这些数据存放在 buf_t.base 缓冲区，同时 nread 表示该缓冲区中的有效数据长度。
  - `nread < 0`：表示在读取数据是发生了 socket 错误，此时 nread 为 -errno。注意，收到 EOF/FIN 时，nread 为 UV_EOF，而不是 0。
  - `nread == 0`：注意等于零不是对方关闭了连接，而是等效于 EAGAIN 错误代码，我们直接忽略 nread = 0 的情况，没有必要去处理它。
  - 当 nread 小于 0 时，应该调用 uv_read_stop() 来停止读取，或者调用 uv_close() 来关闭 handler，因为发生错误后不能再次读取数据。
- `uv_listen()`：执行 listen() 系统调用，将套接字变为监听套接字。
- `uv_accept()`：执行 accept() 系统调用，从监听套接字中接受连接。我们应该传入一个已初始化的 uv_tcp_t 结构体。
- `uv_shutdown()` 发送关闭请求，当 stream 对象上写入请求全部执行完毕后，libuv 将调用 `shutdown(SHUT_WR)` 系统调用。
- `uv_write()`：发送写入请求，当此写入请求执行完毕后，libuv 会调用我们的 write_cb。注意，uv_write() 会将我们的写入请求进行排队。
- `uv_try_write()`：尝试立即写入数据，等效于 `send()` 系统调用，可能一个字节都没发送，可能全部发送完毕，可能是只发送了部分数据。
- `uv_read_start()`：将套接字加入事件循环，监听可读事件，读到数据后会调用 read_cb。遗憾的是，libuv 没有类似 lievent 的 event_cb。
- `uv_read_stop()`：将套接字从事件循环中移除，取消监听可读事件，stop 后可再次 start。在已停止的 handle 上多次调用 stop 是安全的。

**uv_tcp_t 对象**
- `uv_tcp_init()` 用来初始化 uv_tcp_t 句柄结构，uv_tcp_t 是 uv_stream_t 的子类，可进行类型转换，注意它并没有创建套接字。
- `uv_tcp_open()` API 设置 tcp 对象的底层套接字，通常只用于监听套接字，对于 accept 出来的套接字，libuv 会自动为其设置 fd。
- `uv_tcp_bind()` API 将给底层 fd 绑定一个 source 地址，作用同 bind() 系统调用。成功调用此函数并不意味着地址绑定成功。
- `uv_tcp_nodelay()` API 会给底层 fd 设置 nodelay 选项，通常只用于已连接的套接字，对于监听套接字，调用它是没有意义的。
- `uv_tcp_connect()` 发送 connect 请求，在连接成功或连接失败后，会调用我们的回调函数。
- `uv_tcp_close_reset()` 向对端发送 RST，`uv_shutdown()` 与 `uv_tcp_close_reset()` 只能用一个。
- `uv_tcp_getsockname()` 等价于 getsockname() 系统调用，获取底层套接字的 self socket address。
- `uv_tcp_getpeername()` 等价于 getpeername() 系统调用，获取底层套接字的 peer socket address。

# 客户端流程
**启动逻辑**
1. 以线程为单位，每个线程都有各自的 loop 对象，一个 tcp 监听套接字（设置端口重用），一条 tunnel 长连接，长连接没有协议层的握手。
2. 线程启动后，立即尝试建立 tunnel 长连接，只有 tunnel 长连接建立成功的前提下，才能接收新连接，一旦 tunnel 连接断开，将立即进行重连。
3. tunnel 连接断开意味着之前接收的 client 连接全部失效，因此，tunnel 连接断开后，需要给所有未关闭的 client 连接发送 TCP RST，重置连接。
4. 因此 tcp-tunnel 需要使用 uthash，实现一个 hashset，用于存储已接收的所有 tcp 连接对象（如果连接关闭或发生错误则从 hashset 中移除它）。
5. 我们可以利用 loop 对象的 data 字段作为我们的 thread local。必要的字段有：
  - `tunnel_obj`：tunnel 长连接对象，类型应该为 `uv_tcp_t`，不应该设置它的 data 字段，放到 loop->data 中即可。
  - `tunnel_buf`：长连接的 recv buffer，用于存储从 tunnel 连接收到的私有协议数据，我们需要在此 buffer 中进行拆包。
  - `tunnel_buf_offset`：在进行拆包的过程中，在 buffer 的末尾，可能收到的不是一个完整协议头，这种情况下我们必须将这部分数据 memmove 到 buffer 的开头，因此下次调用 uv_alloc_cb() 时应该将 base 指针往后移动，否则 libuv 读取的数据会覆盖这个残留的协议头，这就是 begin_offset 的作用。
  - `tunnel_buf_nread`：nread 比较熟悉，就是存储 buffer 缓冲区的有效数据长度，应用场景：进行拆包时，大多数协议都能立即处理完毕，唯独 data_packet 协议不行，data 协议中附带 payload，当解析到 data 协议时，我们会通过指针直接拿到 client 对象，然后在 client 对象上调用 try_write()，如果没有全部写入完毕，那么需要在 client 上启动一个 write 请求，并将下个协议的起始指针存储到 write 请求的 data 字段，同时 tunnel 的 read 需要暂停，当 write 的回调被调用时，说明已处理完毕，那么我们拿到下个协议的起始指针后，需要借助 tunnel_buf 和 nread 来计算剩余的长度，然后将后半段 buf 指针以及 len 传递给拆包函数。
  - `tunnel_timer`：长连接的 ack 定时器，用于检测连接是否断开，当 client 调用 uv_write() 在 tunnel 连接上排队写入请求时，如果 timer 未启动则启动它，如果 timer 已启动说明之前已进行处理，此时不能再次启动，否则定时器会被重置，这会导致 bug；当我们从服务器收到 ack 协议后，应立即停止 timer；如果 timer 到期，说明在一定时间内没有从服务器收到 ack 协议，证明连接存在问题，则进行重连操作，同时清掉所有已接收的 client 连接。
  - `tunnel_isready`：布尔值，指示 tunnel 长连接是否已建立，因为 libuv 没有 listen_stop() API，因此我们随时会从监听套接字收到新连接，因此当我们接受了新连接之后应首先判断 tunnel 是否已连接，如果为为连接，则发送 TCP RST 重置它，不进行服务。
  - `client_set`：所有的 client 连接对象的集合，应该使用 uthash 进行实现，这样才能实现高效率。应该只有 key（uv_tcp_t 指针），没有 value。

**收到新连接**
1. 当 accept_cb 触发时，收到一个新的 client 连接，首先检查 tunnel_is_ready，如果 tunnel 未建立，则直接 reset。
2. 因为 client 是透明代理，因此需要获取 client 连接的原始目的地址，根据 redirect 还是 tproxy 模式，调用不同 api。
3. 然后创建 client 的 data 结构体，此结构体为 client 对象的上下文，应有如下字段：
  - peer_stream_ptr：与该 client 相关联的服务器端 client 对象的指针，数据类型应该为 uint64_t，这个字段在 establish_packet 返回后，进行填充。
  - tunnel_write_req：这个写入请求对象是用于在 tunnel 长连接上发送当前连接相关的协议数据。data 字段应该存储 client 对象的指针，不然后续会拿不到数据。
  - client_write_req：这个写入请求对象是用于将 tunnel 接收缓冲区中读取到的 payload 发送给本地的 client 连接。data 字段存储下一个协议包的起始地址（指针）。
  - recv_buffer：协议缓冲区，用于从 client 端接收 socket 数据，此 buffer 头部是协议头：connect_packet、data_packet（带payload）、close_packet、error_packet。
  - client_status：状态标记，0 表示未发生任何事件：
    - IN_TUNNEL_WRITE_QUEUE：指示本地 client 正在 tunnel 的发送队列中进行排队。
    - CLOSE_CONN_AFTER_QUEUE：指示本地 client 在排队结束后，会调用 uv_close() 来关闭连接。
    - RESET_CONN_AFTER_QUEUE：指示本地 client 在排队结束后，会调用 uv_tcp_close_reset() 来重置连接。
    - SEND_ERROR_AFTER_QUEUE：指示本地 client 在排队结束后，会构造一个 error_packet，然后 uv_write() 进行排队。
- 拿到目的地址后，生成 connect_packet 协议包，协议包的存储位置就是 recv_buffer，然后我们在 tunnel 上调用 uv_write(tunnel_write_req) 进行排队，同时检查 ack timer，更新 client_status。
- 当 tunnel_write_req 的 write_cb 被调用后，说明协议已成功发送，更新 client_status，去除 PENDING 标记，然后处理完毕。
- 当 tunnel 的 read_cb 触发后，说明从服务端收到了数据，然后调用拆包函数，处理数据包，传入 base、len 参数，总共有 5 种协议类型：
  - ack_packet：从服务端收到了 ack 消息，处理逻辑：停止 ack timer。
  - establish_packet：服务端那边已经成功与目的地址建立了连接，处理逻辑：检查指针所指向的 client 是否真实存在，方式恶意攻击；如果存在，则将服务器那端的 client 指针存到本地 client 的 data 字段，然后 start 本地 client 的读取。
  - data_packet：表示从对端收到 payload，首先也是进行基本的安全检查，然后先使用 try_write() 进行非阻塞发送，如果都发送完了，那么处理完毕；如果发送未完成，则取出 client 上下文的 write_req，暂停 tunnel 的读取，注册 client 的写入请求，当写入回调触发后，说明写入完成，于是再次调用拆包函数，并将下个协议包的起始地址以及剩余的长度传递给拆包函数，拆包函数返回之前，应该检查一下 tunnel 的读取是否已经 start，如果未 start，则应该调用 read_start，防止之后读取事件未注册。
  - close_packet：表示从对端收到 EOF，对端此时其实已经释放了，我们只需关心本地的 client，首先应检查当前 client 是否为 PENDING 状态，如果是，则清除 STATUS_NEED_SEND_TO_PEER 标记。如果不是 PENDING 状态，说明没有在队列中，可以安全的释放本地的 client。
  - error_packet：表示从对端收到错误，对端此时其实已经释放了，我们只需关心本地的 client，首先应检查当前 client 是否为 PENDING 状态，如果是，则清除 STATUS_NEED_SEND_TO_PEER 标记，同时如果当前的 status 是 CLOSE，则改为 ERROR，这样才会体现出 error 的效果，如果不是 PENDING 状态，那么直接 reset 连接即可。

- 当从本地 client 收到数据后，首先 stop read，然后根据情况进行如下判定：
  - 如果是 payload，则

特殊情况枚举：
- 首先明确一点，read 操作与 write 操作都是串行的。
- 读取到 payload 正在排队，write 回调发生错误，状态变更 `IN_TUNNEL_WRITE_QUEUE` => `IN_TUNNEL_WRITE_QUEUE | CLOSE_CONN_AFTER_QUEUE | RESET_CONN_AFTER_QUEUE | SEND_ERROR_AFTER_QUEUE`
- 读取到 EOF 正在排队，write 回调发生错误，状态变更 `IN_TUNNEL_WRITE_QUEUE | CLOSE_CONN_AFTER_QUEUE` => `IN_TUNNEL_WRITE_QUEUE | CLOSE_CONN_AFTER_QUEUE | RESET_CONN_AFTER_QUEUE`

read 随时可以 stop，stop 之后绝对不会再次调用我们的 read_cb，这是单线程以及 libuv 共同保证的。但是 write 请求不能取消，没有 stop 操作，必须等它执行完毕。因此：
- 当 write 中发生错误， 

服务端：
// TODO
