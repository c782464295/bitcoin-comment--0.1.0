
#include "headers.h"



// 一字节方式对齐
#pragma pack(1)
// ircaddr结构体
struct ircaddr
{
    int ip;// ip
    short port;// 端口
};

string EncodeAddress(const CAddress& addr)
{
    struct ircaddr tmp;
    tmp.ip    = addr.ip;
    tmp.port  = addr.port;
    // 对本机ip地址及端口号进行base58编码，随后在得到的结果前加上字符”u”，得到本机IRC nickname
    vector<unsigned char> vch(UBEGIN(tmp), UEND(tmp));
    return string("u") + EncodeBase58Check(vch);
}

bool DecodeAddress(string str, CAddress& addr)
{
    vector<unsigned char> vch;
    if (!DecodeBase58Check(str.substr(1), vch))
        return false;

    struct ircaddr tmp;
    if (vch.size() != sizeof(tmp))
        return false;
    memcpy(&tmp, &vch[0], sizeof(tmp));

    addr  = CAddress(tmp.ip, tmp.port);
    return true;
}





// 发送（套接字，发送的内容指针）
static bool Send(SOCKET hSocket, const char* pszSend)
{
    if (strstr(pszSend, "PONG") != pszSend)
        printf("SENDING: %s\n", pszSend);
    const char* psz = pszSend;
    const char* pszEnd = psz + strlen(psz);
    while (psz < pszEnd)
    {
        int ret = send(hSocket, psz, pszEnd - psz, 0);
        if (ret < 0)
            return false;
        psz += ret;
    }
    return true;
}

bool RecvLine(SOCKET hSocket, string& strLine)
{
    strLine = "";
    loop
    {
        char c;
        int nBytes = recv(hSocket, &c, 1, 0);
        if (nBytes > 0)
        {
            if (c == '\n')
                continue;
            if (c == '\r')
                return true;
            strLine += c;
        }
        else if (nBytes <= 0)
        {
            if (!strLine.empty())
                return true;
            // socket closed
            printf("IRC socket closed\n");
            return false;
        }
        else
        {
            // socket error
            int nErr = WSAGetLastError();
            if (nErr != WSAEMSGSIZE && nErr != WSAEINTR && nErr != WSAEINPROGRESS)
            {
                printf("IRC recv failed: %d\n", nErr);
                return false;
            }
        }
    }
}

bool RecvLineIRC(SOCKET hSocket, string& strLine)
{
    loop
    {
        bool fRet = RecvLine(hSocket, strLine);
        if (fRet)
        {
            if (fShutdown)
                return false;
            vector<string> vWords;
            ParseString(strLine, ' ', vWords);
            if (vWords[0] == "PING")
            {
                strLine[1] = 'O';
                strLine += '\r';
                Send(hSocket, strLine.c_str());
                continue;
            }
        }
        return fRet;
    }
}

bool RecvUntil(SOCKET hSocket, const char* psz1, const char* psz2=NULL, const char* psz3=NULL)
{
    loop
    {
        string strLine;
        if (!RecvLineIRC(hSocket, strLine))
            return false;
        printf("IRC %s\n", strLine.c_str());
        if (psz1 && strLine.find(psz1) != -1)
            return true;
        if (psz2 && strLine.find(psz2) != -1)
            return true;
        if (psz3 && strLine.find(psz3) != -1)
            return true;
    }
}



// 重启IRCSeed服务标识
bool fRestartIRCSeed = false;
// IRC线程
void ThreadIRCSeed(void* parg)
{
    // 无线循环直到退出
    loop
    {
        // 虽然说p2p网络没有服务器,但是必须存在知名节点,否则无从启动网络.
        // 在bitcoin的后续版本，初始节点被硬编码到了源码中
        // 在这个版本，使用的是IRC来获取
        // 连接IRC服务器，服务器名称为chat.freenode.net
        struct hostent* phostent = gethostbyname("chat.freenode.net");
        CAddress addrConnect(*(u_long*)phostent->h_addr_list[0], htons(6667));

        SOCKET hSocket;
        // 进行连接，如果失败，则返回
        if (!ConnectSocket(addrConnect, hSocket))
        {
            printf("IRC connect failed\n");
            return;
        }

        if (!RecvUntil(hSocket, "Found your hostname", "using your IP address instead", "Couldn't look up your hostname"))
        {
            closesocket(hSocket);
            return;
        }
        // CAddress addrLocalHost在net.cpp中定义
        string strMyName = EncodeAddress(addrLocalHost);
        // 是否为本地地址 192.168....
        if (!addrLocalHost.IsRoutable())
            strMyName = strprintf("x%u", GetRand(1000000000));
        // 如果不是本地地址，strMyName为外网地址，否则为随机数
        // 发送昵称信息
        Send(hSocket, strprintf("NICK %s\r", strMyName.c_str()).c_str());
        // 注册账户信息 用户名、密码均为昵称
        Send(hSocket, strprintf("USER %s 8 * : %s\r", strMyName.c_str(), strMyName.c_str()).c_str());
        // 004 表示断开连接
        if (!RecvUntil(hSocket, " 004 "))
        {
            closesocket(hSocket);
            return;
        }
        Sleep(500);
        // 加入#bitcoin频道
        Send(hSocket, "JOIN #bitcoin\r");
        // who命令 得到频道用户在线列表
        Send(hSocket, "WHO #bitcoin\r");
        
        while (!fRestartIRCSeed)
        {
            string strLine;
            if (fShutdown || !RecvLineIRC(hSocket, strLine))
            {
                closesocket(hSocket);
                return;
            }
            if (strLine.empty() || strLine[0] != ':')
                continue;
            printf("IRC %s\n", strLine.c_str());

            vector<string> vWords;
            ParseString(strLine, ' ', vWords);
            if (vWords.size() < 2)
                continue;

            char pszName[10000];
            pszName[0] = '\0';

            if (vWords[1] == "352" && vWords.size() >= 8)
            {
                // index 7 is limited to 16 characters
                // could get full length name at index 10, but would be different from join messages
                strcpy(pszName, vWords[7].c_str());
                printf("GOT WHO: [%s]  ", pszName);
            }

            if (vWords[1] == "JOIN")
            {
                // :username!username@50000007.F000000B.90000002.IP JOIN :#channelname
                strcpy(pszName, vWords[0].c_str() + 1);
                if (strchr(pszName, '!'))
                    *strchr(pszName, '!') = '\0';
                printf("GOT JOIN: [%s]  ", pszName);
            }
            // 获取合适的比特币服务器地址并解析
            if (pszName[0] == 'u')
            {
                CAddress addr;
                if (DecodeAddress(pszName, addr))
                {
                    CAddrDB addrdb;
                    // 将地址保存到mapAddresses向量中
                    if (AddAddress(addrdb, addr))
                        printf("new  ");
                    addr.print();
                }
                else
                {
                    printf("decode failed\n");
                }
            }
        }

        fRestartIRCSeed = false;
        closesocket(hSocket);
    }
}









// 测试用
#ifdef TEST
int main(int argc, char *argv[])
{
    WSADATA wsadata;
    // 初始化winsock
    if (WSAStartup(MAKEWORD(2,2), &wsadata) != NO_ERROR)
    {
        printf("Error at WSAStartup()\n");
        return false;
    }

    ThreadIRCSeed(NULL);

    WSACleanup();
    return 0;
}
#endif
