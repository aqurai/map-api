#ifndef MAP_API_RPC_COMMAND_REGISTERER_H_
#define MAP_API_RPC_COMMAND_REGISTERER_H_

#include <string>

#include <multiagent-mapping-common/command-registerer.h>


namespace map_api {

class Message;
class PeerId;

class RpcCommandRegisterer : public common::CommandRegisterer {
 public:
  RpcCommandRegisterer();

  void handleStatusRpc(const Message& request, Message* response);

  static int commandRpc(const std::string& command, const PeerId& peer);

  static const char kCommandRequest[];
  static const char kCommandResponse[];
};
}  // namespace map_api
#endif  // MAP_API_RPC_COMMAND_REGISTERER_H_