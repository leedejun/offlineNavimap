     // Config.cpp  
      
    #include "Config.hpp"  
    #include <unistd.h>
      
    using namespace std;  
      
namespace conf
{
    Config::Config(string const & fileName){
        GetDirectory();
        m_cfgfilepath = m_Directory+fileName;
    }
      
     bool Config::readConfigFile(const string & key, string & value)
    {
        fstream cfgFile;
        cfgFile.open(m_cfgfilepath.c_str());//打开文件	
        if( ! cfgFile.is_open())
        {
            cout<<"can not open cfg file!"<<endl;
            return false;
        }
        char tmp[1000];
        while(!cfgFile.eof())//循环读取每一行
        {
            cfgFile.getline(tmp,1000);//每行读取前1000个字符，1000个应该足够了
            string line(tmp);
            size_t pos = line.find('=');//找到每行的“=”号位置，之前是key之后是value
            if(pos==string::npos) return false;
            string tmpKey = line.substr(0,pos);//取=号之前
            if(key==tmpKey)
            {
                value = line.substr(pos+1);//取=号之后
                return true;
            }
        }
        return false;
    }
    void Config::GetDirectory()
    {
        char abs_path[1024];
        int cnt = readlink("/proc/self/exe", abs_path, 1024);//获取可执行程序的绝对路径
        if(cnt < 0|| cnt >= 1024)
        {
            return;
        }

        //最后一个'/' 后面是可执行程序名，去掉devel/lib/m100/exe，只保留前面部分路径
        for(int i = cnt; i >= 0; --i)
        {
            if(abs_path[i]=='/')
            {
                abs_path[i + 1]='\0';
                break;
            }
        }
        stringstream stream;
        stream << abs_path;

        m_Directory = stream.str();
    }

    //返回0：无中文，返回1：有中文
    bool Config::IncludeChinese(const char *str)
    {
        char c;
        while(1)
        {
            c=*str++;
            if (c==0) break;  //如果到字符串尾则说明该字符串没有中文字符
            if (c&0x80)        //如果字符高位为1且下一字符高位也是1则有中文字符
                if (*str & 0x80) return true;
        }
        return false;
    }
}