#ifndef _MESSAGEHEADER_HPP_
#define _MESSAGEHEADER_HPP_

enum CMD
{
	CMD_LOGIN,
	CMD_LOGIN_RESPONSE,
	CMD_LOGOUT,
	CMD_LOGOUT_RESPONSE,
	CMD_NEW_USER_JOIN,
	CMD_UNKNOWN
};

// msg header
struct DataHeader
{
	short data_length;
	short cmd;
};

struct UnknownResponse : public DataHeader
{
	UnknownResponse()
	{
	}
	char msg[32];
};

struct Login : public DataHeader
{
public:
	Login()
	{
		data_length = sizeof(Login);
		cmd = CMD_LOGIN;
	}

	char username[32];
	char password[32];
};

struct LoginResponse : public DataHeader
{
	LoginResponse()
	{
	}
	int result;
};

struct Logout : public DataHeader
{
public:
	Logout()
	{
		data_length = sizeof(Logout);
		cmd = CMD_LOGOUT;
	}

	char username[32];
};

struct LogoutResponse : public DataHeader
{
	LogoutResponse()
	{
	}
	int result;
};

struct NewUserJoin : public DataHeader
{
	NewUserJoin()
	{
		data_length = sizeof(NewUserJoin);
		cmd = CMD_NEW_USER_JOIN;
		sock = 0;
	}
	int sock;
};


#endif
