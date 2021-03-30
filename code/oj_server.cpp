#include <stdio.h>
#include <iostream>
#include <jsoncpp/json/json.h>

#include "oj_model.hpp"
#include "httplib.h"
#include "oj_view.hpp"
#include "compile.hpp"

using namespace httplib;

int main()
{
  // 1. 初始化httplib库的server对象
  Server svr;
  ojModel model;
  // 2. 提供三个接口，分别处理三个请求
  // 2.1 获取整个试题列表，get 
  svr.Get("/all_questions",[&model](const Request& req, Response& resp){
     // 1.返回试题列表
     std::vector<Question> questions;
     model.GetAllQuestion(&questions);
     // 2.将试题列表渲染至html页面上去
     std::string html;
     OJView::DrawAllQuestions(questions, &html);
     resp.set_content(html, "text/html");
     });

  // 2.2 获取单个试题
  //  如何表示浏览器想要获取的是哪一个试题？
  //  使用正则表达式解决点击题目后自动跳转至对应的URL
  //  \d表示数字0-9
  //  \d+表示多位数字
  svr.Get(R"(/question/(\d+))",[&model](const Request& req, Response& resp){
      // 1.获取url当中关于试题的数字 & 获取单个试题的信息
      // matches[1]返回的是题目id，并不是一个string
      //std::cout << req.matches[1] << std::endl;
      Question ques;
      // 通过试题数字(id)进而获取试题信息
      model.GetOneQuestion(req.matches[1].str(),&ques); 
      
      // 2.渲染模板的html文件
      std::string html;
      OJView::DrawOneQuestion(ques, &html);  
      
      resp.set_content(html, "text/html");
      });

  // 2.3 解释运行
  // 目前还没有区分到底是提交的是哪一个试题
      // 1.获取试题编号
      // 2.通过 试题编号 进而获取 题目内容 填充到ques中
      // 3.用户点击submit后从浏览器获取代码，将代码进行decode -> code
      // 4.将代码与tail.cpp合并:code + tail.cpp -> src文件 
      // 5.编译模块进行编译：
      //  编译src文件的技术：子进程创建，使用子进程程序替换成为"g++"来编译源码文件
      //    成功:
      //    失败:
      //  运行变异后的文件技术:子进程创建，使用子进程程序替换成为编译出来的可执行程序
      //    成功：
      //    失败:
  svr.Post(R"(/compile/(\d+))",[&model](const Request& req, Response& resp){
      // 1.获取题目id ---> ques_id 
      std::string ques_id = req.matches[1].str();
      
      // 2.获取题目内容 ---> ques 
      Question ques;
      model.GetOneQuestion(ques_id,&ques); 

      // 3.获取代码内容 ---> code 
      /*  以下注释中的代码容错率较低，如果代码中用户代码中出现=，就无法处理 
      std::vector<std::string> vec;
      StringUtil::Split(UrlUtil::UrlDecode(req.body), "=", &vec);
      std::string code = vec[1]; */
      std::unordered_map<std::string, std::string> body_kv;
      UrlUtil::PraseBody(req.body, &body_kv);
      
      // 3.获取完整源文件code + tail.cpp ---> src
      std::string src = body_kv["code"] + ques.tail_cpp_;

      /*
       * 构造JSON对象
       */ 
      Json::Value req_json;
      req_json["code"] = src;
      req_json["stdin"] = "";
      //std::cout << req_json["code"].asString() << std::endl;
      
      Json::Value resp_json;
      Compiler::CompileAndRun(req_json, &resp_json);

      
      // 获取的返回结果都在resp_json中
      std::string case_result = resp_json["stdout"].asString();
      std::string reason = resp_json["reason"].asString();
      
      std::string html;
      OJView::DawCaseResult(case_result, reason, &html);

      resp.set_content(html, "text/html");
      });

  // LOG(INFO, "17878") << ":" << std::endl;
  svr.set_base_dir("./www");
  svr.listen("0.0.0.0",18989);
  return 0;
}
