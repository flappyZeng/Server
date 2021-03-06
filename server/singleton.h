#pragma once 

namespace server{
    //设置一个单例
    template<class T, class X = void, int N = 0>
    class Singleton{
    public:
        static T* getInstance(){
            static T v;
            return &v;
        }
    };

    //智能指针单例
    template<class T, class X = void, int N = 0>
    class SingletonPtr{
    public:
        static std::shared_ptr<T> getInstance(){
            static std::shared_ptr<T> v(new T);
            return v;
        }
    };
}