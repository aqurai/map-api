#ifndef DMAP_PEER_HANDLER_H_
#define DMAP_PEER_HANDLER_H_

#include <condition_variable>
#include <mutex>
#include <unordered_map>
#include <set>
#include <string>

#include "dmap/peer-id.h"

namespace dmap {
class Message;

/**
 * Allows to hold identifiers of peers and exposes common operations
 * on the peers, querying MapApiHub
 */
class PeerHandler {
 public:
  void add(const PeerId& peer);
  /**
   * Sends the message to all currently connected peers and collects their
   * responses
   */
  void broadcast(Message* request,
                 std::unordered_map<PeerId, Message>* responses);

  bool empty() const;
  // If empty, print "info" (if != "") to LOG(INFO), then block until empty.
  void awaitNonEmpty(const std::string& info) const;
  /**
   * Allows user to view peers, e.g. for ConnectResponse
   * TODO(simon) is this cheap? What else to fill ConnectResponse with
   * addresses?
   */
  const std::set<PeerId>& peers() const;

  void remove(const PeerId& peer);
  /**
   * Sends request to specified peer. If peer not among peers_, adds it.
   */
  void request(const PeerId& peer_address, Message* request, Message* response);
  /**
   * Sends request to specified peer. If peer not among peers_, adds it. Returns
   * false on timeout.
   */
  bool try_request(const PeerId& peer_address, Message* request,
                   Message* response);
  /**
   * Returns true if all peers have acknowledged, false otherwise.
   * TODO(tcies) timeouts?
   */
  bool undisputableBroadcast(Message* request);

  size_t size() const;

 private:
  std::set<PeerId> peers_;  // std::set to ensure uniform ordering
  mutable std::mutex mutex_;
  mutable std::condition_variable cv_;
};

} /* namespace dmap */

#endif  // DMAP_PEER_HANDLER_H_