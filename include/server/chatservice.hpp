#ifndef CHATSERVICE_H
#define CHATSERVICE_H
#include<muduo/net/TcpConnection.h>
#include<unordered_map>
#include<functional>
#include "json.hpp"
#include"usermodel.hpp"
#include"offlinemessagemodel.hpp"
#include"friendmodel.hpp"
#include"groupmodel.hpp"
#include<mutex>
using namespace std;
using namespace muduo;
using namespace muduo::net;
using json=nlohmann::json;
//处理消息的事件回调方法类型
using MsgHandler=std::function<void(const TcpConnectionPtr&conn,json &js,Timestamp)>;



//聊天服务器业务类
class ChatService{

public:
    //获取单例对象的接口函数
    static ChatService* instance();
    //处理客户端异常退出
    void clientCloseException(const TcpConnectionPtr &);
    //服务器异常，业务充值
    void reset();
    //处理登录业务
    void login(const TcpConnectionPtr&conn,json &js,Timestamp);
    //处理注册服务
    void reg(const TcpConnectionPtr&conn,json &js,Timestamp);
    //一对一聊天服务
    void oneChat(const TcpConnectionPtr&conn,json &js,Timestamp);
    //添加好友服务
    void addFriend(const TcpConnectionPtr&conn,json &js,Timestamp);
    // 创建群组业务
    void createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 加入群组业务
    void addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 群组聊天业务
      void groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time);
    //获取消息对应的处理器
    MsgHandler getHandler(int msgid);
private:
    ChatService();
    //存储消息id和其对应的业务处理方法
    unordered_map<int,MsgHandler>_msgHandlerMap;
  
    //存储本机在线id账号的通信连接TcpConnection
    unordered_map<int,TcpConnectionPtr>_userConnMap;
    //定义互斥锁保持_userCOnnMap的线程安全
    mutex _connMutex;

      UserModel _userModel;//封装了处理user的方法 数据操作类对象
    OfflineMsgModel _offlineMsgModel;//封装了处理离线消息的方法
    FriendModel _friendModel;//封装了处理添加好友，在线显示好友信息的方法
    GroupModel _groupModel;

};




#endif