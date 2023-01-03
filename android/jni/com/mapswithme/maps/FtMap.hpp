#include "com/mapswithme/core/jni_helper.hpp"
#include "com/mapswithme/platform/Platform.hpp"
//#include "routing_common/ft_car_model_factory.hpp"


class CommandHelper{
    jclass      _commandClass;
    JNIEnv*     _env;
    jmethodID   _getMethd;
    jmethodID   _setMethd;
    jmethodID   _asyncMethd;
public:
    CommandHelper(){
        _env            = jni::GetEnv();
        _commandClass   = jni::GetGlobalClassRef(_env,"com/ftmap/maps/FTMap$Command");
        _getMethd       = _env->GetMethodID(_commandClass, "get", "(Ljava/lang/String;)Ljava/lang/Object;");
        _setMethd       = _env->GetMethodID(_commandClass, "set", "(Ljava/lang/String;Ljava/lang/Object;)Lcom/ftmap/maps/FTMap$Command;");
        _asyncMethd       = _env->GetMethodID(_commandClass, "asyncResult", "(Ljava/lang/Object;)V");
    }

    bool IsJavaInstanceOf(jobject object, const std::string& class_name) {
        jclass clazz = _env->FindClass(class_name.c_str());
        return clazz ? _env->IsInstanceOf(object, clazz) == JNI_TRUE : false;
    }

    jobject getObj(jobject obj,const std::string& key){
        return _env->CallObjectMethod(obj,_getMethd,jni::ToJavaString(_env,key.c_str()));
    }

    jstring getNativeStr(jobject obj,const std::string& key){
        jobject o = getObj(obj,key);
        if( !IsJavaInstanceOf(o,"java/lang/String") )
            throw;
        return static_cast<jstring>(o);
    }

    std::string getStr(jobject obj,const std::string& key){
        return jni::ToNativeString(_env,getNativeStr(obj,key));
    }

    int getInt(jobject obj,const std::string& key){
        jobject o = getObj(obj,key);
        if( !IsJavaInstanceOf(o,"java/lang/Integer") )
            throw;
        jmethodID method = _env->GetMethodID(_env->GetObjectClass(o), "intValue", "()I");
        return _env->CallIntMethod(o, method);
    }

    long getLong(jobject obj,const std::string& key){
        jobject o = getObj(obj,key);
        if( !IsJavaInstanceOf(o,"java/lang/Long") )
            throw;
        jmethodID method = _env->GetMethodID(_env->GetObjectClass(o), "longValue", "()J");
        return _env->CallLongMethod(o, method);
    }

    float getFloat(jobject obj,const std::string& key){
        jobject o = getObj(obj,key);
        if( !IsJavaInstanceOf(o,"java/lang/Float") )
            throw;
        jmethodID method = _env->GetMethodID(_env->GetObjectClass(o), "floatValue", "()F");
        return _env->CallFloatMethod(o, method);
    }

    double getDouble(jobject obj,const std::string& key){
        jobject o = getObj(obj,key);
        if( !IsJavaInstanceOf(o,"java/lang/Double") )
            throw;
        jmethodID method = _env->GetMethodID(_env->GetObjectClass(o), "doubleValue", "()D");
        return _env->CallDoubleMethod(o, method);
    }

    bool getBool(jobject obj,const std::string& key){
        jobject o = getObj(obj,key);
        if( !IsJavaInstanceOf(o,"java/lang/Boolean") ){
            throw;
        }
        jmethodID method = _env->GetMethodID(_env->GetObjectClass(o), "booleanValue", "()Z");
        return _env->CallBooleanMethod(o, method);
    }

    void set(jobject obj, const std::string& key, bool value){
        jclass clazz = _env->FindClass("java/lang/Boolean");
        jmethodID constructorID = _env->GetMethodID(clazz, "<init>", "(Z)V");
        jobject result = _env->NewObject(clazz, constructorID, value ? JNI_TRUE : JNI_FALSE);
        _env->CallObjectMethod(obj,_setMethd,jni::ToJavaString(_env,key.c_str()),result);
    }

    void set(jobject obj, const std::string& key, int value){
        jclass clazz = _env->FindClass("java/lang/Integer");
        jmethodID constructorID = _env->GetMethodID(clazz, "<init>", "(I)V");
        jobject result = _env->NewObject(clazz, constructorID, value);
        _env->CallObjectMethod(obj,_setMethd,jni::ToJavaString(_env,key.c_str()),result);
    }

    void set(jobject obj, const std::string& key, long value){
        jclass clazz = _env->FindClass("java/lang/Long");
        jmethodID constructorID = _env->GetMethodID(clazz, "<init>", "(J)V");
        jobject result = _env->NewObject(clazz, constructorID, value);
        _env->CallObjectMethod(obj,_setMethd,jni::ToJavaString(_env,key.c_str()),result);
    }
    void set(jobject obj, const std::string& key, long long value){
        jclass clazz = _env->FindClass("java/lang/Long");
        jmethodID constructorID = _env->GetMethodID(clazz, "<init>", "(J)V");
        jobject result = _env->NewObject(clazz, constructorID, value);

        _env->CallObjectMethod(obj,_setMethd,jni::ToJavaString(_env,key.c_str()),result);
    }
    void set(jobject obj, const std::string& key, double  value){
        jclass clazz = _env->FindClass("java/lang/Long");
        jmethodID constructorID = _env->GetMethodID(clazz, "<init>", "(J)V");
        jobject result = _env->NewObject(clazz, constructorID, value);
        _env->CallObjectMethod(obj,_setMethd,jni::ToJavaString(_env,key.c_str()),result);
    }


    void set(jobject obj, const std::string& key, jobject value){
        _env->CallObjectMethod(obj,_setMethd,jni::ToJavaString(_env,key.c_str()),value);
    }

    void asyncCall(jobject obj, jobject value){
        jmethodID mid = _env->GetMethodID(_commandClass, "asyncResult", "(Ljava/lang/Object;)V");
        _env->CallVoidMethod(obj,mid,value);
        _env->DeleteGlobalRef(obj);
    }

    static CommandHelper& getIns(){
        static CommandHelper ins;
        return ins;
    }
};

class JsonHelper{
    jclass      _jsonArrayClass;
    jclass      _jsonObjectClass;
    jmethodID   _appendMethd;
    jmethodID   _setMethd;
    JNIEnv*     _env;
public:
    JsonHelper(){
        _env            = jni::GetEnv();
        _jsonArrayClass   = jni::GetGlobalClassRef(_env,"org/json/JSONArray");
        _jsonObjectClass  = jni::GetGlobalClassRef(_env,"org/json/JSONObject");
        _appendMethd       = _env->GetMethodID(_jsonArrayClass, "put", "(Ljava/lang/Object;)Lorg/json/JSONArray;");
        _setMethd           = _env->GetMethodID(_jsonObjectClass, "put", "(Ljava/lang/String;Ljava/lang/Object;)Lorg/json/JSONObject;");
    }

    jobject createJSONArray(){
        jmethodID constructorID = _env->GetMethodID(_jsonArrayClass, "<init>", "()V");
        return _env->NewObject(_jsonArrayClass, constructorID);
    }

    jobject createJSONObject(){
        jmethodID constructorID = _env->GetMethodID(_jsonObjectClass,"<init>","()V");
        return _env->NewObject(_jsonObjectClass, constructorID);
    }

    void append(jobject array,jobject obj){
        _env->CallObjectMethod(array,_appendMethd,obj);
    }
    int length(jobject obj){
        jmethodID mid = jni::GetMethodID(_env,obj,"length","()I");
        return _env->CallIntMethod(obj,mid);
    }
    jobject getJSONObject(jobject obj, int idx){
        jmethodID mid = jni::GetMethodID(_env,obj,"getJSONObject","(I)Lorg/json/JSONObject;");
        return _env->CallObjectMethod(obj,mid,idx);
    }
    jobject getJSONObject(jobject obj, const std::string& key){
        jmethodID mid = jni::GetMethodID(_env,obj,"getJSONObject","(Ljava/lang/String;)Lorg/json/JSONObject;");
        return _env->CallObjectMethod(obj,mid,jni::ToJavaString(_env,key.c_str()));
    }
    bool getBool(jobject obj, const std::string& key){
        jmethodID mid = jni::GetMethodID(_env,obj,"getBoolean","(Ljava/lang/String;)Z");
        return _env->CallBooleanMethod(obj,mid,jni::ToJavaString(_env,key.c_str()));
    }
    int getInt(jobject obj, const std::string& key){
        jmethodID mid = jni::GetMethodID(_env,obj,"getInt","(Ljava/lang/String;)I");
        return _env->CallIntMethod(obj,mid,jni::ToJavaString(_env,key.c_str()));
    }
    long getLong(jobject obj, const std::string& key){
        jmethodID mid = jni::GetMethodID(_env,obj,"getLong","(Ljava/lang/String;)J");
        return _env->CallLongMethod(obj,mid,jni::ToJavaString(_env,key.c_str()));
    }
    long getLong(jobject obj, int idx){
        jmethodID mid = jni::GetMethodID(_env,obj,"getLong","(I)J");
        return _env->CallLongMethod(obj,mid,idx);
    }
   long long getLongLong(jobject obj, int idx){
        jmethodID mid = jni::GetMethodID(_env,obj,"getLong","(I)J");
        return _env->CallLongMethod(obj,mid,idx);
    }
    double getDouble(jobject obj, const std::string& key){
        jmethodID mid = jni::GetMethodID(_env,obj,"getDouble","(Ljava/lang/String;)D");
        return _env->CallDoubleMethod(obj,mid,jni::ToJavaString(_env,key.c_str()));
    }
    double getDouble(jobject obj, int idx){
        jmethodID mid = jni::GetMethodID(_env,obj,"getDouble","(I)D");
        return _env->CallDoubleMethod(obj,mid,idx);
    }
    std::string getString(jobject obj, const std::string& key){
        jmethodID mid = jni::GetMethodID(_env,obj,"getString","(Ljava/lang/String;)Ljava/lang/String;");
        jobject s = _env->CallObjectMethod(obj,mid,jni::ToJavaString(_env,key.c_str()));
        jboolean isCopy;
        std::string const res = _env->GetStringUTFChars((jstring)s, &isCopy);
//        std::string tmp = res;
//        _env->DeleteLocalRef(s);
        return res;
    }
    std::string getString(jobject obj, int idx){
        jmethodID mid = jni::GetMethodID(_env,obj,"getString","(I)Ljava/lang/String;");
        jobject s = _env->CallObjectMethod(obj,mid,idx);
        jboolean isCopy;
        std::string const res = _env->GetStringUTFChars((jstring)s, &isCopy);
//        std::string tmp = res;
//        _env->DeleteLocalRef(s);
        return res;
    }

    jstring toJNIString(const std::string& value){
        return jni::ToJavaString(_env,value.c_str());
    }

    void setString(jobject obj, const std::string& key, const std::string& value){
        _env->CallObjectMethod(obj,_setMethd,jni::ToJavaString(_env,key.c_str()),jni::ToJavaString(_env,value.c_str()));
    }
    void setInt(jobject obj, const std::string& key, int value){
        jmethodID mid = jni::GetMethodID(_env,obj,"put","(Ljava/lang/String;I)Lorg/json/JSONObject;");
        _env->CallObjectMethod(obj,mid,jni::ToJavaString(_env,key.c_str()),value);
    }
    void setDouble(jobject obj, const std::string& key, double value){
        jmethodID mid = jni::GetMethodID(_env,obj,"put","(Ljava/lang/String;D)Lorg/json/JSONObject;");
        _env->CallObjectMethod(obj,mid,jni::ToJavaString(_env,key.c_str()),value);
    }
    void setDouble(jobject obj, double value){
        jmethodID mid = jni::GetMethodID(_env,obj,"put","(D)Lorg/json/JSONArray;");
        _env->CallObjectMethod(obj,mid,value);
    }
    void setBool(jobject obj, const std::string& key, bool value){
        jmethodID mid = jni::GetMethodID(_env,obj,"put","(Ljava/lang/String;Z)Lorg/json/JSONObject;");
        _env->CallObjectMethod(obj,mid,jni::ToJavaString(_env,key.c_str()),(jboolean)value);
    }
    void setObject(jobject obj, const std::string& key, jobject value){
        jmethodID mid = jni::GetMethodID(_env,obj,"put","(Ljava/lang/String;Ljava/lang/Object;)Lorg/json/JSONObject;");
        _env->CallObjectMethod(obj,mid,jni::ToJavaString(_env,key.c_str()),value);
    }

    jobject Point2JSONObject(const m2::PointD& p){
//        auto o = this->createJSONObject();
        jmethodID constructorID = _env->GetMethodID(_jsonObjectClass,"<init>","()V");
        auto o = _env->NewObject(_jsonObjectClass, constructorID);
        jmethodID mid = jni::GetMethodID(_env,o,"put","(Ljava/lang/String;D)Lorg/json/JSONObject;");
        _env->CallObjectMethod(o,mid,jni::ToJavaString(_env,"x"),p.x);
        _env->CallObjectMethod(o,mid,jni::ToJavaString(_env,"y"),p.y);
//        set(o,"x",p.x);
//        set(o,"y",p.y);
        return o;
    }



    static JsonHelper& getIns(){
        static JsonHelper ins;
        return ins;
    }
};
