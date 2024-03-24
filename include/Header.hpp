#ifndef HEADER_HPP
#define HEADER_HPP


#include<string>

const int g_strLen = 30;
//头部信息
enum class CMD
{
    CMD_LOGIN,
    CMD_LOGOUT,
    CMD_LOGIN_RESULT,
    CMD_LOGOUT_RESULT,
    CMD_QUIT,
    CMD_NEWJOIN,
    CMD_ERROR // 
};


//数据头
typedef struct DataHeader
{
    CMD cmd_;
    int length_;

} DataHeader;

//登录命令
typedef struct LOGIN : public DataHeader
{
    char name_[g_strLen] = {0};
    char passWord_[g_strLen] = {0};
} LOGIN;

//登出命令
typedef struct LOGOUT : public DataHeader
{
    char name_[g_strLen] = {0};
} LOGOUT;


//登录返回结果
typedef struct LOGIN_RESULT : public DataHeader
{
    bool result_ = false;
} LOGIN_RESULT;

//登出返回结果
typedef struct LOGOUT_RESULT : public DataHeader
{
    bool result_ = false;
} LOGOUT_RESULT;

//新加入用户
typedef struct NEW_JOIN:DataHeader
{
    char newUserAddr_[g_strLen] = {0};
    unsigned short newUserPort = 0;
    int newUserSock_ = -1;
}NEW_JOIN;
#endif //HEADER_HPP