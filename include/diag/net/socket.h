#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <string>

namespace diag::net {

class TcpServer {
 public:
  TcpServer() = default;
  ~TcpServer();

  TcpServer(const TcpServer&) = delete;
  TcpServer& operator=(const TcpServer&) = delete;

  bool Listen(const std::string& address, uint16_t port);
  int Accept();  // Returns a connected client fd, or -1 on error.
  void Close();

  int fd() const noexcept { return fd_; }
  uint16_t port() const noexcept { return port_; }

 private:
  int fd_ = -1;
  uint16_t port_ = 0;
};

class TcpClient {
 public:
  TcpClient() = default;
  ~TcpClient();

  TcpClient(const TcpClient&) = delete;
  TcpClient& operator=(const TcpClient&) = delete;

  bool Connect(const std::string& address, uint16_t port, int timeout_ms = 5000);
  void Close();

  int fd() const noexcept { return fd_; }

 private:
  int fd_ = -1;
};

std::ptrdiff_t SendAll(int fd, const uint8_t* data, std::size_t size);
std::ptrdiff_t RecvAll(int fd, uint8_t* data, std::size_t size);
bool SetRecvTimeout(int fd, std::chrono::milliseconds timeout);

class UdpSocket {
 public:
  UdpSocket() = default;
  ~UdpSocket();

  UdpSocket(const UdpSocket&) = delete;
  UdpSocket& operator=(const UdpSocket&) = delete;

  bool Bind(const std::string& address, uint16_t port);
  bool SetBroadcast(bool enabled);
  bool SetRecvTimeout(std::chrono::milliseconds timeout);

  // Returns bytes sent, or -1 on error.
  std::ptrdiff_t SendTo(const std::string& address, uint16_t port,
                        const uint8_t* data, std::size_t size);

  // Returns bytes received, 0 on timeout, or -1 on error.
  std::ptrdiff_t RecvFrom(uint8_t* data, std::size_t size,
                          std::string& from_addr, uint16_t& from_port);

  void Close();
  int fd() const noexcept { return fd_; }

 private:
  int fd_ = -1;
};

}  // namespace diag::net
