#include "chatservice.h"
#include "public.h"
#include <vector>
using namespace std;

ChatService::ChatService()
{
    msghandlermap_.insert({LOGIN_MSG,std::bind(&ChatService::login,this,placeholders::_1,placeholders::_2)});
    msghandlermap_.insert({LOGINOUT_MSG,std::bind(&ChatService::loginout,this,placeholders::_1,placeholders::_2)});
    msghandlermap_.insert({REG_MSG,std::bind(&ChatService::reg,this,placeholders::_1,placeholders::_2)});
    msghandlermap_.insert({ONE_CHAT_MSG,std::bind(&ChatService::oneChat,this,placeholders::_1,placeholders::_2)});
    msghandlermap_.insert({ADD_FRIEND_MSG,std::bind(&ChatService::addFriend,this,placeholders::_1,placeholders::_2)});
    msghandlermap_.insert({CREATE_GROUP_MSG,std::bind(&ChatService::createGroup,this,placeholders::_1,placeholders::_2)});
    msghandlermap_.insert({ADD_GROUP_MSG,std::bind(&ChatService::addGroup,this,placeholders::_1,placeholders::_2)});
    msghandlermap_.insert({GROUP_CHAT_MSG,std::bind(&ChatService::groupChat,this,placeholders::_1,placeholders::_2)});
    
    // 连接redis服务器
    if (_redis.connect())
    {
        // 设置上报消息的回调
        _redis.init_notify_handler(std::bind(&ChatService::handleRedisSubscribeMessage,this,placeholders::_1,placeholders::_2));
    }
}
ChatService *ChatService::instance()
{
    static ChatService chatservice;
    return &chatservice;
}

void ChatService::reset()
{
    // 把online状态的用户，设置成offline
    _userModel.resetState();
}

MsgHandler ChatService::GetHandler(int msg)
{
    auto it = msghandlermap_.find(msg);
    if(it == msghandlermap_.end())
    {
        return [=](spConnection conn,json& js){
            cout << "msgid:" << msg << ",can not find handler" << endl;
            // 这里不应该输出屏幕，应该写入日志
        };
    }
    else 
    {
        return msghandlermap_[msg]; 
    }
}
void ChatService::login(const spConnection &conn, json &js)
{
    int id = js["id"].get<int>();
    string pwd = js["password"];

    User user = _userModel.query(id);
    if (user.getId() == id && user.getPwd() == pwd)
    {
        if (user.getState() == "online")
        {
            // 该用户已经登录，不允许重复登录
            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 2;
            response["errmsg"] = "该账号已经登录，请重新输入新账号";
            conn->send(response.dump().c_str(), response.size());
        }
        else
        {
            //登录成功,记录用户连接信息
            {
                lock_guard<mutex> lock(_connMutex);
                _userConnMap.insert({id, conn});
            }

            // id用户登录成功后，向redis订阅channel（id）
            _redis.subscribe(id);

            // 登录成功，更新用户状态信息 state offline=》online
            user.setState("online");
            _userModel.updateState(user);

            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 0;
            response["id"] = user.getId();
            response["name"] = user.getName();
            // 查询用户是否有离线消息
            vector<string> vec = _offlineMsgModel.query(id);
            if(!vec.empty())
            {
                response["offlinemsg"] = vec;
                //读取该用户的离线消息后，把该用户的所有离线消息删除掉
                _offlineMsgModel.remove(id);
            }
            // 查询该用户的好友信息并返回
            vector<User> userVec = _friendModel.query(id);
            if(!userVec.empty())
            {
                vector<string> vec2;
                for(User &user : userVec)
                {
                    json js;
                    js["id"] = user.getId();
                    js["name"] = user.getName();
                    js["state"] = user.getState();
                    vec2.push_back(js.dump());
                }
                response["friends"] = vec2;
            }
            
            // 查询用户的群组信息
            vector<Group> groupuserVec = _groupModel.queryGroups(id);
            if (!groupuserVec.empty())
            {
                // group:[{groupid:[xxx, xxx, xxx, xxx]}]
                vector<string> groupV;
                for (Group &group : groupuserVec)
                {
                    json grpjson;
                    grpjson["id"] = group.getId();
                    grpjson["groupname"] = group.getName();
                    grpjson["groupdesc"] = group.getDesc();
                    vector<string> userV;
                    for (GroupUser &user : group.getUsers())
                    {
                        json js;
                        js["id"] = user.getId();
                        js["name"] = user.getName();
                        js["state"] = user.getState();
                        js["role"] = user.getRole();
                        userV.push_back(js.dump());
                    }
                    grpjson["users"] = userV;
                    groupV.push_back(grpjson.dump());
                }

                response["groups"] = groupV;
            }
            conn->send(response.dump().c_str(), response.size());
        }
    }
    else
    {
        // 该用户不存在，或者用户存在但是密码错误，登录失败
        json response;
        response["msgid"] = LOGIN_MSG_ACK;
        response["errno"] = 1;
        response["errmsg"] = "用户名或者密码错误";
        conn->send(response.dump().c_str(),response.size());
    }
}



void ChatService::reg(const spConnection &conn, json &js)
{
    // printf("reg start\n");
    string name = js["name"];
    string pwd = js["password"];

    User user;
    user.setName(name);
    user.setPwd(pwd);
    // 想要输出到终端，要么要有回车比如/n或者endl，要么要有
    // printf("calling usermodel.insert");
    // fflush(stdout);
    bool state = _userModel.insert(user);
    if(state)
    {
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 0;
        response["id"] = user.getId();
        conn->send(response.dump().c_str(),response.size());
    }
    else
    {
       json response;
       response["msgid"] = REG_MSG_ACK;
       response["errno"] = 1;
       conn->send(response.dump().c_str(),response.size()); 
    }
}

//处理注销业务
void ChatService::loginout(const spConnection &conn, json &js)
{
    int userid = js["id"].get<int>();

    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(userid);
        if(it != _userConnMap.end())
        {
            _userConnMap.erase(it);
        }
    }

    // 用户注销，相当于就是下线，在redis中取消订阅通道
    _redis.unsubscribe(userid);

    // 更新用户的状态信息
    User user(userid, "", "offline");
    _userModel.updateState(user);
}

// 处理客户端异常退出
void ChatService::clientCloseException(const spConnection &conn)
{
    User user;
    {
        lock_guard<mutex> lock(_connMutex);
        for (auto it = _userConnMap.begin(); it != _userConnMap.end(); ++it)
        {
            if (it->second == conn)
            {
                // 从map表删除用户的连接信息
                user.setId(it->first);
                _userConnMap.erase(it);
                break;
            }
        }
    }

    // 用户注销，相当于就是下线，在redis中取消订阅通道
    _redis.unsubscribe(user.getId());
     
    // 更新用户的状态信息
    if(user.getId() != -1)
    {
        user.setState("offline");
        _userModel.updateState(user);
    }
}

// 一对一聊天业务
void ChatService::oneChat(const spConnection &conn, json &js)
{
    int toid = js["toid"].get<int>();

    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(toid);
        if (it != _userConnMap.end())
        {
            // toid在线，转发消息   服务器主动推送消息给toid用户
            it->second->send(js.dump().c_str(), js.size());
            return;
        }
    }

    // 查询toid是否在线 
    User user = _userModel.query(toid);
    if (user.getState() == "online")
    {
        _redis.publish(toid, js.dump());
        return;
    }

    // toid不在线，存储离线消息
    _offlineMsgModel.insert(toid, js.dump());
}

// 添加好友业务
void ChatService::addFriend(const spConnection &conn, json &js)
{
    int userid = js["id"].get<int>();
    int friendid = js["friendid"].get<int>();

    // 存储好友信息
    _friendModel.insert(userid,friendid);
}

// 创建群组业务
void ChatService::createGroup(const spConnection &conn, json &js)
{
    int userid = js["id"].get<int>();
    string name = js["groupname"];
    string desc = js["groupdesc"];

    // 存储新创建的群组信息
    Group group(-1, name, desc);
    if (_groupModel.createGroup(group))
    {
        // 存储群组创建人信息
        _groupModel.addGroup(userid, group.getId(), "creator");
    }
}

// 加入群组业务
void ChatService::addGroup(const spConnection &conn, json &js)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    _groupModel.addGroup(userid, groupid, "normal");
}

// 群组聊天业务
void ChatService::groupChat(const spConnection &conn, json &js)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    vector<int> useridVec = _groupModel.queryGroupUsers(userid, groupid);

    lock_guard<mutex> lock(_connMutex);
    for (int id : useridVec)
    {
        auto it = _userConnMap.find(id);
        if (it != _userConnMap.end())
        {
            // 转发群消息
            it->second->send(js.dump().c_str(), js.size());
        }
        else
        {
            // 查询toid是否在线 
            User user = _userModel.query(id);
            if (user.getState() == "online")
            {
                _redis.publish(id, js.dump());
            }
            else
            {
                // 存储离线群消息
                _offlineMsgModel.insert(id, js.dump());
            }
        }
    }
}

// 从redis消息队列中获取订阅的消息
void ChatService::handleRedisSubscribeMessage(int userid, string msg)
{
    lock_guard<mutex> lock(_connMutex);
    auto it = _userConnMap.find(userid);
    if (it != _userConnMap.end())
    {
        it->second->send(msg.c_str(), msg.size());
        return;
    }

    // 存储该用户的离线消息
    _offlineMsgModel.insert(userid, msg);
}