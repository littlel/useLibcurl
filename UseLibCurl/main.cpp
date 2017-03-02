#include "ccurl.h"
#include <string>
using std::string;

#define USE_LOG4CPLUS
#include "outctrl.h"

int main()
{
	CCurl * pCurl = CCurl::Instance();
	pCurl->setHostUserPwd("admin", "12345", "192.168.10.58");
	
	/*if (pCurl->upload("E:/ChromeGo.7z", "aa/yes.7z"))
	{
	CLOG("upload ok111~~~");
	}*/

	if (pCurl->download("uploading.gif", "E:/123.gif"))
	{
		CLOG("down ok 222~~~");
	}
	system("pause");
	return 0;
}