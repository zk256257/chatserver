# chatserver
基于muduo库实现的聊天服务器和客户端源码，实现了注册登录添加好友添加群组、单聊、群聊等业务，并使用mysql进行数据的存储。

编译方式
cd build
rm -rf *
cmake ..
make

需要启动mysql，以及安装muduo库
对应mysql建表指令
create table User(
id int primary key auto_increment,
name varchar(50)   not null unique ,
password varchar(50) not null,
 state enum('online','offline') default 'offline'
);

create table friend(
userid int not null,
friendid int not null,
primary key(userid,friendid) 
);

create table allgroup(
id int primary key auto_increment,
groupname varchar(50) not null unique,
groupdesc varchar(200) default ''
);

create table groupuser(
groupid int not null,
userid int not null,
grouprole enum('creator','normal') default 'normal',
primary key(groupid,userid)
);

create table offlinemessage(
userid int not null,
message varchar(500) not null
);

