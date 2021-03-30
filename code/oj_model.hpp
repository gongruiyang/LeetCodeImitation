/*
 *  试题模块
 * */
#pragma once 
#include <iostream>
#include <string>
#include <unordered_map>
#include <fstream>
#include <vector>
#include <boost/algorithm/string.hpp>
#include "tools.hpp"


//一道试题需要哪些属性来描述？
struct Question
{
  std::string id_;    // 题目ID
  std::string title_;//题目名称
  std::string star_;//题目的难易程度
  std::string path_;//题目的路径

  std::string desc_;  //题目的描述
  std::string header_cpp_; //题目预定义的头
  std::string tail_cpp_; //题目的尾，包含测试用例以及调用逻辑
};

// 从磁盘中加载题目，给用户提供接口查询
class ojModel
{
  private:
    std::unordered_map<std::string, Question> ques_map_;  //题目ID和内容
  public:
    ojModel()
    {
      //从配置文件中获取试题信息
      load("./oj_data/oj_config.cfg"); 
    }
    ~ojModel()
    {

    }
    //从文件中获取试题信息
    bool load(std::string filename)
    {
      std::ifstream file(filename.c_str());
      if(!file.is_open())
      {
        std::cout << "config file open failed!" << std::endl;
        return false;
      }
      // 1.打开文件成功的情况
      //  1.1 从文件中获取每一行信息
      //    1.1.1 对于每一行信息，还需要 获取 题号、题目名称、题目难易程度、题目路径
      //    1.1.2 保存在Question结构体中
      // 2.把多个question组织在map中
      
      std::string line;
      while(std::getline(file, line))
      {
        //使用Boost库中的split函数:按照空格进行切割
        std::vector<std::string> vec;
        StringUtil::Split(line, "\t", &vec);
        // is_any_of:支持多个字符作为分隔符
        // token_compress_off:是否将多个分割字符看做是一个
        //boost::split(vec, line, boost::is_any_of(" "), boost::token_compress_off);
        
        
        Question ques;
        ques.id_ = vec[0];
        ques.title_ = vec[1];
        ques.star_ = vec[2];
        ques.path_ = vec[3];
        std::string dir = vec[3];
        FileUtil::ReadFile(dir + "/desc.txt",&ques.desc_);
        FileUtil::ReadFile(dir + "/header.cpp",&ques.header_cpp_);
        FileUtil::ReadFile(dir + "/tail.cpp",&ques.tail_cpp_);

        ques_map_[ques.id_] = ques;
      }
      file.close();
      return true;
    }
    // 提供给上层调用者一个获取所有试题的接口
    bool GetAllQuestion(std::vector<Question>* questions)
    {
        // 1.遍历无序的map,将试题信息返回给调用者
        // 对于每一个试题，都是一个Question结构体对象
        for(const auto& kv : ques_map_)
        {
          questions->push_back(kv.second);  // 此时，questions中保存的便是 所有的试题信息
        }
        // questions中试题的顺序并不是按顺序保存的，所以需要 调整顺序
        // 对试题进行 排序
        sort(questions->begin(),questions->end(),[](const Question& l, const Question& r){
            //升序:比较Question中的题目编号:将string->int
            return std::stoi(l.id_) < std::stoi(r.id_);
            });
    }
    // 提供给上层调用者一个获取单个试题的接口
    // id 待查找题目的id
    // ques 是输出参数，将查到的结果返回给调用者
    bool GetOneQuestion(const std::string& id, Question* ques)
    {
      auto it = ques_map_.find(id);
      if(it == ques_map_.end())
      {
        return false;
      }
        *ques = it->second;
        return true;
    }
};
