// ConfigSingleton.h
#ifndef CONFIGSINGLETON_H
#define CONFIGSINGLETON_H

#include <string>
#include <unordered_map>
#include <mutex>

#ifndef VisableRasterTiles
#define VisableRasterTiles "VisableRasterTiles"
#endif

class ConfigSingleton {
public:
    static ConfigSingleton& getInstance() {
        static std::mutex mutex;
        std::lock_guard<std::mutex> lock(mutex);
        static ConfigSingleton instance;
        return instance;
    }

    // 删除拷贝构造函数和赋值操作符，确保单例
    ConfigSingleton(ConfigSingleton const&) = delete;
    void operator=(ConfigSingleton const&) = delete;

    // 配置获取和设置方法
    void setBoolValue(const std::string& key, bool value) {
        std::lock_guard<std::mutex> lock(mutex_);
        m_configBoolData[key] = value;
    }

    bool getBoolValue(const std::string& key) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = m_configBoolData.find(key);
        if (it != m_configBoolData.end()) {
            return it->second;
        }
        return false;
    }

    void setVisableRasterTiles(bool value)
    {
        std::string key = VisableRasterTiles;
        setBoolValue(key, value);
    }

    bool getVisableRasterTiles()
    {
        std::string key = VisableRasterTiles;
        return getBoolValue(key);
    }

private:
    // 私有构造函数
    ConfigSingleton() {}

    // 存储配置数据的容器
    std::unordered_map<std::string, bool> m_configBoolData;

    // 互斥锁
    mutable std::mutex mutex_;
};

#endif // CONFIGSINGLETON_H
