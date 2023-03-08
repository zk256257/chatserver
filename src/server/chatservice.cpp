 #include"chatservice.hpp"
  #include"public.hpp"
  #include<json.hpp>
  #include<map>
  #include<iostream>
#include<muduo/base/Logging.h>
using namespace muduo;
  using namespace std;
 ChatService* ChatService::instance(){
    static ChatService service;
    return &service;
  }//_msgHandlerMap
  ChatService::ChatService(){
_msgHandlerMap.insert({LOGIN_MSG,std::bind(&ChatService::login,this,_1,_2,_3)});
_msgHandlerMap.insert({REG_MSG,std::bind(&ChatService::reg,this,_1,_2,_3)});
_msgHandlerMap.insert({ONE_CHAT_MSG,std::bind(&ChatService::oneChat,this,_1,_2,_3)});
_msgHandlerMap.insert({ADD_FRIEND_MSG,std::bind(&ChatService::addFriend,this,_1,_2,_3)});

    // 群组业务管理相关事件处理回调注册
    _msgHandlerMap.insert({CREATE_GROUP_MSG, std::bind(&ChatService::createGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({ADD_GROUP_MSG, std::bind(&ChatService::addGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({GROUP_CHAT_MSG, std::bind(&ChatService::groupChat, this, _1, _2, _3)});

  }
//获取消息对应处理器
  MsgHandler  ChatService::getHandler(int msgid){
    //记录错误日志，msgid没有对应的事件处理回调
    auto it =_msgHandlerMap.find(msgid);
    if(it==_msgHandlerMap.end()){
     return [=](const TcpConnectionPtr&conn,json &js,Timestamp){
        LOG_ERROR<<"msgid"<<msgid<<"can not find handler";
     };
    }else{
    return _msgHandlerMap[msgid];
    }
   
    }

//处理客户端异常退出
    void ChatService::clientCloseException(const TcpConnectionPtr &conn){
       User user;
      {
      lock_guard<mutex>lock(_connMutex);
      for (auto it=_userConnMap.begin();it!=_userConnMap.end();it++){
        if(it->second==conn){
          //从map表删除连接信息
          user.setId(it->first);

          _userConnMap.erase(it);
          break;
        }
      }
      }
      if(user.getId()!=-1){
      user.setState("offline");
      _userModel.updateState(user);
      }

    }

      //处理登录业务
    void ChatService::login(const TcpConnectionPtr&conn,json &js,Timestamp){
        int id=js["id"].get<int>();//转成整型
        string pwd=js["password"];
        User user=_userModel.query(id);
        if(user.getId()==id&&user.getPwd()==pwd){
          if(user.getState()=="online"){
             json response;
       response["msgid"]=LOGIN_MSG_ACK;
     
        response["errno"]=2;//账号已经登录
        response["errmsg"]="该账户已经登录，请重新输入账号";
        conn->send(response.dump());
          }else{
            //登录成功在_userConnMap中记录用户连接信息
            {
              lock_guard<mutex>lock(_connMutex);
            _userConnMap.insert({id,conn});
            }
            
          //登陆成功 要更新用户状态信息 offline=>onliue
          user.setState("online");
          _userModel.updateState(user);
            json response;
       response["msgid"]=LOGIN_MSG_ACK;
       response["id"]=user.getId();
        response["errno"]=0;//效应成功用0表示
        response["name"]=user.getName();
        //查询用户是否有离线消息
        vector<string>vec=_offlineMsgModel.query(id);
        if(!vec.empty()){
          response["offlinemsg"]=vec;
          //把用户的离线消息从数据库中删除，防止下次登录还发
          _offlineMsgModel.remove(id);
        }
        //查询该用户的好友信息并返回
        vector<User>userVec=_friendModel.query(id);
        if(!userVec.empty()){
          vector<string>vec2;
          for(User &user:userVec){
            json js;
            js["id"]=user.getId();
            js["name"]=user.getName();
            js["state"]=user.getState();
            vec2.push_back(js.dump());
          }
          response["friends"]=vec2;
        }
        conn->send(response.dump());
        }
        }
        else{
          //登陆失败 用户不存在 或者用户名存在但密码错误
               json response;
        response["msgid"]=LOGIN_MSG_ACK;
        response["errno"]=1;//效应成功用0表示
     
        conn->send(response.dump());
        }
    }
    //处理注册服务
    void ChatService:: reg(const TcpConnectionPtr&conn,json &js,Timestamp){
      string name=js["name"];
      string pwd=js["password"];
      User user;
      user.setName(name);
      user.setPwd(pwd);
      bool state=_userModel.insert(user);
      if(state){
        //注册成功
       json response;
       response["msgid"]=REG_MSG_ACK;
       response["id"]=user.getId();
        response["errno"]=0;//效应成功用0表示
        conn->send(response.dump());
      }else{
        //注册失败
         json response;
       response["msgid"]=REG_MSG_ACK;
      
        response["errno"]=1;//效应成功用0表示
        conn->send(response.dump());
      }
    }

//一对一聊天服务
    void ChatService::oneChat(const TcpConnectionPtr&conn,json &js,Timestamp)
    {
      int toid=js["to"].get<int>();
      
      {
        lock_guard<mutex>lock(_connMutex);
        auto it=_userConnMap.find(toid);
          //用户在线 转发消息
        if (it!=_userConnMap.end()){
          it->second->send(js.dump());
          return;
        }
        //用户不在线 
        else{
          //存储离线消息
          _offlineMsgModel.insert(toid,js.dump());
        }
      }
    }
 //重置所有用户状态信息
    void ChatService::reset(){
      //把online状态的用户设置为offline
      _userModel.resetState();
    }


    // 添加好友业务 msgid id friendid
void ChatService::addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int friendid = js["friendid"].get<int>();

    // 存储好友信息
    _friendModel.insert(userid, friendid);
}

// 创建群组业务
void ChatService::createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    std::cout<<js;
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
void ChatService::addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    _groupModel.addGroup(userid, groupid, "normal");
}

// 群组聊天业务
void ChatService::groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
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
            it->second->send(js.dump());
        }
        else{
       
       
                // 存储离线群消息
                _offlineMsgModel.insert(id, js.dump());
            }
        }
}



