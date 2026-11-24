#pragma once
// Preview 19 designer helpers: snapping and project templates, portable across OSes.
#include "studio_designer15.hpp"
#include <algorithm>
#include <cstddef>
namespace nova_designer19 {
inline int snap(int value,int grid){
  if(grid<=1)return value;
  return value>=0?((value+grid/2)/grid)*grid:((value-grid/2)/grid)*grid;
}
inline void move(nova_designer15::Form& form,size_t index,int x,int y,bool grid=true){
  form.move(index,grid?snap(x,10):x,grid?snap(y,10):y);
}
inline void resize(nova_designer15::Form& form,size_t index,int w,int h,bool grid=true){
  form.resize(index,grid?snap(w,10):w,grid?snap(h,10):h);
}
// Precise alignment and duplication are deterministic and validated by Form.
inline void centerHorizontally(nova_designer15::Form& form,size_t index){
  if(index>=form.widgets.size())throw std::out_of_range("Unknown widget");
  auto& widget=form.widgets[index];form.move(index,(form.width-widget.w)/2,widget.y);
}
inline void centerVertically(nova_designer15::Form& form,size_t index){
  if(index>=form.widgets.size())throw std::out_of_range("Unknown widget");
  auto& widget=form.widgets[index];form.move(index,widget.x,(form.height-widget.h)/2);
}
inline size_t duplicate(nova_designer15::Form& form,size_t index){
  if(index>=form.widgets.size())throw std::out_of_range("Unknown widget");
  if(form.widgets.size()>=32)throw std::length_error("32 widgets maximum");
  const auto source=form.widgets[index];
  form.widgets.push_back(source);
  form.move(form.widgets.size()-1,source.x+20,source.y+20);
  return form.widgets.size()-1;
}
enum class Template { Login, Feedback, Dashboard };
inline nova_designer15::Form starter(Template preset){
  using namespace nova_designer15;
  Form f;
  f.widgets.clear();
  if(preset==Template::Login){
    f.title=L"Nova Login";
    f.add(Kind::Label);f.widgets.back()={Kind::Label,40,35,310,35,L"Welcome back"};
    f.add(Kind::Input);f.widgets.back()={Kind::Input,40,95,330,35,L"Username"};
    f.add(Kind::Input);f.widgets.back()={Kind::Input,40,145,330,35,L"Password"};
    f.add(Kind::Checkbox);f.widgets.back()={Kind::Checkbox,40,195,220,30,L"Remember me"};
    f.add(Kind::Button);f.widgets.back()={Kind::Button,40,245,175,42,L"Sign in"};
  }else if(preset==Template::Feedback){
    f.title=L"Nova Feedback";
    f.add(Kind::Label);f.widgets.back()={Kind::Label,35,30,380,35,L"Send us your feedback"};
    f.add(Kind::Input);f.widgets.back()={Kind::Input,35,85,365,35,L"Your email"};
    f.add(Kind::Textarea);f.widgets.back()={Kind::Textarea,35,142,500,155,L"Your message"};
    f.add(Kind::Button);f.widgets.back()={Kind::Button,35,318,205,42,L"Send feedback"};
  }else{
    f.title=L"Nova Dashboard";
    f.add(Kind::Label);f.widgets.back()={Kind::Label,30,30,440,40,L"Project dashboard"};
    f.add(Kind::Label);f.widgets.back()={Kind::Label,30,102,360,35,L"Total tasks: 0"};
    f.add(Kind::Button);f.widgets.back()={Kind::Button,30,160,175,44,L"Add task"};
    f.add(Kind::Button);f.widgets.back()={Kind::Button,225,160,175,44,L"Refresh"};
    f.add(Kind::Textarea);f.widgets.back()={Kind::Textarea,30,240,490,110,L"Activity log"};
  }
  f.validate();return f;
}
} // namespace nova_designer19
