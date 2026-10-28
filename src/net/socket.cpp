#include "diag/net/socket.h"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

namespace diag::net {

namespace {

int SetNonBlocking(int fd, bool enabled) {
  int flags = fcntl(fd, F_GETFL, 0);
  if (flags < 0) return -1;
  flags = enabled ? (flags | O_NONBLOCK) : (flags & ~O_NONBLOCK);
  return fcntl(fd, F_SETFL, flags);
}

}  // namespace

TcpServer::~TcpServer() { Close(); }

bool TcpServer::Listen(const std::string& address, uint16_t port) {
  Close();

  int s = socket(AF_INET, SOCK_STREAM, 0);
  if (s < 0) return false;

  SetNonBlocking(s, true);

  int one = 1;
  setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  if (address.empty() || address == "0.0.0.0") {
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
  } else if (inet_pton(AF_INET, address.c_str(), &addr.sin_addr) != 1) {
    ::close(s);
    return false;
  }

  if (bind(s, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
    ::close(s);
    return false;
  }
  if (listen(s, 16) != 0) {
    ::close(s);
    return false;
  }

  sockaddr_in bound{};
  socklen_t bound_len = sizeof(bound);
  if (getsockname(s, reinterpret_cast<sockaddr*>(&bound), &bound_len) == 0) {
    port_ = ntohs(bound.sin_port);
  } else {
    port_ = port;
  }

  fd_ = s;
  return true;
}

int TcpServer::Accept() {
  if (fd_ < 0) return -1;

  pollfd pfd{fd_, POLLIN, 0};
  int rc = poll(&pfd, 1, 250);
  if (rc <= 0) {
    if (rc == 0) errno = EAGAIN;
    return -1;
  }

  int client = accept(fd_, nullptr, nullptr);
  if (client < 0) return -1;
  SetNonBlocking(client, false);
  return client;
}

void TcpServer::Close() {
  if (fd_ >= 0) {
    ::close(fd_);
    fd_ = -1;
  }
}

TcpClient::~TcpClient() { Close(); }

bool TcpClient::Connect(const std::string& address, uint16_t port,
                        int timeout_ms) {
  Close();

  int s = socket(AF_INET, SOCK_STREAM, 0);
  if (s < 0) return false;

  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  if (inet_pton(AF_INET, address.c_str(), &addr.sin_addr) != 1) {
    ::close(s);
    return false;
  }

  SetNonBlocking(s, true);

  int rc = ::connect(s, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
  if (rc == 0) {
    SetNonBlocking(s, false);
    fd_ = s;
    return true;
  }
  if (errno != EINPROGRESS) {
    ::close(s);
    return false;
  }

  fd_set wfds;
  FD_ZERO(&wfds);
  FD_SET(s, &wfds);
  timeval tv{timeout_ms / 1000, (timeout_ms % 1000) * 1000};
  rc = select(s + 1, nullptr, &wfds, nullptr, &tv);
  if (rc <= 0) {
    ::close(s);
    return false;
  }

  int so_error = 0;
  socklen_t so_error_len = sizeof(so_error);
  if (getsockopt(s, SOL_SOCKET, SO_ERROR, &so_error, &so_error_len) != 0 ||
      so_error != 0) {
    ::close(s);
    return false;
  }

  SetNonBlocking(s, false);
  fd_ = s;
  return true;
}

void TcpClient::Close() {
  if (fd_ >= 0) {
    ::close(fd_);
    fd_ = -1;
  }
}

std::ptrdiff_t SendAll(int fd, const uint8_t* data, std::size_t size) {
  std::size_t sent = 0;
  while (sent < size) {
    ssize_t n = ::send(fd, data + sent, size - sent, MSG_NOSIGNAL);
    if (n < 0) {
      if (errno == EINTR) continue;
      return -1;
    }
    if (n == 0) break;
    sent += static_cast<std::size_t>(n);
  }
  return static_cast<std::ptrdiff_t>(sent);
}

std::ptrdiff_t RecvAll(int fd, uint8_t* data, std::size_t size) {
  std::size_t got = 0;
  while (got < size) {
    ssize_t n = ::recv(fd, data + got, size - got, 0);
    if (n < 0) {
      if (errno == EINTR) continue;
      return -1;
    }
    if (n == 0) break;  // Peer closed the connection.
    got += static_cast<std::size_t>(n);
  }
  return static_cast<std::ptrdiff_t>(got);
}

bool SetRecvTimeout(int fd, std::chrono::milliseconds timeout) {
  timeval tv{timeout.count() / 1000, (timeout.count() % 1000) * 1000};
  return setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) == 0;
}

UdpSocket::~UdpSocket() { Close(); }

bool UdpSocket::Bind(const std::string& address, uint16_t port) {
  Close();

  int s = socket(AF_INET, SOCK_DGRAM, 0);
  if (s < 0) return false;

  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  if (address.empty() || address == "0.0.0.0") {
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
  } else if (inet_pton(AF_INET, address.c_str(), &addr.sin_addr) != 1) {
    ::close(s);
    return false;
  }

  if (bind(s, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
    ::close(s);
    return false;
  }

  fd_ = s;
  return true;
}

bool UdpSocket::SetBroadcast(bool enabled) {
  if (fd_ < 0) return false;
  int one = enabled ? 1 : 0;
  return setsockopt(fd_, SOL_SOCKET, SO_BROADCAST, &one, sizeof(one)) == 0;
}

bool UdpSocket::SetRecvTimeout(std::chrono::milliseconds timeout) {
  if (fd_ < 0) return false;
  return diag::net::SetRecvTimeout(fd_, timeout);
}

std::ptrdiff_t UdpSocket::SendTo(const std::string& address, uint16_t port,
                                 const uint8_t* data, std::size_t size) {
  if (fd_ < 0) return -1;

  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  if (inet_pton(AF_INET, address.c_str(), &addr.sin_addr) != 1) return -1;

  ssize_t n = sendto(fd_, data, size, 0,
                     reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
  return static_cast<std::ptrdiff_t>(n);
}

std::ptrdiff_t UdpSocket::RecvFrom(uint8_t* data, std::size_t size,
                                   std::string& from_addr,
                                   uint16_t& from_port) {
  if (fd_ < 0) return -1;

  sockaddr_in src{};
  socklen_t src_len = sizeof(src);
  ssize_t n = recvfrom(fd_, data, size, 0,
                       reinterpret_cast<sockaddr*>(&src), &src_len);
  if (n < 0) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) return 0;
    return -1;
  }

  char buf[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &src.sin_addr, buf, sizeof(buf));
  from_addr = buf;
  from_port = ntohs(src.sin_port);
  return static_cast<std::ptrdiff_t>(n);
}

void UdpSocket::Close() {
  if (fd_ >= 0) {
    ::close(fd_);
    fd_ = -1;
  }
}

}  // namespace diag::net
