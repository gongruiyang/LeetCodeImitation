#pragma once 
#include <atomic>
#include <iostream>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <jsoncpp/json/json.h>
#include "tools.hpp"

enum errorno
{
  OK = 0,
  PRAM_ERROR,   //参数错误
  INTERNAL_ERROR, //内部错误
  COMPILE_ERROR,  //编译错误
  RUN_ERROR //运行错误
};

class Compiler{
  public:
    /*
     * Json::Value Req 请求的json
     *    { "code":"xxx", "stdin":"xxx" }
     *  Json::Value* Resp 出参，返回给调用者
     *    { "errorno":"xx", "reason":"xxx" }
     */ 
    static void CompileAndRun(Json::Value Req, Json::Value* Resp)
    {
      
      // 1.参数是否错误
      if(Req["code"].empty())
      {
        (*Resp)["errorno"] = PRAM_ERROR;
        (*Resp)["reason"] = "Pram error";
        return;
      }
      // 2.将代码写到文件里面 文件命名规则：tmp_时间戳_变量值.cpp
      //    时间戳：用于区分不同时间创建出来的文件
      //    变量值：用于区分 高并发情况 下产生的 时间戳相同 的情况下 区分文件
      std::string code = Req["code"].asString();
      std::string file_nameheader = WriteTmpFile(code); 
      if(file_nameheader == "") //写文件失败
      {
        (*Resp)["errorno"] = INTERNAL_ERROR;
        (*Resp)["reason"] = "write file failed";
        return ;
      }
      // 3.编译
      if(!Compile(file_nameheader))
      {
        (*Resp)["errorno"] = COMPILE_ERROR;
        std::string reason;
        FileUtil::ReadFile(CompileErrorPath(file_nameheader).c_str(),&reason);
        (*Resp)["reason"] = reason;
        return ;
      }

      // 4.运行
      // -1 -2   >0
      int ret = Run(file_nameheader);
      if(ret != 0)
      {
        (*Resp)["errorno"] = RUN_ERROR;
        std::string reason = "program exit by sig:" + std::to_string(ret);
        (*Resp)["reason"] = reason;
        return ;
      }

      // 5.构造响应
      (*Resp)["errorno"] = OK;
      (*Resp)["reason"] = "Compile and Run OK";
      
      std::string stdout_str;
      FileUtil::ReadFile(stdoutPath(file_nameheader).c_str(), &stdout_str);
      (*Resp)["stdout"] = stdout_str;
      
      std::string stderr_str; 
      FileUtil::ReadFile(stderrPath(file_nameheader).c_str(), &stderr_str);
      (*Resp)["stderr"] = stderr_str;
     
      // 6.删除临时文件
      clean(file_nameheader); 

      return ;
    }

  private:
    static void clean(const std::string& file_name)
    {
      unlink(SrcPath(file_name).c_str());
      unlink(CompileErrorPath(file_name).c_str());
      unlink(stdoutPath(file_name).c_str());
      unlink(stderrPath(file_name).c_str());
      unlink(ExePath(file_name).c_str());
    }

    static int Run(const std::string& file_name)
    {
      // 1.创建子进程
      int pid = fork();
      // 2.父进程进行进程等待，子进程进行进程程序替换
      if(pid < 0)
      {
        return -1;
      }
      else if(pid > 0)
      {
        int status;
        waitpid(pid,&status,0);
        return status & 0x7f;
      }
      else 
      {
        // 注册一个定时器,防止用户提交的代码中含有死循环等长时间运行
        alarm(1);
        // 限制进程使用的资源
        struct rlimit rlim;
        rlim.rlim_cur = 30000 * 1024; // 3wk
        rlim.rlim_max = RLIM_INFINITY;
        setrlimit(RLIMIT_AS, &rlim);
        
        int stdout_fd = open(stdoutPath(file_name).c_str(), O_CREAT | O_WRONLY, 0666);
        if(stdout_fd < 0)
        {
          return -2;
        }
        dup2(stdout_fd,1);
        
        int stderr_fd = open(stderrPath(file_name).c_str(), O_CREAT | O_WRONLY, 0666);
        if(stderr_fd < 0)
        {
          return -2;
        }
        dup2(stderr_fd, 2);
        
        execl(ExePath(file_name).c_str(), ExePath(file_name).c_str(), NULL);
        exit(0);
      }
      return 0;
    }

    static bool Compile(const std::string file_name)
    {
      // 1.创建子进程 
      int pid = fork();
      // 2.父进程进行进程等待，子进程进行进程程序替换
      if(pid > 0)
      {
        waitpid(pid,NULL,0);
      }
      else if(pid == 0)
      {
        int fd = open(CompileErrorPath(file_name).c_str(), O_CREAT | O_WRONLY, 0666);
        if(fd < 0)
        {
          return false;
        }
        dup2(fd,2);// 将标准错误重定向为fd，标准错误的输出，将会被输出在文件当中

        // 进行进程程序替换成g++ SrcPath(file_name) -o ExePath(file_name) -std=c++11
        execlp("g++","g++",SrcPath(file_name).c_str(),"-o",ExePath(file_name).c_str(),"-std=c++11","-D","CompileOnline",NULL);
        
        close(fd);
        exit(0);
      }
      else 
      {
        return false;
      }
      // 如果替换失败了，就要返回false，判断失败的标准：是否产生了可执行文件
      // 使用stat函数查看可执行文件属性
      struct stat st;
      int ret = stat(ExePath(file_name).c_str(), &st);
      if(ret < 0)
      {
        return false;
      }
      return true;
    }


    static std::string stdoutPath(const std::string& filename)
    {
      return "./tmp_file/" + filename + "_stdout";
    }
    static std::string stderrPath(const std::string& filename)
    {
      return "./tmp_file/" + filename + "_stderr";
    }


    // 获取编译错误文件路径
    static std::string CompileErrorPath(const std::string& filename)
    {
      return "./tmp_file/" + filename + "_compil_err_info";
    }
    // 编译好的文件路径
    static std::string ExePath(const std::string& filename)
    {
      return "./tmp_file/" + filename + "_executable";
    }

    // 获取源文件路径和文件名 
    static std::string SrcPath(const std::string& filename)
    {
      return "./tmp_file/" + filename + ".cpp";
    }
    
    static std::string WriteTmpFile(const std::string& code)
    {
      // 1.组织文件名称，区分源码文件，以及后面生成的可执行文件
      static std::atomic_uint id(0);
      std::string tmp_filename = "tmp_" + std::to_string(TimeUtil::GetTimeStampMs()) + "_" + std::to_string(id);
      id++;
      // 2.将code写入文件中
      FileUtil::WriteFile(SrcPath(tmp_filename), code);
      return tmp_filename;
    }
};
