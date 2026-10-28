#include "diag/node_ecu.h"

#include <utility>

#include "doip/entity_server.h"

namespace diag {

class EcuNode::Impl {
 public:
  explicit Impl(NodeConfig cfg) {
    doip::EntityConfig c;
    c.bind_address = cfg.bind_address;
    c.tcp_port = cfg.tcp_port;
    c.udp_port = cfg.udp_port;
    c.logical_address = cfg.logical_address;
    c.functional_address = cfg.functional_address;
    c.max_connections = cfg.max_connections;
    c.vehicle = cfg.vehicle;
    c.version = cfg.doip_version;
    c.inactivity = cfg.timing.inactivity;
    if (c.vehicle.logical_address == 0) {
      c.vehicle.logical_address = cfg.logical_address;
    }
    server_ = std::make_unique<doip::EntityServer>(std::move(c));
  }

  std::unique_ptr<doip::EntityServer> server_;
};

EcuNode::EcuNode(NodeConfig config)
    : impl_(std::make_unique<Impl>(std::move(config))) {}

EcuNode::~EcuNode() = default;

Error EcuNode::start() {
  return impl_->server_->Start() ? Error::kOk : Error::kInternal;
}

void EcuNode::stop() { impl_->server_->Stop(); }

Error EcuNode::registerHandler(
    uint8_t sid, std::shared_ptr<uds::ServiceHandler> handler) {
  impl_->server_->RegisterHandler(sid, std::move(handler));
  return Error::kOk;
}

void EcuNode::unregisterHandler(uint8_t sid) {
  impl_->server_->UnregisterHandler(sid);
}

Error EcuNode::setDidValue(uint16_t did, std::vector<uint8_t> value) {
  impl_->server_->SetDidValue(did, std::move(value));
  return Error::kOk;
}

Error EcuNode::setDtc(uint32_t dtc, uint8_t status) {
  impl_->server_->SetDtc(dtc, status);
  return Error::kOk;
}

void EcuNode::clearDtcs() { impl_->server_->ClearDtcs(); }

void EcuNode::setSink(std::shared_ptr<EventSink> sink) {
  impl_->server_->SetSink(std::move(sink));
}

}  // namespace diag
