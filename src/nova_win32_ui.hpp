// Nova Native 0.4: Win32 controls, layout and common file dialogs (no Python/Tk).
#pragma once
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <commdlg.h>
#include <functional>
#include <exception>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>
#include <algorithm>

inline std::wstring novaToWide(const std::string& utf8) {
  if(utf8.empty()) return L"";
  int count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,utf8.data(),(int)utf8.size(),nullptr,0);
  if(count<=0) throw std::runtime_error("Invalid UTF-8 string");
  std::wstring result(count,L'\0');
  MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,utf8.data(),(int)utf8.size(),&result[0],count);
  return result;
}
inline std::string novaToUtf8(const std::wstring& text) {
  if(text.empty())return "";
  int count=WideCharToMultiByte(CP_UTF8,0,text.data(),(int)text.size(),nullptr,0,nullptr,nullptr);
  if(count<=0)throw std::runtime_error("Unable to encode UTF-8");
  std::string out(count,'\0');WideCharToMultiByte(CP_UTF8,0,text.data(),(int)text.size(),&out[0],count,nullptr,nullptr);
  return out;
}
class NovaWin32Ui {
  enum class Type {Window,Label,Button,Input,Textarea,Checkbox,Select,MenuItem};
  struct Control {HWND handle=nullptr;Type type=Type::Label;std::string callback;};
  std::unordered_map<int,Control> controls;
  std::unordered_map<std::string,HMENU> menus;
  HWND mainWindow=nullptr;
  HMENU mainMenu=nullptr;
  int nextId=100,nextY=20;
  std::function<void(const std::string&)> runCallback;
  std::exception_ptr callbackError;

  static LRESULT CALLBACK windowProc(HWND h,UINT m,WPARAM w,LPARAM l) {
    NovaWin32Ui* self=reinterpret_cast<NovaWin32Ui*>(GetWindowLongPtrW(h,GWLP_USERDATA));
    if(m==WM_NCCREATE){auto* cs=reinterpret_cast<CREATESTRUCTW*>(l);self=static_cast<NovaWin32Ui*>(cs->lpCreateParams);SetWindowLongPtrW(h,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(self));}
    if(self){
      if(m==WM_COMMAND&&(HIWORD(w)==BN_CLICKED||HIWORD(w)==0)){
        auto it=self->controls.find((int)LOWORD(w));
        if(it!=self->controls.end()&&!it->second.callback.empty()&&self->runCallback){
          try{self->runCallback(it->second.callback);}catch(...){self->callbackError=std::current_exception();PostMessageW(h,WM_CLOSE,0,0);}return 0;
        }
      }
      if(m==WM_DESTROY){self->mainWindow=nullptr;self->controls.clear();self->menus.clear();self->mainMenu=nullptr;PostQuitMessage(0);return 0;}
    }
    return DefWindowProcW(h,m,w,l);
  }
  Control& lookup(int id) {
    auto it=controls.find(id);
    if(it==controls.end()||it->second.type==Type::MenuItem||!IsWindow(it->second.handle))throw std::runtime_error("Invalid or closed UI control");
    return it->second;
  }
  int add(const wchar_t* cls,DWORD style,DWORD exStyle,const std::string& caption,Type type,int height) {
    if(!mainWindow||!IsWindow(mainWindow))throw std::runtime_error("Create a window before adding controls");
    int id=nextId++;RECT area{};GetClientRect(mainWindow,&area);
    int width=std::max<int>(120, area.right-40);
    auto captionW=novaToWide(caption);
    HWND h=CreateWindowExW(exStyle,cls,captionW.c_str(),WS_CHILD|WS_VISIBLE|style,
                            20,nextY,width,height,mainWindow,(HMENU)(INT_PTR)id,GetModuleHandleW(nullptr),nullptr);
    if(!h)throw std::runtime_error("Win32: unable to create control");
    SendMessageW(h,WM_SETFONT,(WPARAM)GetStockObject(DEFAULT_GUI_FONT),TRUE);
    nextY+=height+10;controls[id]={h,type,""};return id;
  }
public:
  int createWindow(const std::string& title,int width,int height) {
    if(mainWindow&&IsWindow(mainWindow))throw std::runtime_error("Only one Nova window is supported in Native 0.4");
    if(width<200||width>3000||height<120||height>2200)throw std::runtime_error("Window size out of range");
    WNDCLASSW wc{};wc.lpfnWndProc=windowProc;wc.hInstance=GetModuleHandleW(nullptr);
    wc.hCursor=LoadCursorW(nullptr,MAKEINTRESOURCEW(32512));wc.hbrBackground=(HBRUSH)(COLOR_WINDOW+1);
    wc.lpszClassName=L"NovaNativeWindow03";
    if(!RegisterClassW(&wc)&&GetLastError()!=ERROR_CLASS_ALREADY_EXISTS)throw std::runtime_error("Win32: cannot register window class");
    auto caption=novaToWide(title);
    mainWindow=CreateWindowExW(0,wc.lpszClassName,caption.c_str(),WS_OVERLAPPEDWINDOW,
                               CW_USEDEFAULT,CW_USEDEFAULT,width,height,nullptr,nullptr,wc.hInstance,this);
    if(!mainWindow)throw std::runtime_error("Win32: unable to create window");
    nextY=20;nextId=100;controls[1]={mainWindow,Type::Window,""};return 1;
  }
  void requireWindow(int id){if(id!=1||!mainWindow||!IsWindow(mainWindow))throw std::runtime_error("Expected a Nova window");}
  int label(int parent,const std::string& text){requireWindow(parent);return add(L"STATIC",SS_LEFT,0,text,Type::Label,28);}
  int button(int parent,const std::string& text,const std::string& callback){requireWindow(parent);int id=add(L"BUTTON",BS_PUSHBUTTON|WS_TABSTOP,0,text,Type::Button,36);controls[id].callback=callback;return id;}
  int input(int parent,const std::string& text){requireWindow(parent);return add(L"EDIT",ES_AUTOHSCROLL|WS_TABSTOP,WS_EX_CLIENTEDGE,text,Type::Input,32);}
  int textarea(int parent,const std::string& text){requireWindow(parent);int id=add(L"EDIT",ES_MULTILINE|ES_AUTOVSCROLL|ES_WANTRETURN|WS_VSCROLL|WS_TABSTOP,WS_EX_CLIENTEDGE,text,Type::Textarea,125);SendMessageW(controls[id].handle,EM_LIMITTEXT,4*1024*1024,0);return id;}
  int checkbox(int parent,const std::string& text,bool checked){requireWindow(parent);int id=add(L"BUTTON",BS_AUTOCHECKBOX|WS_TABSTOP,0,text,Type::Checkbox,28);SendMessageW(controls[id].handle,BM_SETCHECK,checked?BST_CHECKED:BST_UNCHECKED,0);return id;}
  int select(int parent,const std::vector<std::string>& choices){
    requireWindow(parent);int id=add(L"COMBOBOX",CBS_DROPDOWNLIST|WS_VSCROLL|WS_TABSTOP,WS_EX_CLIENTEDGE,"",Type::Select,220);
    HWND h=controls[id].handle;
    for(auto& item:choices){auto w=novaToWide(item);if(SendMessageW(h,CB_ADDSTRING,0,(LPARAM)w.c_str())==CB_ERRSPACE)throw std::runtime_error("Cannot add select option");}
    if(!choices.empty())SendMessageW(h,CB_SETCURSEL,0,0);
    // Combobox reserves its full dropdown height; keep next control directly underneath collapsed height.
    nextY-=220-32;return id;
  }
  int menu(int parent,const std::string& group,const std::string& text,const std::string& callback){
    requireWindow(parent);
    if(!mainMenu){mainMenu=CreateMenu();if(!mainMenu)throw std::runtime_error("Cannot create menu bar");if(!SetMenu(mainWindow,mainMenu))throw std::runtime_error("Cannot attach menu bar");}
    HMENU popup=nullptr;auto it=menus.find(group);
    if(it==menus.end()){
      popup=CreatePopupMenu();if(!popup)throw std::runtime_error("Cannot create menu");auto title=novaToWide(group);
      if(!AppendMenuW(mainMenu,MF_POPUP,(UINT_PTR)popup,title.c_str()))throw std::runtime_error("Cannot add menu");
      menus.emplace(group,popup);
    }else popup=it->second;
    int id=nextId++;auto name=novaToWide(text);
    if(!AppendMenuW(popup,MF_STRING,(UINT_PTR)id,name.c_str()))throw std::runtime_error("Cannot add menu item");
    controls[id]={nullptr,Type::MenuItem,callback};DrawMenuBar(mainWindow);return id;
  }
  void place(int id,int x,int y,int width,int height){
    auto& c=lookup(id);if(c.type==Type::Window)throw std::runtime_error("Cannot place window");
    if(x<0||y<0||width<1||height<1||x>5000||y>5000||width>5000||height>5000)throw std::runtime_error("UI coordinates out of range");
    // A Win32 combobox needs enough total height for its popup choices.
    int controlHeight=c.type==Type::Select?std::max(height+160,190):height;
    if(!MoveWindow(c.handle,x,y,width,controlHeight,TRUE))throw std::runtime_error("Could not position control");
  }
  bool isCheckbox(int id){return lookup(id).type==Type::Checkbox;}
  bool getChecked(int id){auto& c=lookup(id);if(c.type!=Type::Checkbox)throw std::runtime_error("Not a checkbox");return SendMessageW(c.handle,BM_GETCHECK,0,0)==BST_CHECKED;}
  void setChecked(int id,bool checked){auto& c=lookup(id);if(c.type!=Type::Checkbox)throw std::runtime_error("Not a checkbox");SendMessageW(c.handle,BM_SETCHECK,checked?BST_CHECKED:BST_UNCHECKED,0);}
  void setText(int id,const std::string& value){
    auto& c=lookup(id);if(c.type==Type::Window||c.type==Type::Checkbox)throw std::runtime_error("Cannot set text on this control");
    if(c.type==Type::Select){auto w=novaToWide(value);auto i=SendMessageW(c.handle,CB_FINDSTRINGEXACT,(WPARAM)-1,(LPARAM)w.c_str());if(i==CB_ERR)throw std::runtime_error("Select option not found");SendMessageW(c.handle,CB_SETCURSEL,(WPARAM)i,0);return;}
    auto wide=novaToWide(value);if(!SetWindowTextW(c.handle,wide.c_str()))throw std::runtime_error("Unable to set control text");
  }
  std::string getText(int id){
    auto& c=lookup(id);if(c.type==Type::Checkbox)throw std::runtime_error("Checkbox get uses boolean");
    if(c.type==Type::Select){int ix=(int)SendMessageW(c.handle,CB_GETCURSEL,0,0);if(ix==CB_ERR)return "";int n=(int)SendMessageW(c.handle,CB_GETLBTEXTLEN,(WPARAM)ix,0);if(n<0||n>4*1024*1024)throw std::runtime_error("Select text too large");std::vector<wchar_t> b((size_t)n+1,L'\0');SendMessageW(c.handle,CB_GETLBTEXT,(WPARAM)ix,(LPARAM)b.data());return novaToUtf8(std::wstring(b.data()));}
    int n=GetWindowTextLengthW(c.handle);if(n<0||n>4*1024*1024)throw std::runtime_error("UI text too large");
    std::vector<wchar_t> b((size_t)n+1,L'\0');GetWindowTextW(c.handle,b.data(),n+1);return novaToUtf8(std::wstring(b.data()));
  }
  // Returns an empty string when user cancels; runtime turns that into Nova null.
  std::string fileDialog(int parent,bool save,const std::string& title){
    requireWindow(parent);std::vector<wchar_t> path(32768,L'\0');
    static const wchar_t filter[]=L"All files (*.*)\0*.*\0Text files (*.txt)\0*.txt\0Nova (*.nova)\0*.nova\0\0";
    auto name=novaToWide(title);OPENFILENAMEW ofn{};ofn.lStructSize=sizeof(ofn);ofn.hwndOwner=mainWindow;
    ofn.lpstrFilter=filter;ofn.nFilterIndex=1;ofn.lpstrFile=path.data();ofn.nMaxFile=(DWORD)path.size();ofn.lpstrTitle=name.c_str();
    ofn.Flags=OFN_EXPLORER|OFN_NOCHANGEDIR|OFN_PATHMUSTEXIST|(save?OFN_OVERWRITEPROMPT:OFN_FILEMUSTEXIST);
    if(!(save?GetSaveFileNameW(&ofn):GetOpenFileNameW(&ofn))){DWORD err=CommDlgExtendedError();if(err)throw std::runtime_error("Windows file dialog error "+std::to_string(err));return "";}
    return novaToUtf8(std::wstring(path.data()));
  }
  void close(int id){requireWindow(id);PostMessageW(mainWindow,WM_CLOSE,0,0);}
  void run(int id,std::function<void(const std::string&)> callback){
    requireWindow(id);runCallback=std::move(callback);callbackError=nullptr;
    ShowWindow(mainWindow,SW_SHOW);UpdateWindow(mainWindow);
    MSG msg{};int status=0;
    while((status=(int)GetMessageW(&msg,nullptr,0,0))>0){TranslateMessage(&msg);DispatchMessageW(&msg);}
    runCallback={};if(status==-1)throw std::runtime_error("Win32 message loop failed");
    if(callbackError)std::rethrow_exception(callbackError);
  }
  bool alert(const std::string& title,const std::string& message){auto t=novaToWide(title),m=novaToWide(message);return MessageBoxW(mainWindow,m.c_str(),t.c_str(),MB_OK|MB_ICONINFORMATION)==IDOK;}
  bool confirm(const std::string& title,const std::string& message){auto t=novaToWide(title),m=novaToWide(message);return MessageBoxW(mainWindow,m.c_str(),t.c_str(),MB_YESNO|MB_ICONQUESTION)==IDYES;}
};
#endif
