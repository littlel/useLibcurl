#include <sstream>
#include "ccurl.h"

#define USE_LOG4CPLUS
#include "outctrl.h"

using std::stringstream;

#ifdef _DEBUG
#pragma comment(lib, "libcurl_a_debug.lib")
#else
#pragma comment(lib, "libcurl_a.lib")
#endif

CCurl *CCurl::m_pInstance = nullptr;  
CCurl::CGarbo CCurl::m_Garbo;

bool CCurl::m_bGlobalInit = false;
CURL *CCurl::curl = nullptr;

curl_off_t CCurl::m_nUploaded = 0;
curl_off_t CCurl::m_nDownloaded = 0;

CCurl::CCurl()
{
	init();
}

CCurl * CCurl::Instance()
{
	if (m_pInstance == nullptr)
	{
		m_pInstance = new CCurl();
	}
	return m_pInstance;
}

CCurl::CGarbo::~CGarbo()
{
	if (curl != nullptr)
	{
		curl_easy_cleanup(curl);
	}

	if (m_bGlobalInit)
	{
		curl_global_cleanup();
	}

	if (m_pInstance != nullptr)
	{
		delete m_pInstance;
	}
}

size_t CCurl::readCallback(char *buffer, size_t size, size_t nitems, void *instream)
{
	return fread(buffer, size, nitems, (FILE *)instream);
}

size_t CCurl::writeCallback(char *buffer, size_t size, size_t nitems, void *outstream)
{
	return fwrite(buffer, size, nitems, (FILE *)outstream);
}

int CCurl::uploadProgressCallback(void *clientp, curl_off_t dltotal, curl_off_t dlnow,
								  curl_off_t ultotal, curl_off_t ulnow)
{
	/* 注意后面四个形参
	 * 1. 含义 : 表征传输过程中的文件大小（单位：字节Bytes）
	 *    dltotal : 下载文件总大小
	 *	  dlnow   : 已下载大小
	 *    ultotal : 上传文件总大小
	 *    ulnow	  : 已上传大小（单位：字节Bytes）
	 * 2. 类型: curl_off_t 
	 *    其输出时格式采用 CURL_FORMAT_OFF_T
	 *    例如: printf("uplaoding "CURL_FORMAT_OFF_T"/"CURL_FORMAT_OFF_T"...\n", ulnow, ultotal);
	 */ 
	if (ultotal == 0 || ulnow == 0)
	{
		return 0;
	}

	if (m_nUploaded != ulnow)
	{
		// uplaoded percentage
		float nPos = (ulnow * 100.0 / ultotal);
		CLOG("uplaoding %.2f%%...", nPos);

		// uplaoding speed
		double speed;
		curl_easy_getinfo(curl, CURLINFO_SPEED_UPLOAD, &speed);
		string strSpeed = speedConvert(speed);
		CLOG("uplaoding speed: %s", strSpeed.c_str());

		m_nUploaded = ulnow;
	}

	return 0;
}

int CCurl::downloadProgressCallback(void *clientp, curl_off_t dltotal, curl_off_t dlnow,
								  curl_off_t ultotal, curl_off_t ulnow)
{
	if (dltotal == 0 || dlnow == 0)
	{
		return 0;
	}

	if (m_nDownloaded != dlnow)
	{
		// downloaded percentage
		float nPos = (dlnow * 100.0 / dltotal);
		CLOG("downloading %.2f%%...", nPos);

		// downloading speed
		double speed;
		curl_easy_getinfo(curl, CURLINFO_SPEED_DOWNLOAD, &speed);
		string strSpeed = speedConvert(speed);
		CLOG("downloading speed: %s", strSpeed.c_str());

		m_nDownloaded = dlnow;
	}

	return 0;
}

string CCurl::speedConvert(double &speed)
{
	stringstream ss;
	if (speed < 1024.00)
	{
		ss << speed;
		ss << " B/s";
	}
	else if (speed >= 1024.00 && speed < 1024.00 * 1024.00)
	{  
		speed /= 1024.00;  
		ss << speed;
		ss << " KiB/s";
	}
	else if (speed >= 1024.00 * 1024.00 && speed < 1024.00 * 1024.00 * 1024.00)  
	{  
		speed /= (1024.00 * 1024.00);  
		ss << speed;
		ss << " MiB/s";
	}
	else
	{  
		speed /= (1024.00 * 1024.00 * 1024.00);  
		ss << speed;
		ss << " GiB/s";
	}

	return ss.str();
}

void CCurl::init()
{
	CLOG(curl_version());

	// global initialization
	CURLcode res = curl_global_init(CURL_GLOBAL_DEFAULT);
	if (CURLE_OK != res)
	{
		m_bGlobalInit = false;
		CLOG("Global initialization failed");
		exit(res);
	}
	m_bGlobalInit = true;

	// initialization of easy handle
	curl = curl_easy_init();
	if (nullptr == curl)
	{
		CLOG("Init curl failed");
		exit(1);
	}

	CLOG("Init curl successfully!");
}

int CCurl::get_file_size(FILE *file)
{
	if (file == nullptr)
	{
		return 0;
	}

	fseek(file, 0L, SEEK_END);
	int nFileSize = ftell(file);
	fseek(file, 0L, SEEK_SET);

	return nFileSize;
}

void CCurl::setUploadOpt(FILE *file, const char* url)
{
	if (curl == nullptr || url == nullptr || m_strUserPwd.empty())
	{
		CLOG("null or empty error");
		return;
	}

	int nSize = get_file_size(file);
	if (nSize == 0)
	{
		CLOG("File's size is zero");
		return;
	}

	m_nUploaded = 0;

	// reset all options of curl handle
	curl_easy_reset(curl);

	// specify target URL
	curl_easy_setopt(curl, CURLOPT_URL, url);

	// specify user and password
	curl_easy_setopt(curl, CURLOPT_USERPWD, m_strUserPwd.c_str());

	// enable uploading
	curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);

	// create dir if not existed on ftp server
	curl_easy_setopt(curl, CURLOPT_FTP_CREATE_MISSING_DIRS, 1L);

	// read callback function
	curl_easy_setopt(curl, CURLOPT_READFUNCTION, &CCurl::readCallback);
	
	// specify which file to upload
	curl_easy_setopt(curl, CURLOPT_READDATA, file);

	// set the size of the file to upload
	curl_easy_setopt(curl, CURLOPT_INFILESIZE, nSize);

	// get the process
	curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
	curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, &CCurl::uploadProgressCallback); 
	curl_easy_setopt(curl, CURLOPT_XFERINFODATA, nullptr);

#ifdef _DEBUG
	curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
#endif
}

void CCurl::setDownLoadOpt(FILE *file, const char* url)
{
	if (curl == nullptr || url == nullptr || m_strUserPwd.empty())
	{
		CLOG("null or empty error");
		return;
	}

	m_nDownloaded = 0;

	// reset all options of curl handle
	curl_easy_reset(curl);

	// specify target URL
	curl_easy_setopt(curl, CURLOPT_URL, url);

	// specify user and password
	curl_easy_setopt(curl, CURLOPT_USERPWD, m_strUserPwd.c_str());

	// write callback function
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &CCurl::writeCallback);

	// Set a pointer to pass to the callback
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, file);

	// get the process
	curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
	curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, &CCurl::downloadProgressCallback); 
	curl_easy_setopt(curl, CURLOPT_XFERINFODATA, nullptr);
	
#ifdef _DEBUG
	curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
#endif
}

void CCurl::setUserPwd(char *name, char *pwd)
{
	if (name != nullptr && pwd != nullptr)
	{
		m_strUserPwd = string(name);
		m_strUserPwd = m_strUserPwd + ":" + pwd;
	}
}

void CCurl::setHostInfo(char *ip, int port/* = 21*/)
{
	if (ip != nullptr)
	{
		stringstream ss;
		ss << string("ftp://");
		ss << ip;
		if (port != 21)
		{
			ss << ":";
			ss << port;
		}
		ss << "/";

		m_strServerUrl = ss.str();
	}
}

void CCurl::setHostUserPwd(char *name, char *pwd, char *ip, int port/* = 21*/)
{
	setUserPwd(name, pwd);
	setHostInfo(ip, port);
}

string CCurl::getUploadFullName(char *localFullName, char *remoteFileName)
{
	if (m_strServerUrl.empty() || localFullName == nullptr)
	{
		return "";
	}

	string strRemoteFile;
	if (remoteFileName == nullptr)
	{
		char *filename = strrchr(localFullName, '/') + 1;
		strRemoteFile = m_strServerUrl + filename;
	}
	else
	{
		strRemoteFile = m_strServerUrl + remoteFileName;
	}

	return strRemoteFile;
}

bool CCurl::upload(char *localFullName, char *remoteFileName/* = nullptr*/)
{
	// check
	if (curl == nullptr)
	{
		CLOG("Curl handle null");
		return false;
	}

	if (m_strServerUrl.empty())
	{
		CLOG("Please set ftp server info");
		return false;
	}

	if (localFullName == nullptr)
	{
		CLOG("Please set valid uploaded file");
		return false;
	}

	// get CURLOPT_URL
	string strRemoteFile = getUploadFullName(localFullName, remoteFileName);
		
	// open file
	FILE *fpSrc = fopen(localFullName, "rb");
	if (nullptr == fpSrc)
	{
		CLOG("File open error");
		return false;
	}

	// curl_easy_setopt for uploading
	setUploadOpt(fpSrc, strRemoteFile.c_str());

	// execute
	CURLcode res = curl_easy_perform(curl);
	
	// close file
	if (fpSrc != nullptr)
	{
		fclose(fpSrc);
	}

	if (res != CURLE_OK)
	{
		CLOG("curl_easy_perform() failed: %s", curl_easy_strerror(res));
		return false;
	}

	return true;
}

bool CCurl::download(char *remoteFileName, char *localFullName/* = nullptr*/)
{
	// check
	if (curl == nullptr)
	{
		CLOG("Curl handle null");
		return false;
	}

	if (m_strServerUrl.empty())
	{
		CLOG("Please set ftp server info");
		return false;
	}

	if (remoteFileName == nullptr)
	{
		CLOG("Please set valid downloaded file");
		return false;
	}

	// get CURLOPT_URL
	string strRemoteFile = m_strServerUrl + remoteFileName;

	// open file
	if (localFullName == nullptr)
	{
		localFullName = remoteFileName;
	}
	FILE *fpSrc = fopen(localFullName, "wb");
	if (nullptr == fpSrc)
	{
		CLOG("File open error");
		return false;
	}

	// curl_easy_setopt for downloading
	setDownLoadOpt(fpSrc, strRemoteFile.c_str());

	// execute
	CURLcode res = curl_easy_perform(curl);

	// close file
	if (fpSrc != nullptr)
	{
		fclose(fpSrc);
	}

	if (res != CURLE_OK)
	{
		CLOG("curl_easy_perform() failed: %s", curl_easy_strerror(res));
		return false;
	}

	return true;
}