#pragma once 
#include <iostream>
#include <vector>
#include "ctemplate/template.h"
#include "oj_model.hpp"
// 该文件用于填充模板

class OJView
{
  public:
    static void DrawAllQuestions(std::vector<struct Question>& questions, std::string* html)
    {
      // 1.创建template字典
      ctemplate::TemplateDictionary dict("all_questions");
      // 2.遍历vector，每一个试题构造成一个字典
      for(auto& ques : questions)
      {
        ctemplate::TemplateDictionary* sub_dict = dict.AddSectionDictionary("question");
        sub_dict->SetValue("id", ques.id_);
        sub_dict->SetValue("id",ques.id_);
        sub_dict->SetValue("title",ques.title_);
        sub_dict->SetValue("star",ques.star_);
      }
      // 3.填充
      ctemplate::Template* tl = ctemplate::Template::GetTemplate("./template/all_questions.html", ctemplate::DO_NOT_STRIP);
      // 4.渲染
      tl->Expand(html, &dict);
    }
    
    static void DrawOneQuestion(const Question& ques, std::string* html)
    {
      ctemplate::TemplateDictionary dict("question");
      dict.SetValue("id", ques.id_);
      dict.SetValue("title", ques.title_);
      dict.SetValue("star", ques.star_);
      dict.SetValue("desc", ques.desc_);
      dict.SetValue("id", ques.id_);
      dict.SetValue("code", ques.header_cpp_);
      // 填充
      ctemplate::Template* tl = ctemplate::Template::GetTemplate("./template/question.html", ctemplate::DO_NOT_STRIP);
      // 渲染
      tl->Expand(html, &dict);
    }

    static void DawCaseResult(const std::string& q_result, const std::string& reason, std::string* html)
    {
      ctemplate::TemplateDictionary dict("question");
      dict.SetValue("compile_result", reason);
      dict.SetValue("case_result", q_result);
      ctemplate::Template* tl = ctemplate::Template::GetTemplate("./template/case_result.html", ctemplate::DO_NOT_STRIP);
      // 渲染
      tl->Expand(html, &dict);

    }

};//class end
