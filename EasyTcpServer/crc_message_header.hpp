#ifndef _CRC_MESSAGE_HEADER_HPP_
#define _CRC_MESSAGE_HEADER_HPP_

enum CMD
{
	CMD_LOGIN,
	CMD_LOGIN_RESPONSE,
	CMD_LOGOUT,
	CMD_LOGOUT_RESPONSE,
	CMD_NEW_USER_JOIN,
	CMD_HEART_C2S,
	CMD_HEART_S2C,
	CMD_UNKNOWN
};

// msg header
struct CRCDataHeader
{
	short data_length;
	short cmd;
};

struct UnknownResponse : public CRCDataHeader
{
	UnknownResponse()
	{
		data_length = sizeof(UnknownResponse);
		cmd = CMD_UNKNOWN;
		strcpy(msg, "error command.");
	}
	char msg[32];
};

struct Login : public CRCDataHeader
{
public:
	Login()
	{
		data_length = sizeof(Login);
		cmd = CMD_LOGIN;
	}

	char username[32];
	char password[32];
	char data[100 - 68];
};

struct LoginResponse : public CRCDataHeader
{
	LoginResponse()
	{
		data_length = sizeof(LoginResponse);
		cmd = CMD_LOGIN_RESPONSE;
		result = 1;
	}
	int result;
	char data[100 - 8];
};

struct Logout : public CRCDataHeader
{
public:
	Logout()
	{
		data_length = sizeof(Logout);
		cmd = CMD_LOGOUT;
	}

	char username[32];
};

struct LogoutResponse : public CRCDataHeader
{
	LogoutResponse()
	{
		data_length = sizeof(LogoutResponse);
		cmd = CMD_LOGOUT_RESPONSE;
		result = 1;
	}
	int result;
};

struct NewUserJoin : public CRCDataHeader
{
	NewUserJoin()
	{
		data_length = sizeof(NewUserJoin);
		cmd = CMD_NEW_USER_JOIN;
		sock = 0;
	}
	int sock;
};

struct HeartS2C : public CRCDataHeader
{
	HeartS2C()
	{
		data_length = sizeof(HeartS2C);
		cmd = CMD_HEART_S2C;
	}
};

struct HeartC2S : public CRCDataHeader
{
	HeartC2S()
	{
		data_length = sizeof(HeartC2S);
		cmd = CMD_HEART_C2S;
	}
};

#endif
