#include "../studio/studio_designer19.hpp"
#include "../studio/studio_living17.hpp"
#include <cassert>
int main(){
  using namespace nova_designer19;
  assert(snap(23,10)==20&&snap(26,10)==30&&snap(-6,10)==-10);
  for(auto kind:{Template::Login,Template::Feedback,Template::Dashboard}){
    auto form=starter(kind);form.validate();
    assert(nova_living17::importEditable(nova_living17::exportEditable(form)));
    assert(!form.widgets.empty());
    move(form,0,27,37,true);assert(form.widgets[0].x==30&&form.widgets[0].y==40);
    resize(form,0,121,34,true);assert(form.widgets[0].w==120&&form.widgets[0].h==30);
    centerHorizontally(form,0);
    assert(form.widgets[0].x==(form.width-form.widgets[0].w)/2);
    centerVertically(form,0);
    assert(form.widgets[0].y==(form.height-form.widgets[0].h)/2);
    const auto original=form.widgets[0];
    size_t copy=duplicate(form,0);
    assert(copy==form.widgets.size()-1);
    assert(form.widgets[copy].caption==original.caption);
    form.validate();
    try{centerHorizontally(form,999);assert(false);}catch(const std::out_of_range&){}
  }
}
