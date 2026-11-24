#pragma once
// Win32 visual form designer. The generated code is sent to Studio for confirmation;
// the designer never writes files or runs generated code on its own.
#include "studio_designer15.hpp"
#include "studio_living17.hpp"
#include "studio_designer19.hpp"
#include <windows.h>
#include <windowsx.h>
#include <algorithm>
#include <cstddef>
#include <memory>
#include <string>

namespace nova_designer15 {
static constexpr int ADD_LABEL=6101,ADD_INPUT=6102,ADD_BUTTON=6103,
   ADD_CHECK=6104,ADD_TEXTAREA=6105,REMOVE=6106,EXPORT=6107,
   CAPTION=6108,APPLY=6109,TITLE=6110,WIDTH_PLUS=6111,WIDTH_MINUS=6112,
   HEIGHT_PLUS=6113,HEIGHT_MINUS=6114, GRID=6115,PREVIEW=6116,
   PROP_X=6117,PROP_Y=6118,PROP_W=6119,PROP_H=6120,APPLY_PROPS=6121,
   TEMPLATE_LOGIN=6122,TEMPLATE_FEEDBACK=6123,TEMPLATE_DASHBOARD=6124,
   ID_DESIGNER_CENTER_X=6125,ID_DESIGNER_CENTER_Y=6126,ID_DESIGNER_DUPLICATE=6127;
class DesignerWindow {
  HWND owner=nullptr,window=nullptr,caption=nullptr,title=nullptr,summary=nullptr;
  HWND propX=nullptr,propY=nullptr,propW=nullptr,propH=nullptr,preview=nullptr;
  Form form;
  std::uint64_t sourceDocumentId=0;
  std::wstring sourceSnapshot;
  int selected=-1,dragLastX=0,dragLastY=0;
  bool dragging=false,resizing=false,gridEnabled=true,autoDelete=false;
  static constexpr int canvasX=32,canvasY=158;
  static constexpr double scale=0.78;
  static constexpr UINT WM_EXPORT_FORM=WM_APP+79;
  static int screenX(int logical){return canvasX+(int)(logical*scale+0.5);}
  static int screenY(int logical){return canvasY+(int)(logical*scale+0.5);}
  HWND button(LPCWSTR text,int id,int x,int y,int w){
    return CreateWindowW(L"BUTTON",text,WS_VISIBLE|WS_CHILD|WS_TABSTOP|BS_PUSHBUTTON,
      x,y,w,31,window,(HMENU)(INT_PTR)id,GetModuleHandleW(nullptr),nullptr);
  }
  HWND edit(int id,int x,int y,int w,const wchar_t* value){
    return CreateWindowExW(WS_EX_CLIENTEDGE,L"EDIT",value,WS_VISIBLE|WS_CHILD|ES_AUTOHSCROLL,
      x,y,w,30,window,(HMENU)(INT_PTR)id,GetModuleHandleW(nullptr),nullptr);
  }
  static void setNumber(HWND edit,int number){
    if(edit)SetWindowTextW(edit,std::to_wstring(number).c_str());
  }
  void info(){
    std::wstring s=selected<0?L"Choose a widget or add one above":
      L"Selected: "+std::wstring(kindName(form.widgets[(size_t)selected].kind))+
      L"  |  drag to move, +/- to resize";
    if(summary)SetWindowTextW(summary,s.c_str());
    if(caption){SetWindowTextW(caption,selected<0?L"":form.widgets[(size_t)selected].caption.c_str());
      EnableWindow(caption,selected>=0);}
    if(selected>=0){const auto& w=form.widgets[(size_t)selected];
      setNumber(propX,w.x);setNumber(propY,w.y);setNumber(propW,w.w);setNumber(propH,w.h);}
    else{for(HWND box:{propX,propY,propW,propH})if(box)SetWindowTextW(box,L"");}
    for(HWND box:{propX,propY,propW,propH})if(box)EnableWindow(box,selected>=0);
  }
  RECT rect(const Widget& widget)const{
    return {screenX(widget.x),screenY(widget.y),screenX(widget.x+widget.w),screenY(widget.y+widget.h)};
  }
  void draw(HWND hwnd){
    PAINTSTRUCT ps{};HDC dc=BeginPaint(hwnd,&ps);
    RECT background{};GetClientRect(hwnd,&background);
    HBRUSH dark=CreateSolidBrush(RGB(20,26,39));FillRect(dc,&background,dark);DeleteObject(dark);
    RECT mainFrame{canvasX-8,canvasY-8,screenX(form.width)+8,screenY(form.height)+8};
    HBRUSH frame=CreateSolidBrush(RGB(215,223,235));FillRect(dc,&mainFrame,frame);DeleteObject(frame);
    RECT inside{canvasX,canvasY,screenX(form.width),screenY(form.height)};
    HBRUSH white=CreateSolidBrush(RGB(251,252,255));FillRect(dc,&inside,white);DeleteObject(white);
    if(gridEnabled){
      HPEN pen=CreatePen(PS_SOLID,1,RGB(235,239,246));HGDIOBJ old=SelectObject(dc,pen);
      for(int x=10;x<form.width;x+=10){int sx=screenX(x);
        MoveToEx(dc,sx,inside.top,nullptr);LineTo(dc,sx,inside.bottom);}
      for(int y=10;y<form.height;y+=10){int sy=screenY(y);
        MoveToEx(dc,inside.left,sy,nullptr);LineTo(dc,inside.right,sy);}
      SelectObject(dc,old);DeleteObject(pen);
    }
    SetBkMode(dc,TRANSPARENT);
    for(size_t i=0;i<form.widgets.size();++i){
      const auto& v=form.widgets[i];RECT r=rect(v);
      const bool chosen=(int)i==selected;
      HBRUSH fill=CreateSolidBrush(v.kind==Kind::Button?RGB(78,128,222):
                                  v.kind==Kind::Input||v.kind==Kind::Textarea?RGB(241,244,250):
                                  RGB(252,252,255));
      FillRect(dc,&r,fill);DeleteObject(fill);
      HPEN pen=CreatePen(PS_SOLID,chosen?3:1,chosen?RGB(242,131,39):RGB(148,162,185));
      HGDIOBJ oldPen=SelectObject(dc,pen);
      MoveToEx(dc,r.left,r.top,nullptr);LineTo(dc,r.right,r.top);LineTo(dc,r.right,r.bottom);
      LineTo(dc,r.left,r.bottom);LineTo(dc,r.left,r.top);
      SelectObject(dc,oldPen);DeleteObject(pen);
      RECT text=r;text.left+=7;text.right-=5;
      SetTextColor(dc,v.kind==Kind::Button?RGB(255,255,255):RGB(25,39,57));
      std::wstring label=v.kind==Kind::Checkbox?L"[ ] "+v.caption:v.caption;
      DrawTextW(dc,label.c_str(),-1,&text,DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS);
      if(chosen){RECT handle{r.right-8,r.bottom-8,r.right+2,r.bottom+2};
        HBRUSH grip=CreateSolidBrush(RGB(242,131,39));FillRect(dc,&handle,grip);DeleteObject(grip);}
    }
    RECT hint{30,650,700,690};SetTextColor(dc,RGB(190,205,229));
    DrawTextW(dc,L"Preview: move widgets with the mouse. Adjust text and size on the right.\nExport creates a new .nova tab or proposes an edit to the current form (never auto-saved).",-1,&hint,DT_WORDBREAK);
    EndPaint(hwnd,&ps);
  }
  void clickAt(int x,int y){
    selected=-1;
    for(size_t i=form.widgets.size();i>0;--i){
      RECT r=rect(form.widgets[i-1]);
      if(x>=r.left&&x<r.right&&y>=r.top&&y<r.bottom){selected=(int)(i-1);break;}
    }
    dragging=selected>=0;resizing=false;dragLastX=x;dragLastY=y;
    if(selected>=0){RECT r=rect(form.widgets[(size_t)selected]);
      resizing=x>=r.right-12&&y>=r.bottom-12;}

    if(dragging)SetCapture(window);
    info();InvalidateRect(window,nullptr,FALSE);
  }
  void moveAt(int x,int y){
    if(!dragging||selected<0)return;
    auto& w=form.widgets[(size_t)selected];
    const int dx=(int)((x-dragLastX)/scale);
    const int dy=(int)((y-dragLastY)/scale);
    if(dx||dy){
      if(resizing)nova_designer19::resize(form,(size_t)selected,w.w+dx,w.h+dy,gridEnabled);
      else nova_designer19::move(form,(size_t)selected,w.x+dx,w.y+dy,gridEnabled);
      dragLastX=x;dragLastY=y;info();InvalidateRect(window,nullptr,FALSE);
    }
  }
  void add(Kind kind){
    try{form.add(kind);selected=(int)form.widgets.size()-1;info();InvalidateRect(window,nullptr,FALSE);}
    catch(const std::exception&){MessageBoxW(window,L"Maximum 32 form widgets.",L"Nova Designer",MB_ICONWARNING);}
  }
  void applyProperties(){
    if(selected<0)return;
    auto parse=[&](HWND box)->int{
      wchar_t buffer[32]{};GetWindowTextW(box,buffer,32);
      if(!buffer[0])throw std::invalid_argument("Missing property");
      wchar_t* end=nullptr;const long value=wcstol(buffer,&end,10);
      if(*end||value<0||value>1500)throw std::invalid_argument("Invalid property");
      return (int)value;
    };
    try{
      const int x=parse(propX),y=parse(propY),w=parse(propW),h=parse(propH);
      form.resize((size_t)selected,w,h);
      form.move((size_t)selected,x,y);
      info();InvalidateRect(window,nullptr,FALSE);
    }catch(...){MessageBoxW(window,L"Enter valid numeric X / Y / Width / Height values.",L"Widget properties",MB_ICONWARNING);}
  }
  void showPreview(){
    if(preview&&IsWindow(preview)){SetForegroundWindow(preview);return;}
    try{
      wchar_t name[256]{};GetWindowTextW(title,name,256);
      if(name[0])form.title=name;
      form.validate();
      WNDCLASSW previewClass{};previewClass.hInstance=GetModuleHandleW(nullptr);
      previewClass.lpfnWndProc=DefWindowProcW;
      previewClass.hCursor=LoadCursorW(nullptr,MAKEINTRESOURCEW(32512));
      previewClass.hbrBackground=(HBRUSH)(COLOR_WINDOW+1);
      previewClass.lpszClassName=L"NovaStudioNext19FormPreview";
      if(!RegisterClassW(&previewClass)&&GetLastError()!=ERROR_CLASS_ALREADY_EXISTS)
        throw std::runtime_error("Cannot register preview window");
      preview=CreateWindowW(previewClass.lpszClassName,form.title.c_str(),WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,CW_USEDEFAULT,form.width+40,form.height+80,window,nullptr,GetModuleHandleW(nullptr),nullptr);
      if(!preview)throw std::runtime_error("No preview window");
      for(const auto& item:form.widgets){
        LPCWSTR klass=L"STATIC";DWORD style=WS_VISIBLE|WS_CHILD;
        switch(item.kind){
          case Kind::Label:break;
          case Kind::Input:klass=L"EDIT";style|=WS_BORDER|ES_AUTOHSCROLL;break;
          case Kind::Textarea:klass=L"EDIT";style|=WS_BORDER|ES_MULTILINE|ES_AUTOVSCROLL|WS_VSCROLL;break;
          case Kind::Button:klass=L"BUTTON";style|=BS_PUSHBUTTON;break;
          case Kind::Checkbox:klass=L"BUTTON";style|=BS_AUTOCHECKBOX;break;
        }
        // No callbacks: preview is a safe visual representation, not code execution.
        CreateWindowW(klass,item.caption.c_str(),style,item.x+12,item.y+12,item.w,item.h,
                      preview,nullptr,GetModuleHandleW(nullptr),nullptr);
      }
      ShowWindow(preview,SW_SHOW);
    }catch(...){MessageBoxW(window,L"Cannot preview this form.",L"Nova Designer",MB_ICONERROR);}
  }
  void loadTemplate(nova_designer19::Template kind){
    if(MessageBoxW(window,L"Replace the current visual layout with a starter template?\n"
      L"Unsaved designer changes will be lost.",
      L"Nova designer templates",MB_YESNO|MB_ICONQUESTION)!=IDYES)return;
    form=nova_designer19::starter(kind);selected=-1;
    SetWindowTextW(title,form.title.c_str());info();InvalidateRect(window,nullptr,FALSE);
  }
  void adjust(int dw,int dh){
    if(selected<0)return;
    auto& w=form.widgets[(size_t)selected];nova_designer19::resize(form,(size_t)selected,w.w+dw,w.h+dh,gridEnabled);
    InvalidateRect(window,nullptr,FALSE);
  }
  void exportCode(){
    try{
      wchar_t titleValue[256]{};GetWindowTextW(title,titleValue,256);
      form.title=titleValue;
      if(form.title.empty())throw std::invalid_argument("Window title is required");
      // form.toNova() is embedded in the canonical editable designer export.
      std::wstring value=nova_living17::exportEditable(form);
      if(MessageBoxW(window,sourceDocumentId?L"Propose replacing the ORIGINAL designer-generated code?\nNova Studio will check for unsaved edits and ask again before applying.":L"Create a NEW unsaved Nova tab with this generated window?\nYou can review it before Save, Run or Build EXE.",
                     L"Review generated application",MB_ICONQUESTION|MB_YESNO)!=IDYES)return;
      if(sourceDocumentId){
        auto* change=new nova_living17::DesignerChange{sourceDocumentId,sourceSnapshot,std::move(value)};
        if(!PostMessageW(owner,WM_APP+81,0,(LPARAM)change)){
          delete change;MessageBoxW(window,L"Nova Studio is no longer available.",L"Nova Designer",MB_ICONERROR);
        }
      }else{
        auto* code=new std::wstring(std::move(value));
        if(!PostMessageW(owner,WM_EXPORT_FORM,0,(LPARAM)code)){
          delete code;MessageBoxW(window,L"Nova Studio is no longer available.",L"Nova Designer",MB_ICONERROR);
        }
      }
    }catch(const std::exception&){MessageBoxW(window,L"Invalid title or a control is outside the form.",L"Nova Designer",MB_ICONERROR);}
  }
  LRESULT message(HWND hwnd,UINT msg,WPARAM w,LPARAM l){
    switch(msg){
      case WM_CREATE:{
        window=hwnd;
        CreateWindowW(L"STATIC",L"NOVA APP DESIGNER  |  Visual layout to real Nova code",
                      WS_VISIBLE|WS_CHILD,32,20,710,24,hwnd,nullptr,GetModuleHandleW(nullptr),nullptr);
        int x=32;
        const struct {LPCWSTR text;int id;int width;} actions[]={
          {L"+ Label",ADD_LABEL,99},{L"+ Input",ADD_INPUT,99},{L"+ Button",ADD_BUTTON,99},
          {L"+ Checkbox",ADD_CHECK,115},{L"+ Textarea",ADD_TEXTAREA,120},{L"Remove",REMOVE,88}};
        for(const auto& a:actions){button(a.text,a.id,x,70,a.width);x+=a.width+8;}
        button(L"Login layout",TEMPLATE_LOGIN,32,108,120);
        button(L"Feedback",TEMPLATE_FEEDBACK,160,108,115);
        button(L"Dashboard",TEMPLATE_DASHBOARD,283,108,115);
        button(L"Snap: ON",GRID,409,108,104);
        button(L"Live preview",PREVIEW,521,108,145);
        CreateWindowW(L"STATIC",L"WINDOW TITLE",WS_VISIBLE|WS_CHILD,725,110,235,25,
                      hwnd,nullptr,GetModuleHandleW(nullptr),nullptr);
        title=edit(TITLE,725,138,275,form.title.c_str());
        CreateWindowW(L"STATIC",L"SELECTED WIDGET TEXT",WS_VISIBLE|WS_CHILD,725,195,235,25,
                      hwnd,nullptr,GetModuleHandleW(nullptr),nullptr);
        caption=edit(CAPTION,725,225,275,L"");
        button(L"Apply text",APPLY,725,264,130);
        CreateWindowW(L"STATIC",L"POSITION  (X / Y)",WS_VISIBLE|WS_CHILD,725,306,275,22,
                      hwnd,nullptr,GetModuleHandleW(nullptr),nullptr);
        propX=edit(PROP_X,725,329,126,L"");propY=edit(PROP_Y,874,329,126,L"");
        CreateWindowW(L"STATIC",L"SIZE  (WIDTH / HEIGHT)",WS_VISIBLE|WS_CHILD,725,375,275,22,
                      hwnd,nullptr,GetModuleHandleW(nullptr),nullptr);
        propW=edit(PROP_W,725,398,126,L"");propH=edit(PROP_H,874,398,126,L"");
        button(L"Apply position / size",APPLY_PROPS,725,439,275);
        summary=CreateWindowW(L"STATIC",L"",WS_VISIBLE|WS_CHILD|SS_LEFT,
                              725,484,275,42,hwnd,nullptr,GetModuleHandleW(nullptr),nullptr);
        button(L"Center X",ID_DESIGNER_CENTER_X,725,540,85);
        button(L"Center Y",ID_DESIGNER_CENTER_Y,815,540,85);
        button(L"Duplicate",ID_DESIGNER_DUPLICATE,905,540,95);
        button(L"GENERATE NOVA CODE",EXPORT,725,590,275);
        info();return 0;
      }
      case WM_LBUTTONDOWN:clickAt(GET_X_LPARAM(l),GET_Y_LPARAM(l));return 0;
      case WM_MOUSEMOVE:moveAt(GET_X_LPARAM(l),GET_Y_LPARAM(l));return 0;
      case WM_LBUTTONUP:if(dragging){dragging=false;resizing=false;ReleaseCapture();}return 0;
      case WM_COMMAND:{
        switch(LOWORD(w)){
          case ADD_LABEL:add(Kind::Label);break;
          case ADD_INPUT:add(Kind::Input);break;
          case ADD_BUTTON:add(Kind::Button);break;
          case ADD_CHECK:add(Kind::Checkbox);break;
          case ADD_TEXTAREA:add(Kind::Textarea);break;
          case REMOVE:if(selected>=0){form.erase((size_t)selected);selected=-1;info();InvalidateRect(window,nullptr,FALSE);}break;
          case APPLY:if(selected>=0){wchar_t buffer[512]{};GetWindowTextW(caption,buffer,512);
            form.widgets[(size_t)selected].caption=buffer;InvalidateRect(window,nullptr,FALSE);}break;
          case APPLY_PROPS:applyProperties();break;
          case ID_DESIGNER_CENTER_X:if(selected>=0){nova_designer19::centerHorizontally(form,(size_t)selected);
            info();InvalidateRect(window,nullptr,FALSE);}break;
          case ID_DESIGNER_CENTER_Y:if(selected>=0){nova_designer19::centerVertically(form,(size_t)selected);
            info();InvalidateRect(window,nullptr,FALSE);}break;
          case ID_DESIGNER_DUPLICATE:if(selected>=0){try{selected=(int)nova_designer19::duplicate(form,(size_t)selected);
            info();InvalidateRect(window,nullptr,FALSE);}catch(const std::exception&){
            MessageBoxW(window,L"Cannot duplicate: limit of 32 widgets reached.",L"Nova Designer",MB_ICONWARNING);}}break;
          case GRID:gridEnabled=!gridEnabled;
            SetWindowTextW(GetDlgItem(window,GRID),gridEnabled?L"Snap: ON":L"Snap: OFF");
            InvalidateRect(window,nullptr,FALSE);break;
          case PREVIEW:showPreview();break;
          case TEMPLATE_LOGIN:loadTemplate(nova_designer19::Template::Login);break;
          case TEMPLATE_FEEDBACK:loadTemplate(nova_designer19::Template::Feedback);break;
          case TEMPLATE_DASHBOARD:loadTemplate(nova_designer19::Template::Dashboard);break;
          case WIDTH_PLUS:adjust(20,0);break;
          case WIDTH_MINUS:adjust(-20,0);break;
          case HEIGHT_PLUS:adjust(0,15);break;
          case HEIGHT_MINUS:adjust(0,-15);break;
          case EXPORT:exportCode();break;
        }return 0;
      }
      case WM_PAINT:draw(hwnd);return 0;
      case WM_ERASEBKGND:return 1;
      case WM_CLOSE:if(preview&&IsWindow(preview))DestroyWindow(preview);
        DestroyWindow(hwnd);return 0;
      case WM_NCDESTROY:SetWindowLongPtrW(hwnd,GWLP_USERDATA,0);if(autoDelete)delete this;
         return DefWindowProcW(hwnd,msg,w,l);
    }
    return DefWindowProcW(hwnd,msg,w,l);
  }
  static LRESULT CALLBACK proc(HWND hwnd,UINT msg,WPARAM w,LPARAM l){
    auto* self=(DesignerWindow*)GetWindowLongPtrW(hwnd,GWLP_USERDATA);
    if(msg==WM_NCCREATE){self=(DesignerWindow*)((CREATESTRUCTW*)l)->lpCreateParams;
      SetWindowLongPtrW(hwnd,GWLP_USERDATA,(LONG_PTR)self);}
    return self?self->message(hwnd,msg,w,l):DefWindowProcW(hwnd,msg,w,l);
  }
public:
  explicit DesignerWindow(HWND parent,const std::optional<Form>& existing=std::nullopt,
                          std::uint64_t documentId=0,std::wstring snapshot={})
    :owner(parent),sourceDocumentId(documentId),sourceSnapshot(std::move(snapshot)){
    if(existing)form=*existing;
    else{form.add(Kind::Label);form.add(Kind::Input);form.add(Kind::Button);}
  }
  static void show(HWND owner,const std::wstring& existingCode={},std::uint64_t documentId=0){
    std::optional<Form> existing;
    if(documentId){
      existing=nova_living17::importEditable(existingCode);
      if(!existing){MessageBoxW(owner,L"This source is not an unchanged designer-generated form.\nManual edits are protected. Create a new design instead.",
                            L"Nova Living UI",MB_ICONWARNING);return;}
    }
    static const wchar_t* name=L"NovaStudioNextVisualDesigner15";
    WNDCLASSW wc{};wc.hInstance=GetModuleHandleW(nullptr);wc.lpfnWndProc=proc;
    wc.hCursor=LoadCursorW(nullptr,MAKEINTRESOURCEW(32512));
    wc.lpszClassName=name;wc.hbrBackground=(HBRUSH)(COLOR_WINDOW+1);
    if(!RegisterClassW(&wc)&&GetLastError()!=ERROR_CLASS_ALREADY_EXISTS){
      MessageBoxW(owner,L"Cannot register designer window.",L"Nova Designer",MB_ICONERROR);return;}
    auto* self=new DesignerWindow(owner,existing,documentId,existingCode);
    HWND hwnd=CreateWindowW(name,L"Nova App Designer | Preview 19",
            WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX,
            CW_USEDEFAULT,CW_USEDEFAULT,1040,735,owner,nullptr,GetModuleHandleW(nullptr),self);
    if(!hwnd){delete self;MessageBoxW(owner,L"Cannot create visual designer.",L"Nova Designer",MB_ICONERROR);return;}
    self->autoDelete=true;
    ShowWindow(hwnd,SW_SHOW);UpdateWindow(hwnd);
  }
};
} // namespace nova_designer15
