#include "diag/node_tester.h"

#include <utility>

#include "doip/tester_client.h"

namespace diag {

class TesterNode::Impl {
 public:
  explicit Impl(NodeConfig cfg) {
    doip::TesterConfig c;
    c.bind_address = cfg.bind_address;
    c.tcp_port = cfg.tcp_port;
    c.udp_port = cfg.udp_port;
    c.logical_address = cfg.logical_address;
    c.functional_address = cfg.functional_address;
    c.version = cfg.doip_version;
    c.timing = cfg.timing;
    client_ = std::make_unique<doip::TesterClient>(std::move(c));
  }

  std::unique_ptr<doip::TesterClient> client_;
};

TesterNode::TesterNode(NodeConfig config)
    : impl_(std::make_unique<Impl>(std::move(config))) {}

TesterNode::~TesterNode() = default;

Error TesterNode::start() { return impl_->client_->Start(); }

void TesterNode::stop() { impl_->client_->Stop(); }

Error TesterNode::discover(std::vector<VehicleInfo>& out,
                           std::chrono::milliseconds timeout) {
  return impl_->client_->Discover(out, timeout);
}

Error TesterNode::connect(std::string_view address, LogicalAddress target) {
  return impl_->client_->Connect(std::string(address), target);
}

Error TesterNode::routingActivate(doip::ActivationType type) {
  return impl_->client_->RoutingActivate(type);
}

Error TesterNode::disconnect() { return impl_->client_->Disconnect(); }

Error TesterNode::send(const uds::Request& req, uds::Response& resp) {
  return impl_->client_->Send(req, resp);
}

Error TesterNode::sendFunctional(const uds::Request& req,
                                 uds::Response& resp) {
  return impl_->client_->SendFunctional(req, resp);
}

void TesterNode::setSink(std::shared_ptr<EventSink> sink) {
  impl_->client_->SetSink(std::move(sink));
}

}  // namespace diag
