#pragma once

#include<pthread.h>
#include<unistd.h>
#include<sys/syscall.h>
#include<stdint.h>
#include<string>
#include<vector>
#include<numeric>
namespace server{

    //全局的方法使用大写字母开头
    pid_t GetThreadId();

    //输出程序运行栈, 参数设定，bt，存储栈输出， size: 输出栈的层数， skip： 跳过若干层(方便读写)
    void BackTrace(std::vector<std::string>& bt, int size, int skip = 1);
    std::string BackTrace(int size = 64, int skip = 1, const std::string& prefix = "");

    uint64_t GetCurrentMs();
    uint64_t GetCurrentUs();
}
