#ifndef CHATSERVICE_H
#define CHATSERVICE_H

#include <muduoC11/TcpServer.h>
#include <unordered_map>
#include <functional>
#include "json.hpp"
#include "usermodel.h"
#include "offlinemessagemodel.h"
#include "friendmodel.h"
#include "groupmodel.h"
#include <mutex>
#include "redis.hpp"

using namespace std;
using json = nlohmann::json;
using MsgHandler = function<void(spConnection conn,json& js)>;

class ChatService
{
private:
    unordered_map<int,MsgHandler> msghandlermap_;
    unordered_map<int, spConnection> _userConnMap;
    mutex _connMutex;
    UserModel _userModel;
    OfflineMsgModel _offlineMsgModel;
    FriendModel _friendModel;
    GroupModel _groupModel;
    Redis _redis;
    ChatService();
public:
    static ChatService* instance();
    void login(const spConnection &conn,json& js);
    void reg(const spConnection &conn,json& js);
    void oneChat(const spConnection &conn,json& js);
    void addFriend(const spConnection &conn,json& js);
    // 创建群组业务
    void createGroup(const spConnection &conn,json& js);
    // 加入群组业务
    void addGroup(const spConnection &conn,json& js);
    // 群组聊天业务
    void groupChat(const spConnection &conn,json& js);
    void loginout(const spConnection &conn,json& js);
    MsgHandler GetHandler(int msg);
    void clientCloseException(const spConnection &conn);
    void handleRedisSubscribeMessage(int userid, string msg);
    void reset();
};
#endif