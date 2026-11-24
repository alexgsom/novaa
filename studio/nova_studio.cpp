// Nova Studio 1.0 Beta 1: Win32 UTF-8 editor, search, highlighting, self-hosted build.
// Requires the prebuilt native Nova runtime and the Nova-authored .nbc compiler.
#define UNICODE
#define _UNICODE
#define NOMINMAX
#include <windows.h>
#include <commdlg.h>
#include <shellapi.h>
#include <richedit.h>
#include "studio_text.hpp"
#include <algorithm>
#include <atomic>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>
namespace fs=std::filesystem;
static constexpr UINT JOB_DONE=WM_APP+77, HIGHLIGHT_TIMER=91;
static constexpr int ID_EDITOR=101,ID_OUTPUT=102,ID_STATUS=103;
static constexpr int ID_NEW=201,ID_OPEN=202,ID_SAVE=203,ID_SAVE_AS=204,ID_CHECK=205,ID_RUN=206,ID_BUILD=207,ID_BUILD_GUI=208,ID_STOP=209,ID_ABOUT=210,ID_FIND=211,ID_REPLACE=212,ID_NEXT=213,ID_PREVIOUS=214;
static constexpr size_t MAX_FILE=4*1024*1024;
static std::wstring toWide(const std::string& s){
  if(s.empty())return L"";
  int n=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,s.data(),(int)s.size(),nullptr,0);
  if(!n)throw std::runtime_error("Invalid UTF-8 encoding");
  std::wstring w((size_t)n,L'\0');MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,s.data(),(int)s.size(),&w[0],n);return w;
}
static std::string toUtf8(const std::wstring& w){
  if(w.empty())return "";
  int n=WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,w.data(),(int)w.size(),nullptr,0,nullptr,nullptr);
  if(!n)throw std::runtime_error("Invalid Unicode data");
  std::string s((size_t)n,'\0');WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,w.data(),(int)w.size(),&s[0],n,nullptr,nullptr);return s;
}
static std::wstring quoted(const std::wstring& v){
  std::wstring out=L"\"";size_t slashes=0;
  for(wchar_t c:v){
    if(c==L'\\'){++slashes;continue;}
    if(c==L'"'){out.append(slashes*2+1,L'\\');out+=c;slashes=0;continue;}
    out.append(slashes,L'\\');slashes=0;out+=c;
  }
  out.append(slashes*2,L'\\');return out+L'"';
}
static std::wstring editorText(HWND h){
  int n=GetWindowTextLengthW(h);if(n<0||(size_t)n>MAX_FILE)throw std::runtime_error("Document too large");
  std::wstring w((size_t)n+1,L'\0');w.resize((size_t)GetWindowTextW(h,&w[0],n+1));return w;
}
struct Result{DWORD code=1;std::wstring text;bool stopped=false;};
class Studio{
  HWND mainWindow=nullptr,editor=nullptr,output=nullptr,status=nullptr;
  HFONT font=nullptr;HACCEL accelerators=nullptr;HMODULE richLibrary=nullptr;
  bool dirty=false,loading=false,busy=false,richEdit=false;
  UINT findMessage=0;HWND findWindow=nullptr;
  FINDREPLACEW searchDialog{};
  wchar_t searchText[256]{}, replaceText[256]{};
  bool searchDown=true,searchCase=false,searchWhole=false;
  std::wstring statusMessage=L"Ready";
  fs::path file,root,runtime,compiler,windowed;
  std::thread task;CRITICAL_SECTION processLock{};HANDLE running=nullptr;
  std::atomic<bool> cancelled{false};
  enum class Action{Check,Run,Build,BuildGui};
  static LRESULT CALLBACK callback(HWND h,UINT m,WPARAM w,LPARAM l){
    Studio* s=reinterpret_cast<Studio*>(GetWindowLongPtrW(h,GWLP_USERDATA));
    if(m==WM_NCCREATE){s=(Studio*)((CREATESTRUCTW*)l)->lpCreateParams;SetWindowLongPtrW(h,GWLP_USERDATA,(LONG_PTR)s);}
    return s?s->onMessage(h,m,w,l):DefWindowProcW(h,m,w,l);
  }
  void error(const std::wstring& s){MessageBoxW(mainWindow,s.c_str(),L"Nova Studio",MB_OK|MB_ICONERROR);}
  void setTitle(){auto name=file.empty()?L"Untitled.nova":file.filename().wstring();
    SetWindowTextW(mainWindow,(name+(dirty?L" *":L"")+L" - Nova Studio 1.0 Beta 1").c_str());}
  void updateStatus(){
    if(!status||!editor)return;
    DWORD a=0,b=0;SendMessageW(editor,EM_GETSEL,(WPARAM)&a,(LPARAM)&b);
    LONG line=(LONG)SendMessageW(editor,EM_LINEFROMCHAR,(WPARAM)a,0);
    LONG start=(LONG)SendMessageW(editor,EM_LINEINDEX,(WPARAM)line,0);
    std::wstring text=statusMessage+L" | Ln "+std::to_wstring(line+1)+L", Col "+std::to_wstring(a-(DWORD)std::max<LONG>(0,start)+1);
    SetWindowTextW(status,text.c_str());
  }
  void setStatus(const std::wstring& s){statusMessage=s;updateStatus();}
  static LONG richIndex(const std::wstring& text,size_t raw){
    // RichEdit counts a CRLF pair as one paragraph separator for selections.
    LONG index=0;
    for(size_t i=0;i<std::min(raw,text.size());++i)
      if(text[i]!=L'\n'||i==0||text[i-1]!=L'\r')++index;
    return index;
  }
  static size_t rawIndex(const std::wstring& text,LONG index){
    if(index<=0)return 0;
    LONG seen=0;
    for(size_t i=0;i<text.size();++i){
      if(text[i]==L'\n'&&i>0&&text[i-1]==L'\r')continue;
      if(seen++>=index)return i;
    }
    return text.size();
  }
  void setSelection(const std::wstring& doc,size_t begin,size_t end){
    LONG from=richEdit?richIndex(doc,begin):(LONG)begin;
    LONG until=richEdit?richIndex(doc,end):(LONG)end;
    SendMessageW(editor,EM_SETSEL,(WPARAM)from,(LPARAM)until);
    SendMessageW(editor,EM_SCROLLCARET,0,0);SetFocus(editor);updateStatus();
  }
  void applyColor(COLORREF color){
    CHARFORMAT2W cf{};cf.cbSize=sizeof(cf);cf.dwMask=CFM_COLOR;cf.crTextColor=color;
    SendMessageW(editor,EM_SETCHARFORMAT,SCF_SELECTION,(LPARAM)&cf);
  }
  void highlight(){
    if(!richEdit||loading)return;
    auto doc=editorText(editor);
    // Avoid freezing the UI on large source files; plain text remains editable.
    if(doc.size()>128*1024)return;
    auto tokens=nova_studio::tokenize(doc,3000);
    // Precompute RichEdit positions once: highlighting must stay linear in document size.
    std::vector<LONG> positions(doc.size()+1);
    LONG position=0;
    for(size_t i=0;i<doc.size();++i){
      positions[i]=position;
      if(doc[i]!=L'\n'||i==0||doc[i-1]!=L'\r')++position;
    }
    positions[doc.size()]=position;
    CHARRANGE selection{};SendMessageW(editor,EM_EXGETSEL,0,(LPARAM)&selection);
    auto mask=SendMessageW(editor,EM_SETEVENTMASK,0,0);
    loading=true;
    SendMessageW(editor,EM_SETSEL,0,-1);applyColor(RGB(30,38,53));
    for(const auto& tok:tokens){
      CHARRANGE r{positions[tok.begin],positions[tok.end]};
      SendMessageW(editor,EM_EXSETSEL,0,(LPARAM)&r);
      switch(tok.kind){
        case nova_studio::Kind::Keyword:applyColor(RGB(22,69,166));break;
        case nova_studio::Kind::String:applyColor(RGB(151,65,24));break;
        case nova_studio::Kind::Number:applyColor(RGB(115,59,144));break;
        case nova_studio::Kind::Comment:applyColor(RGB(44,122,69));break;
      }
    }
    SendMessageW(editor,EM_EXSETSEL,0,(LPARAM)&selection);
    SendMessageW(editor,EM_SETEVENTMASK,0,mask);
    loading=false;updateStatus();
  }
  void openSearch(bool replacing){
    if(findWindow){SetForegroundWindow(findWindow);return;}
    searchDialog={};searchDialog.lStructSize=sizeof(searchDialog);
    searchDialog.hwndOwner=mainWindow;searchDialog.lpstrFindWhat=searchText;
    searchDialog.wFindWhatLen=256;searchDialog.lpstrReplaceWith=replaceText;
    searchDialog.wReplaceWithLen=256;
    searchDialog.Flags=(searchDown?FR_DOWN:0)|(searchCase?FR_MATCHCASE:0)|(searchWhole?FR_WHOLEWORD:0);
    findWindow=replacing?ReplaceTextW(&searchDialog):FindTextW(&searchDialog);
    if(!findWindow)error(L"Cannot open Find/Replace dialog");
  }
  bool findOccurrence(bool backwards=false){
    if(!searchText[0])return false;
    auto doc=editorText(editor);DWORD a=0,b=0;
    SendMessageW(editor,EM_GETSEL,(WPARAM)&a,(LPARAM)&b);
    size_t begin=richEdit?rawIndex(doc,(LONG)a):std::min<size_t>(a,doc.size());
    size_t end=richEdit?rawIndex(doc,(LONG)b):std::min<size_t>(b,doc.size());
    // F3 moves past a currently selected hit; Shift+F3 searches backwards.
    auto match=backwards?nova_studio::findPrevious(doc,searchText,begin,searchCase,searchWhole)
                        :nova_studio::findNext(doc,searchText,end,searchCase,searchWhole);
    if(!match){setStatus(L"Text not found");MessageBeep(MB_ICONINFORMATION);return false;}
    setSelection(doc,match.begin,match.end);setStatus(L"Found");return true;
  }
  void onSearchMessage(const FINDREPLACEW* f){
    if(!f)return;
    if(f->Flags&FR_DIALOGTERM){findWindow=nullptr;return;}
    searchCase=!!(f->Flags&FR_MATCHCASE);searchWhole=!!(f->Flags&FR_WHOLEWORD);searchDown=!!(f->Flags&FR_DOWN);
    if(f->Flags&FR_FINDNEXT){findOccurrence(!searchDown);return;}
    if(f->Flags&FR_REPLACEALL){
      try{auto doc=editorText(editor);
        auto result=nova_studio::replaceAll(doc,searchText,replaceText,searchCase,searchWhole,MAX_FILE);
        if(result.count){SetWindowTextW(editor,result.text.c_str());dirty=true;setTitle();
          SetTimer(mainWindow,HIGHLIGHT_TIMER,180,nullptr);}
        setStatus(std::to_wstring(result.count)+L" occurrence(s) replaced");
      }catch(const std::exception& e){error(L"Replace all: "+toWide(e.what()));}
      return;
    }
    if(f->Flags&FR_REPLACE){
      auto doc=editorText(editor);DWORD a=0,b=0;
      SendMessageW(editor,EM_GETSEL,(WPARAM)&a,(LPARAM)&b);
      size_t first=richEdit?rawIndex(doc,(LONG)a):std::min<size_t>(a,doc.size());
      size_t last=richEdit?rawIndex(doc,(LONG)b):std::min<size_t>(b,doc.size());
      if(last>=first && nova_studio::equalAt(doc,searchText,first,searchCase) &&
         last-first==wcslen(searchText) && (!searchWhole||nova_studio::boundary(doc,first,last-first))){
        SendMessageW(editor,EM_REPLACESEL,TRUE,(LPARAM)replaceText);
      }
      findOccurrence(false);
    }
  }
  bool chooseFile(bool saving,fs::path& selected,LPCWSTR title,LPCWSTR filter){
    std::vector<wchar_t> buffer(32768,L'\0');
    if(!selected.empty()){auto name=selected.wstring();if(name.size()<buffer.size())std::copy(name.begin(),name.end(),buffer.begin());}
    OPENFILENAMEW of{};of.lStructSize=sizeof(of);of.hwndOwner=mainWindow;of.lpstrFile=buffer.data();of.nMaxFile=(DWORD)buffer.size();
    of.lpstrFilter=filter;of.lpstrTitle=title;of.Flags=OFN_EXPLORER|OFN_PATHMUSTEXIST|(saving?OFN_OVERWRITEPROMPT:OFN_FILEMUSTEXIST);
    of.lpstrDefExt=(saving&&wcsstr(filter,L"Nova programs"))?L"nova":(saving?L"exe":nullptr);
    if(!(saving?GetSaveFileNameW(&of):GetOpenFileNameW(&of))){if(CommDlgExtendedError())error(L"Could not open the Windows file dialog");return false;}
    selected=fs::path(buffer.data());return true;
  }
  bool saveFile(bool saveAs){
    static const wchar_t filter[]=L"Nova programs (*.nova)\0*.nova\0All files\0*.*\0\0";
    fs::path dest=file;
    if(dest.empty()||saveAs){if(!chooseFile(true,dest,L"Save Nova program",filter))return false;}
    try{
      auto bytes=toUtf8(editorText(editor));if(bytes.size()>MAX_FILE)throw std::runtime_error("Source exceeds size limit");
      fs::path temp=dest;temp+=L".nova-writing";
      if(fs::exists(temp))throw std::runtime_error("Stale temporary file blocks save");
      try{
        {std::ofstream out(temp,std::ios::binary);if(!out)throw std::runtime_error("Cannot create file");
          out.write(bytes.data(),(std::streamsize)bytes.size());if(!out)throw std::runtime_error("Write failed");}
        if(!MoveFileExW(temp.c_str(),dest.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))
          throw std::runtime_error("Windows could not replace the saved file");
      }catch(...){std::error_code ec;fs::remove(temp,ec);throw;}
      file=dest;dirty=false;setTitle();setStatus(L"Saved as UTF-8");return true;
    }catch(const std::exception& e){error(L"Save failed: "+toWide(e.what()));return false;}
  }
  bool discard(){
    if(!dirty)return true;
    int answer=MessageBoxW(mainWindow,L"Save changes?",L"Nova Studio",MB_YESNOCANCEL|MB_ICONQUESTION);
    return answer==IDNO||(answer==IDYES&&saveFile(false));
  }
  void loadFile(const fs::path& target){
    try{
      std::ifstream in(target,std::ios::binary);if(!in)throw std::runtime_error("File not readable");
      std::string bytes((std::istreambuf_iterator<char>(in)),std::istreambuf_iterator<char>());
      if(bytes.size()>MAX_FILE)throw std::runtime_error("File exceeds 4 MiB");
      if(bytes.compare(0,3,"\xef\xbb\xbf")==0)bytes.erase(0,3);
      std::wstring text=toWide(bytes);
      if(text.find(L'\0')!=std::wstring::npos)throw std::runtime_error("NUL bytes are not allowed in source files");
      loading=true;SetWindowTextW(editor,text.c_str());loading=false;
      file=target;dirty=false;setTitle();setStatus(L"Opened UTF-8 source");
      SetTimer(mainWindow,HIGHLIGHT_TIMER,140,nullptr);
    }catch(const std::exception& e){loading=false;error(L"Open failed: "+toWide(e.what()));}
  }
  void openFile(){
    if(!discard())return;
    static const wchar_t filter[]=L"Nova programs (*.nova)\0*.nova\0All files\0*.*\0\0";
    fs::path target;
    if(chooseFile(false,target,L"Open Nova program",filter))loadFile(target);
  }
  void relayout(int w,int h){if(!editor)return;int pad=8,foot=25;int upper=std::max(120,(h-foot-pad*3)*2/3);
    MoveWindow(editor,pad,pad,std::max(100,w-2*pad),upper,TRUE);
    MoveWindow(output,pad,upper+pad*2,std::max(100,w-2*pad),std::max(45,h-foot-upper-pad*3),TRUE);
    MoveWindow(status,pad,std::max(0,h-foot),std::max(100,w-2*pad),foot,TRUE);}
  struct ProcessResult{DWORD exit=1;std::wstring output;};
  ProcessResult spawn(const fs::path& exe,const std::vector<std::wstring>& args,const fs::path& workingDir){
    if(cancelled.load())throw std::runtime_error("Operation cancelled");
    std::wstring line=quoted(exe.wstring());for(auto& a:args)line+=L" "+quoted(a);
    SECURITY_ATTRIBUTES sa{};sa.nLength=sizeof(sa);sa.bInheritHandle=TRUE;
    HANDLE read=nullptr,write=nullptr;
    if(!CreatePipe(&read,&write,&sa,0))throw std::runtime_error("CreatePipe failed");
    SetHandleInformation(read,HANDLE_FLAG_INHERIT,0);
    HANDLE input=CreateFileW(L"NUL",GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,&sa,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(input==INVALID_HANDLE_VALUE){CloseHandle(write);CloseHandle(read);throw std::runtime_error("Cannot open NUL");}
    STARTUPINFOW si{};si.cb=sizeof(si);si.dwFlags=STARTF_USESTDHANDLES;si.hStdInput=input;si.hStdOutput=write;si.hStdError=write;
    PROCESS_INFORMATION pi{};std::vector<wchar_t> cmd(line.begin(),line.end());cmd.push_back(L'\0');
    BOOL started=CreateProcessW(exe.c_str(),cmd.data(),nullptr,nullptr,TRUE,CREATE_NO_WINDOW,nullptr,
                                workingDir.empty()?nullptr:workingDir.c_str(),&si,&pi);
    CloseHandle(input);CloseHandle(write);
    if(!started){DWORD code=GetLastError();CloseHandle(read);throw std::runtime_error("CreateProcess failed: "+std::to_string(code));}
    CloseHandle(pi.hThread);
    EnterCriticalSection(&processLock);running=pi.hProcess;
    if(cancelled.load())TerminateProcess(running,1223);
    LeaveCriticalSection(&processLock);
    std::string outputBytes;char buf[8192];DWORD n=0;
    bool truncated=false;
    while(ReadFile(read,buf,sizeof(buf),&n,nullptr)&&n){
      const size_t remaining=1024*1024-outputBytes.size();
      outputBytes.append(buf,std::min<size_t>(n,remaining));
      if(n>remaining)truncated=true;
    }
    CloseHandle(read);WaitForSingleObject(pi.hProcess,INFINITE);
    ProcessResult result;GetExitCodeProcess(pi.hProcess,&result.exit);
    EnterCriticalSection(&processLock);running=nullptr;LeaveCriticalSection(&processLock);CloseHandle(pi.hProcess);
    try{result.output=toWide(outputBytes);}catch(...){result.output=L"Output was not UTF-8 (or was truncated mid-character)";}
    if(truncated)result.output+=L"\r\n[Output truncated after 1 MiB]\r\n";
    return result;
  }
  void stop(){cancelled=true;EnterCriticalSection(&processLock);if(running)TerminateProcess(running,1223);
    LeaveCriticalSection(&processLock);setStatus(L"Stopping...");}
  void start(Action action){
    if(busy||!saveFile(false))return;
    if(!fs::exists(runtime)||!fs::exists(compiler)){error(L"Nova runtime/compiler missing. Install bin/nova-native.exe and compiler/novac_selfhost.nbc");return;}
    if(action==Action::BuildGui&&!fs::exists(windowed)){error(L"Missing bin/nova-windowed.exe");return;}
    fs::path destination;
    if(action==Action::Build||action==Action::BuildGui){
      static const wchar_t filter[]=L"Windows executables (*.exe)\0*.exe\0\0";
      destination=file;destination.replace_extension(L".exe");
      if(!chooseFile(true,destination,L"Build native EXE",filter))return;
      std::error_code ec;if(fs::exists(destination)&&fs::equivalent(destination,file,ec)&&!ec){error(L"EXE output cannot overwrite source");return;}
    }
    if(task.joinable())task.join();busy=true;cancelled=false;
    SetWindowTextW(output,L"Compiling through Nova self-hosted compiler...\r\n");setStatus(L"Working...");
    const fs::path source=file;
    task=std::thread([this,action,source,destination](){
      std::unique_ptr<Result> result(new Result());fs::path temp;
      try{
        wchar_t tempPath[MAX_PATH+1]{};DWORD len=GetTempPathW(MAX_PATH,tempPath);
        if(len==0||len>MAX_PATH)throw std::runtime_error("No temp folder");
        wchar_t tempName[MAX_PATH+1]{};if(!GetTempFileNameW(tempPath,L"NVC",0,tempName))throw std::runtime_error("No temporary filename");
        temp=tempName;temp+=L".nbc";DeleteFileW(tempName);
        auto build=spawn(runtime,{L"exec",compiler.wstring(),L"--max-steps",L"15000000",L"--",source.wstring(),temp.wstring()},source.parent_path());
        result->code=build.exit;result->text=L"[Compile]\r\n"+build.output;
        if(build.exit==0&&!cancelled){
          if(action==Action::Check)result->text+=L"\r\nBytecode compilation succeeded.\r\n";
          else if(action==Action::Run){auto r=spawn(runtime,{L"exec",temp.wstring()},source.parent_path());
            result->code=r.exit;result->text+=L"\r\n[Run]\r\n"+r.output;}
          else{
            auto runner=action==Action::BuildGui?windowed:runtime;
            // Pack to an .exe in the destination folder, then commit atomically.
            // This allows rebuilding an existing EXE after dialog confirmation while
            // preserving the previous working EXE if compilation or packing fails.
            fs::path staged=destination.parent_path()/
                (destination.stem().wstring()+L".nova-building-"+std::to_wstring(GetCurrentProcessId())+L".exe");
            if(fs::exists(staged))throw std::runtime_error("Stale staged executable blocks build");
            try{
              auto r=spawn(runtime,{L"pack",temp.wstring(),staged.wstring(),L"--runtime",runner.wstring()},source.parent_path());
              result->code=r.exit;result->text+=L"\r\n[Pack]\r\n"+r.output;
              if(r.exit==0&&!cancelled){
                if(!MoveFileExW(staged.c_str(),destination.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))
                  throw std::runtime_error("Windows could not replace the EXE (is it running or locked?)");
                result->text+=L"\r\nCreated "+destination.wstring()+L"\r\n";
              }
            }catch(...){std::error_code ec;fs::remove(staged,ec);throw;}
            std::error_code ec;fs::remove(staged,ec);
          }
        }
      }catch(const std::exception& e){result->code=1;result->text+=L"\r\n"+toWide(e.what());}
      std::error_code ec;if(!temp.empty())fs::remove(temp,ec);
      result->stopped=cancelled.load();if(!PostMessageW(mainWindow,JOB_DONE,0,(LPARAM)result.get()))return;
      result.release();
    });
  }
  LRESULT onMessage(HWND h,UINT m,WPARAM w,LPARAM l){
    if(findMessage && m==findMessage){onSearchMessage((FINDREPLACEW*)l);return 0;}
    if(m==WM_CREATE){
      richLibrary=LoadLibraryW(L"Msftedit.dll");richEdit=richLibrary!=nullptr;
      editor=CreateWindowExW(WS_EX_CLIENTEDGE,richEdit?L"RICHEDIT50W":L"EDIT",L"",
                  WS_VISIBLE|WS_CHILD|WS_VSCROLL|WS_HSCROLL|ES_MULTILINE|ES_AUTOVSCROLL|ES_WANTRETURN|ES_NOHIDESEL,
                  0,0,100,100,h,(HMENU)ID_EDITOR,GetModuleHandleW(nullptr),nullptr);
      if(!editor && richEdit){FreeLibrary(richLibrary);richLibrary=nullptr;richEdit=false;
        editor=CreateWindowExW(WS_EX_CLIENTEDGE,L"EDIT",L"",WS_VISIBLE|WS_CHILD|WS_VSCROLL|ES_MULTILINE|ES_AUTOVSCROLL|ES_WANTRETURN,
                            0,0,100,100,h,(HMENU)ID_EDITOR,GetModuleHandleW(nullptr),nullptr);}
      if(!editor)return -1;
      output=CreateWindowExW(WS_EX_CLIENTEDGE,L"EDIT",L"",WS_VISIBLE|WS_CHILD|WS_VSCROLL|ES_MULTILINE|ES_AUTOVSCROLL|ES_READONLY,
                             0,0,100,100,h,(HMENU)ID_OUTPUT,GetModuleHandleW(nullptr),nullptr);
      status=CreateWindowW(L"STATIC",L"Ready",WS_VISIBLE|WS_CHILD,0,0,100,20,h,(HMENU)ID_STATUS,GetModuleHandleW(nullptr),nullptr);
      font=CreateFontW(-17,0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,FIXED_PITCH,L"Consolas");
      SendMessageW(editor,WM_SETFONT,(WPARAM)font,TRUE);SendMessageW(output,WM_SETFONT,(WPARAM)font,TRUE);
      if(richEdit){SendMessageW(editor,EM_EXLIMITTEXT,0,(LPARAM)MAX_FILE);
        SendMessageW(editor,EM_SETEVENTMASK,0,ENM_CHANGE|ENM_SELCHANGE);
        SendMessageW(editor,EM_SETBKGNDCOLOR,0,RGB(253,253,253));}
      else SendMessageW(editor,EM_LIMITTEXT,(WPARAM)MAX_FILE,0);
      loading=true;SetWindowTextW(editor,L"# Nova Studio 1.0 Beta 1\r\nshow \"Привіт, Nova!\"\r\n");loading=false;dirty=false;
      findMessage=RegisterWindowMessageW(FINDMSGSTRINGW);
      HMENU menu=CreateMenu(),files=CreatePopupMenu(),edit=CreatePopupMenu(),nova=CreatePopupMenu();
      AppendMenuW(files,MF_STRING,ID_NEW,L"New\tCtrl+N");AppendMenuW(files,MF_STRING,ID_OPEN,L"Open\tCtrl+O");
      AppendMenuW(files,MF_STRING,ID_SAVE,L"Save\tCtrl+S");AppendMenuW(files,MF_STRING,ID_SAVE_AS,L"Save as...");
      AppendMenuW(edit,MF_STRING,ID_FIND,L"Find...\tCtrl+F");
      AppendMenuW(edit,MF_STRING,ID_REPLACE,L"Replace...\tCtrl+H");
      AppendMenuW(edit,MF_STRING,ID_NEXT,L"Find next\tF3");
      AppendMenuW(edit,MF_STRING,ID_PREVIOUS,L"Find previous\tShift+F3");
      AppendMenuW(nova,MF_STRING,ID_RUN,L"Run\tF5");AppendMenuW(nova,MF_STRING,ID_CHECK,L"Compile\tF6");
      AppendMenuW(nova,MF_STRING,ID_BUILD,L"Build EXE\tF7");AppendMenuW(nova,MF_STRING,ID_BUILD_GUI,L"Build windowed EXE");
      AppendMenuW(nova,MF_STRING,ID_STOP,L"Stop\tShift+F5");
      AppendMenuW(menu,MF_POPUP,(UINT_PTR)files,L"File");AppendMenuW(menu,MF_POPUP,(UINT_PTR)edit,L"Edit");
      AppendMenuW(menu,MF_POPUP,(UINT_PTR)nova,L"Nova");
      AppendMenuW(menu,MF_STRING,ID_ABOUT,L"About");SetMenu(h,menu);DragAcceptFiles(h,TRUE);setTitle();return 0;
    }
    if(m==WM_SIZE){relayout(LOWORD(l),HIWORD(l));return 0;}
    if(m==WM_COMMAND){
      if(LOWORD(w)==ID_EDITOR&&HIWORD(w)==EN_CHANGE){if(!loading){dirty=true;setTitle();
         SetTimer(h,HIGHLIGHT_TIMER,220,nullptr);updateStatus();}return 0;}
      switch(LOWORD(w)){
        case ID_NEW:if(discard()){file.clear();loading=true;SetWindowTextW(editor,L"");loading=false;dirty=false;setTitle();
          SetTimer(h,HIGHLIGHT_TIMER,140,nullptr);}break;
        case ID_OPEN:openFile();break;case ID_SAVE:saveFile(false);break;case ID_SAVE_AS:saveFile(true);break;
        case ID_FIND:openSearch(false);break;case ID_REPLACE:openSearch(true);break;
        case ID_NEXT:findOccurrence(false);break;case ID_PREVIOUS:findOccurrence(true);break;
        case ID_CHECK:start(Action::Check);break;case ID_RUN:start(Action::Run);break;
        case ID_BUILD:start(Action::Build);break;case ID_BUILD_GUI:start(Action::BuildGui);break;
        case ID_STOP:if(busy)stop();break;
        case ID_ABOUT:MessageBoxW(h,L"Nova Studio 1.0 Beta 1\nNative Win32 editor, search/replace, syntax colors, self-hosted Nova compiler and EXE packaging.\nNo Python runtime required.",L"About",MB_OK);break;
      }return 0;
    }
    if(m==WM_NOTIFY && (HWND)((NMHDR*)l)->hwndFrom==editor && ((NMHDR*)l)->code==EN_SELCHANGE){
      updateStatus();return 0;
    }
    if(m==WM_TIMER&&w==HIGHLIGHT_TIMER){KillTimer(h,HIGHLIGHT_TIMER);highlight();return 0;}
    if(m==JOB_DONE){std::unique_ptr<Result> r((Result*)l);if(task.joinable())task.join();busy=false;
      SetWindowTextW(output,r->text.c_str());setStatus(r->stopped?L"Stopped":(r->code==0?L"Success":L"Error"));return 0;}
    if(m==WM_DROPFILES){
      HDROP drop=(HDROP)w;
      UINT size=DragQueryFileW(drop,0, nullptr,0);
      if(size>0&&size<32768){std::wstring p((size_t)size+1,L'\0');
        DragQueryFileW(drop,0,&p[0],size+1);p.resize(size);
        if(discard())loadFile(fs::path(p));
      }
      DragFinish(drop);return 0;
    }
    if(m==WM_CLOSE){if(discard())DestroyWindow(h);return 0;}
    if(m==WM_DESTROY){KillTimer(h,HIGHLIGHT_TIMER);if(findWindow)DestroyWindow(findWindow);
      DragAcceptFiles(h,FALSE);if(busy)stop();if(task.joinable())task.join();if(font)DeleteObject(font);
      if(accelerators)DestroyAcceleratorTable(accelerators);if(richLibrary)FreeLibrary(richLibrary);
      PostQuitMessage(0);return 0;}
    return DefWindowProcW(h,m,w,l);
  }
public:
  Studio(){InitializeCriticalSection(&processLock);}
  ~Studio(){DeleteCriticalSection(&processLock);}
  int show(HINSTANCE instance,int initial,const fs::path& initialFile={},bool guiSmoke=false){
    wchar_t location[32768]{};auto n=GetModuleFileNameW(nullptr,location,32768);if(!n||n>=32768)return 2;
    root=fs::path(location).parent_path();if(root.filename()==L"bin")root=root.parent_path();
    runtime=root/L"bin"/L"nova-native.exe";windowed=root/L"bin"/L"nova-windowed.exe";
    compiler=root/L"compiler"/L"novac_selfhost.nbc";
    WNDCLASSW wc{};wc.hInstance=instance;wc.lpfnWndProc=callback;wc.hCursor=LoadCursorW(nullptr,IDC_ARROW);
    wc.hbrBackground=(HBRUSH)(COLOR_WINDOW+1);wc.lpszClassName=L"NovaStudio10Window";
    if(!RegisterClassW(&wc))return 2;
    mainWindow=CreateWindowW(wc.lpszClassName,L"Nova Studio",WS_OVERLAPPEDWINDOW,
                              CW_USEDEFAULT,CW_USEDEFAULT,1050,760,nullptr,nullptr,instance,this);
    if(!mainWindow)return 2;
    ACCEL bindings[]={{FVIRTKEY|FCONTROL,'N',ID_NEW},{FVIRTKEY|FCONTROL,'O',ID_OPEN},{FVIRTKEY|FCONTROL,'S',ID_SAVE},
                       {FVIRTKEY|FCONTROL,'F',ID_FIND},{FVIRTKEY|FCONTROL,'H',ID_REPLACE},
                       {FVIRTKEY,VK_F3,ID_NEXT},{FVIRTKEY|FSHIFT,VK_F3,ID_PREVIOUS},
                       {FVIRTKEY,VK_F5,ID_RUN},{FVIRTKEY,VK_F6,ID_CHECK},{FVIRTKEY,VK_F7,ID_BUILD},{FVIRTKEY|FSHIFT,VK_F5,ID_STOP}};
    accelerators=CreateAcceleratorTableW(bindings,(int)(sizeof(bindings)/sizeof(bindings[0])));
    if(!initialFile.empty())loadFile(initialFile);
    // Actual Win32 GUI smoke test for CI: instantiate the editor and controls,
    // exercise Unicode text, native WM_COMMAND handling and focus/selection
    // without a person interacting with the desktop.
    if(guiSmoke){
      ShowWindow(mainWindow,SW_SHOW);UpdateWindow(mainWindow);
      int result=0;
      try{
        if(!IsWindow(mainWindow)||!IsWindow(editor)||!IsWindow(output)||!IsWindow(status))result=21;
        if(!result){
          loading=true;
          if(!SetWindowTextW(editor,L"покажи \"Київ Łódź\"\r\n"))result=22;
          loading=false;
          if(!result && editorText(editor).find(L"Київ Łódź")==std::wstring::npos)result=23;
        }
        if(!result){
          // The real editor window receives and processes the New menu action.
          dirty=false;
          SendMessageW(mainWindow,WM_COMMAND,MAKEWPARAM(ID_NEW,0),0);
          if(!editorText(editor).empty())result=24;
        }
        if(!result){
          SendMessageW(mainWindow,WM_SIZE,0,MAKELPARAM(1024,700));
          if(!IsWindowVisible(editor))result=25;
        }
      }catch(...){result=26;}
      DestroyWindow(mainWindow);return result;
    }
    SetTimer(mainWindow,HIGHLIGHT_TIMER,100,nullptr);updateStatus();
    ShowWindow(mainWindow,initial);UpdateWindow(mainWindow);MSG msg{};
    while(GetMessageW(&msg,nullptr,0,0)>0){
      if(findWindow&&IsDialogMessageW(findWindow,&msg))continue;
      if(!TranslateAcceleratorW(mainWindow,accelerators,&msg)){
        TranslateMessage(&msg);DispatchMessageW(&msg);
      }
    }
    return (int)msg.wParam;
  }
};
int WINAPI wWinMain(HINSTANCE h,HINSTANCE,LPWSTR args,int show){
  if(args&&wcsstr(args,L"--self-test")){
    try{
      const std::wstring value=L"Київ Łódź Żółw Nova";
      if(toWide(toUtf8(value))!=value||quoted(L"abc\\").empty())return 12;
      if(nova_studio::findNext(L"Київ Łódź",L"ŁÓDŹ").begin!=5)return 14;
      if(nova_studio::replaceAll(L"Nova nova",L"nova",L"мова").count!=2)return 15;
      if(nova_studio::tokenize(L"покажи \"Hi\" # test").size()!=3)return 16;
      return 0;
    }catch(...){return 13;}
  }
  fs::path initial;
  int count=0;LPWSTR* argsv=CommandLineToArgvW(GetCommandLineW(),&count);
  if(argsv){if(count>=2&&argsv[1][0]!=L'-')initial=fs::path(argsv[1]);LocalFree(argsv);}
  Studio studio;return studio.show(h,show,initial,args&&wcsstr(args,L"--gui-smoke")?true:false);
}
