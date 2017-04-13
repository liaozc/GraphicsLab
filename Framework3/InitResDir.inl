#include<string>
std::string g_workDir;
#define ResDir(localPath) (g_workDir + localPath).c_str()
void initWorkDir()
{
	char ch[256];
	GetCurrentDirectory(256, ch);
	std::string path(ch);
	int size = (int)path.size();
	for (int i = 0; i < size; ++i) {
		if (path[i] == '\\')
			path[i] = '/';
	}
	int slash = path.rfind('/');
	g_workDir = path.substr(0, slash) + LocalDir;
}
