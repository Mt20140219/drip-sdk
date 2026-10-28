# diag-sdk

面向车企开发/测试人员的 C++17 DoIP + UDS 诊断协议栈 SDK。同一套协议栈支持
ECU 侧（服务端）和 Tester 侧（客户端）两种角色。

## 当前状态

- M0：公共 API 头文件已定义（`include/diag/`）。
- M1：基础层（socket、字节缓冲、十六进制日志）已实现。
- M2：DoIP 传输层已跑通——车辆发现、路由激活、保活、诊断报文
  ACK/NACK 和诊断报文传输；ECU 支持可配置上限的多客户端并发连接。
- M3/M4（部分）：UDS 会话层与应用层核心服务已实现——10 会话控制、11 复位、
  14 清 DTC、19 读 DTC、22 读 DID、27 安全访问、28 通信控制、2E 写 DID、
  2F IO 控制、31 例程控制、34/36/37 下载流程、3E 保活、85 DTC 设置，
  包含会话/安全状态、NRC、DID/DTC 数据表和 response pending（0x78）。
- M5（进行中）：轻量自测套件已加入，覆盖 ByteBuffer、DoIP 编解码、UDS
  服务正负响应、安全访问锁定、response pending 和多线程并发访问。
- 尚未实现：完整服务集（14/28/2E/2F/31/34/35/36/37/85 等）、功能寻址、
  多客户端并发细节、符合性测试向量。

## 构建

```sh
cmake -S . -B build
cmake --build build
```

## 运行测试

```sh
cmake -S . -B build -DDIAG_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

## CI

GitHub Actions 配置位于 `.github/workflows/ci.yml`，会在 gcc 和 clang 两个
编译器下分别执行：配置 → 构建 → 单元测试（ctest）→ DoIP 端到端冒烟测试。

## 运行示例

先启动模拟 ECU，再运行 tester：

```sh
./build/examples/ecu_sim &
./build/examples/tester
```

tester 会依次执行：UDP 车辆发现 → TCP 连接 → 路由激活 → 切换扩展会话 →
读 VIN → 读 DTC → 安全访问（含错误 key 和正确 key）→ response pending →
TesterPresent → ECU 复位。

## 目录

```text
include/diag/    公共 API 头文件
src/             协议栈实现（net、doip、uds、node）
examples/        参考实现：ecu_sim（ECU 侧）、tester（Tester 侧）
```
