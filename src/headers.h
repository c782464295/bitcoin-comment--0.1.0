

#ifdef _MSC_VER
#pragma warning(disable:4786)
#pragma warning(disable:4804)
#pragma warning(disable:4717)
#endif
#ifdef _WIN32_WINNT
#undef _WIN32_WINNT
#endif
//#define _WIN32_WINNT 0x0400
#define WIN32_LEAN_AND_MEAN 1
#include <wx/wx.h>
#include <wx/clipbrd.h>
#include <wx/snglinst.h>
#include <openssl/ecdsa.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/sha.h>
#include <openssl/ripemd.h>
#include <winsock2.h>
#include <mswsock.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <io.h>
#include <math.h>
#include <limits.h>
#include <float.h>
#include <assert.h>
#include <process.h>
#include <malloc.h>
#include <memory>
#define BOUNDSCHECK 1
#include <sstream>
#include <string>
#include <vector>
#include <list>
#include <deque>
#include <map>
#include <set>
#include <algorithm>
#include <numeric>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/tuple/tuple_comparison.hpp>
#include <boost/tuple/tuple_io.hpp>
#include <boost/array.hpp>
#pragma hdrstop
using namespace std;
using namespace boost;



#include "serialize.h"//序列化
#include "uint256.h"//无符号256位整数运算相关
#include "util.h"//hash函数等
#include "key.h"//公钥私钥、验证、签名
#include "bignum.h"//大数运算相关
#include "base58.h"//base58编码相关
#include "script.h"//脚本
#include "db.h"//数据库
#include "net.h"//网络，获取IP地址，节点连接及广播消息
#include "irc.h"//irc远程中继聊天服务，主要用于发现节点
#include "main.h"//主程序
#include "market.h"// 未知
#include "uibase.h"//页面布局
#include "ui.h"//绑定按钮事件等
