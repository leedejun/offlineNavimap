    #pragma once
    #include <string>  
    #include <map> 
    #include <iostream> 
    #include <fstream>  
    #include <sstream>
    #include <stdio.h>
      
    /* 
    * \brief Generic configuration Class 
    * 
    */  
   namespace conf
{
    class Config {   
    public:  
        std::string m_cfgfilepath;
        std::string m_Directory;
        Config(std::string const & fileName);
        
        bool readConfigFile(const std::string & key, std::string & value);
        void GetDirectory();
        bool IncludeChinese(const char *str);
    };  
      
      
 
}