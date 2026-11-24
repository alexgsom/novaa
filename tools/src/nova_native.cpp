// Nova Native 0.9 Windows-first — independent runtime and Stage-1 bootstrap VM (C++17).
// No Python runtime, no Python embedded, no external dependencies.
// License: MIT. This is a deliberately limited subset of Nova 1.2.
#include <algorithm>
#include <cstring>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <filesystem>
#include <iterator>
#ifdef _WIN32
#include "nova_win32_ui.hpp"
#include <shellapi.h>
#endif
#include <functional>
#include <iomanip>
#include <iostream>
#include <limits>
#include <climits>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>
using namespace std;

struct NovaError : runtime_error { explicit NovaError(const string& msg):runtime_error(msg){} };
static string trim(const string& s) { auto l=s.find_first_not_of(" \t\r\n"),r=s.find_last_not_of(" \t\r\n"); return l==string::npos?"":s.substr(l,r-l+1); }
static string stripComment(const string& s) {
  char quote=0; bool esc=false;
  for (size_t i=0;i<s.size();++i) { char c=s[i]; if (quote) { if(esc)esc=false; else if(c=='\\')esc=true; else if(c==quote)quote=0; }
    else if(c=='\''||c=='"')quote=c; else if(c=='#')return trim(s.substr(0,i)); }
  return trim(s);
}
static const unordered_map<string,string> ALIAS = {
  {"let","let"},{"нехай","let"},{"niech","let"},
  {"show","show"},{"покажи","show"},{"pokaż","show"},
  {"if","if"},{"якщо","if"},{"jeśli","if"},
  {"else","else"},{"інакше","else"},{"inaczej","else"},
  {"end","end"},{"кінець","end"},{"koniec","end"},
  {"repeat","repeat"},{"повтори","repeat"},{"powtórz","repeat"},
  {"for","for"},{"для","for"},{"dla","for"},
  {"try","try"},{"спробуй","try"},{"spróbuj","try"},
  {"catch","catch"},{"злови","catch"},{"złap","catch"},
  {"match","match"},{"вибір","match"},{"wybierz","match"},
  {"case","case"},{"коли","case"},{"przypadek","case"},
  {"function","function"},{"функція","function"},{"funkcja","function"},
  {"return","return"},{"поверни","return"},{"zwróć","return"},
  {"break","break"},{"перерви","break"},{"przerwij","break"},
  {"continue","continue"},{"продовж","continue"},{"kontynuuj","continue"},
  {"raise","raise"},{"викинь","raise"},{"rzuć","raise"},
  {"assert","assert"},{"перевір","assert"},{"sprawdź","assert"}
};
static const unordered_map<string,string> LEX_ALIAS = {
  {"істина","true"},{"prawda","true"},{"хиба","false"},{"fałsz","false"},
  {"нічого","null"},{"nic","null"},{"і","and"},{"oraz","and"},
  {"або","or"},{"lub","or"},{"не","not"},{"nie","not"}
};
static bool nameStart(unsigned char c) { return (c>='A'&&c<='Z')||(c>='a'&&c<='z')||c=='_'||c>=128; }
static bool nameContinue(unsigned char c) { return nameStart(c)||(c>='0'&&c<='9'); }
static vector<string> utf8Chars(const string& s) {
  vector<string> out;
  for(size_t i=0;i<s.size();){
    unsigned char c=(unsigned char)s[i];
    size_t width=c<128?1:(c&0xE0)==0xC0?2:(c&0xF0)==0xE0?3:(c&0xF8)==0xF0?4:1;
    width=min(width,s.size()-i);
    out.push_back(s.substr(i,width));i+=width;
  }
  return out;
}
static bool validName(const string& s) { if(s.empty()||!nameStart((unsigned char)s[0]))return false; for(unsigned char c:s)if(!nameContinue(c))return false;return true; }

struct Value {
  using List=vector<Value>; using Object=map<string,Value>;
  variant<monostate,bool,double,string,shared_ptr<List>,shared_ptr<Object>> data;
  Value():data(monostate{}){} Value(nullptr_t):data(monostate{}){} Value(bool v):data(v){}
  Value(int v):data(double(v)){} Value(double v):data(v){} Value(string v):data(std::move(v)){}
  Value(const char* v):data(string(v)){}
  static Value list(List x) { Value v;v.data=make_shared<List>(std::move(x));return v; }
  static Value object(Object x) { Value v;v.data=make_shared<Object>(std::move(x));return v; }
  bool number()const{return holds_alternative<double>(data);} 
  double num(int line)const{if(!number())throw NovaError("Runtime error (line "+to_string(line)+"): expected a number");return get<double>(data);}
  bool truth()const {
    if(holds_alternative<monostate>(data))return false;
    if(auto p=get_if<bool>(&data))return *p;
    if(auto p=get_if<double>(&data))return *p!=0;
    if(auto p=get_if<string>(&data))return !p->empty();
    if(auto p=get_if<shared_ptr<List>>(&data))return !(*p)->empty();
    return !get<shared_ptr<Object>>(data)->empty();
  }
  string reprSafe(bool quoted,set<const void*>& visited)const {
    if(holds_alternative<monostate>(data))return "None";
    if(auto p=get_if<bool>(&data))return *p?"True":"False";
    if(auto p=get_if<double>(&data)){ostringstream o;o<<setprecision(15)<<*p;return o.str();}
    if(auto p=get_if<string>(&data)){
      if(!quoted)return *p;
      string out="\"";for(char c:*p){
        if(c=='\\'||c=='"')out+='\\';
        if(c=='\n'){out+="\\n";continue;}
        out+=c;
      }return out+'"';
    }
    if(auto p=get_if<shared_ptr<List>>(&data)){
      const void* id=p->get();if(!visited.insert(id).second)return "[...]";
      string out="[";for(size_t i=0;i<(*p)->size();++i){if(i)out+=", ";out+=(*p)->at(i).reprSafe(true,visited);}
      visited.erase(id);return out+"]";
    }
    const auto& obj=get<shared_ptr<Object>>(data);const void* id=obj.get();
    if(!visited.insert(id).second)return "{...}";
    string out="{";bool first=true;
    for(const auto& item:*obj){if(!first)out+=", ";first=false;
      out+=Value(item.first).reprSafe(true,visited)+": "+item.second.reprSafe(true,visited);
    }
    visited.erase(id);return out+"}";
  }
  string repr(bool quoted=false)const {set<const void*> visiting;return reprSafe(quoted,visiting);}

};
#include "nova_json.hpp"
#include "nova_sqlite.hpp"
#include "nova_http.hpp"
#ifdef NOVA_EMBEDDED_APP
#include "nova_embedded_app.hpp"
#endif
#ifdef NOVA_EMBEDDED_BYTECODE_APP
#include "nova_embedded_bytecode.hpp"
#endif
static bool equalSafe(const Value& a,const Value& b,set<pair<const void*,const void*>>& visited){
  if(a.data.index()!=b.data.index())return false;
  if(holds_alternative<monostate>(a.data))return true;
  if(auto p=get_if<bool>(&a.data))return *p==get<bool>(b.data);
  if(auto p=get_if<double>(&a.data))return *p==get<double>(b.data);
  if(auto p=get_if<string>(&a.data))return *p==get<string>(b.data);
  if(auto p=get_if<shared_ptr<Value::List>>(&a.data)){
    auto& other=*get<shared_ptr<Value::List>>(b.data);
    if((*p)->size()!=other.size())return false;
    pair<const void*,const void*> key={p->get(),&other};
    if(!visited.insert(key).second)return true;
    for(size_t i=0;i<other.size();++i)if(!equalSafe((*p)->at(i),other[i],visited))return false;
    return true;
  }
  const auto& x=*get<shared_ptr<Value::Object>>(a.data),&y=*get<shared_ptr<Value::Object>>(b.data);
  if(x.size()!=y.size())return false;
  pair<const void*,const void*> key={&x,&y};if(!visited.insert(key).second)return true;
  for(auto& item:x){auto it=y.find(item.first);if(it==y.end()||!equalSafe(item.second,it->second,visited))return false;}
  return true;
}
static bool equalValue(const Value& a,const Value& b){set<pair<const void*,const void*>> visited;return equalSafe(a,b,visited);}

struct Token {string text;string kind;size_t column;};
static vector<Token> tokenize(const string& s,int line){
  vector<Token> a;
  for(size_t i=0;i<s.size();){
    if(isspace((unsigned char)s[i])){++i;continue;}
    size_t begin=i;char c=s[i];
    if(isdigit((unsigned char)c)) {while(i<s.size()&&isdigit((unsigned char)s[i]))++i;if(i+1<s.size()&&s[i]=='.'&&isdigit((unsigned char)s[i+1])){++i;while(i<s.size()&&isdigit((unsigned char)s[i]))++i;}a.push_back({s.substr(begin,i-begin),"number",begin+1});}
    else if(nameStart((unsigned char)c)){while(i<s.size()&&nameContinue((unsigned char)s[i]))++i;auto raw=s.substr(begin,i-begin);auto it=LEX_ALIAS.find(raw);a.push_back({it==LEX_ALIAS.end()?raw:it->second,"name",begin+1});}
    else if(c=='"'||c=='\''){
      char q=c;++i;bool done=false,esc=false;for(;i<s.size();++i){char d=s[i];if(esc){esc=false;continue;}if(d=='\\'){esc=true;continue;}if(d==q){++i;done=true;break;}}
      if(!done)throw NovaError("Syntax error (line "+to_string(line)+"): unterminated string");
      a.push_back({s.substr(begin,i-begin),"string",begin+1});
    } else {
      string op=s.substr(i,min((size_t)2,s.size()-i));
      if(op=="=="||op=="!="||op=="<="||op==">="||op=="**"||op=="//")i+=2;
      else if(string("+-*/%<>()[]{},.=:!").find(c)!=string::npos){op=string(1,c);++i;}
      else throw NovaError("Syntax error (line "+to_string(line)+"): invalid token at column "+to_string(i+1));
      a.push_back({op,"op",begin+1});
    }
  }
  a.push_back({"","eof",s.size()+1});return a;
}
struct Expr;using E=shared_ptr<Expr>;
struct Expr {string kind, text;Value literal;vector<E> items;vector<pair<E,E>> pairs;};
static E makeExpr(const string& kind,string text="",vector<E> items={}){auto e=make_shared<Expr>();e->kind=kind;e->text=std::move(text);e->items=std::move(items);return e;}
class ExprParser {
  vector<Token> tokens;size_t pos=0;int line;
  string peek()const{return pos<tokens.size()?tokens[pos].text:"";}
  string pop(){if(pos>=tokens.size())throw NovaError("Syntax error (line "+to_string(line)+"): incomplete expression");return tokens[pos++].text;}
  void expect(const string& s){if(peek()!=s)throw NovaError("Syntax error (line "+to_string(line)+"): expected '"+s+"' at column "+to_string(tokens[pos].column));pop();}
  static int prec(const string& s){if(s=="or")return 1;if(s=="and")return 2;if(s=="=="||s=="!="||s=="<"||s==">"||s=="<="||s==">=")return 3;if(s=="+"||s=="-")return 4;if(s=="*"||s=="/"||s=="//"||s=="%")return 5;if(s=="**")return 7;return 0;}
  vector<E> seq(const string& end) {vector<E> out;if(peek()!=end){while(true){out.push_back(expr(0));if(peek()!=",")break;pop();if(peek()==end)break;}}expect(end);return out;}
  E expr(int minPrec=0){
    string t=pop();E left;
    if(t=="-"||t=="+"||t=="not")left=makeExpr("unary",t,{expr(6)});
    else if(t=="("){left=expr();expect(")");}
    else if(t=="[")left=makeExpr("list","",seq("]"));
    else if(t=="{"){
      left=makeExpr("object");if(peek()!="}"){while(true){E key=expr();expect(":");E val=expr();left->pairs.push_back({key,val});if(peek()!=",")break;pop();if(peek()=="}")break;}}expect("}");
    }
    else if(t=="true"||t=="false"||t=="null"){left=makeExpr("literal");if(t=="true")left->literal=Value(true);else if(t=="false")left->literal=Value(false);}
    else if(tokens[pos-1].kind=="number"){
      left=makeExpr("literal");try{left->literal=Value(stod(t));}catch(...){throw NovaError("Syntax error (line "+to_string(line)+"): invalid number");}
    }
    else if(tokens[pos-1].kind=="string"){
      string s;for(size_t i=1;i+1<t.size();++i){if(t[i]=='\\'&&i+2<t.size()){++i;char e=t[i];s+=(e=='n'?'\n':e=='t'?'\t':e=='r'?'\r':e);}else s+=t[i];}
      left=makeExpr("literal");left->literal=Value(s);
    }
    else if(tokens[pos-1].kind=="name")left=makeExpr("name",t);
    else throw NovaError("Syntax error (line "+to_string(line)+"): unexpected token '"+t+"'");
    while(true){string op=peek();
      if(op=="."&&minPrec<=9){pop();string id=pop();if(!validName(id))throw NovaError("Syntax error (line "+to_string(line)+"): expected property name");left=makeExpr("attr",id,{left});continue;}
      if(op=="["&&minPrec<=9){pop();E idx=expr();expect("]");left=makeExpr("index","",{left,idx});continue;}
      if(op=="("&&minPrec<=9){pop();vector<E> args=seq(")");args.insert(args.begin(),left);left=makeExpr("call","",std::move(args));continue;}
      int p=prec(op);if(!p||p<minPrec)break;pop();E right=expr(op=="**"?p:p+1);left=makeExpr("binary",op,{left,right});
    }return left;
  }
public:
  ExprParser(const string& s,int ln):tokens(tokenize(s,ln)),line(ln){}
  E parse(){E out=expr();if(peek()!="")throw NovaError("Syntax error (line "+to_string(line)+"): unexpected token '"+peek()+"'");return out;}
};
struct Stmt {string kind,arg;int line=0;vector<Stmt> body,other;vector<pair<string,vector<Stmt>>> cases;vector<int> caseLines;};
struct Line {int number;string kind,arg;};
struct Block {vector<Stmt> statements;string stop,arg;int line=0;};
class SourceParser {
  vector<Line> lines;size_t pos=0;
  Block parseBlock(const set<string>& stops={}) {
    Block out;
    while(pos<lines.size()){
      auto l=lines[pos++];if(l.kind=="else"||l.kind=="end"||l.kind=="case"||l.kind=="catch"){
        if(!stops.count(l.kind))throw NovaError("Syntax error (line "+to_string(l.number)+"): unexpected "+l.kind);
        if((l.kind=="else"||l.kind=="end"||l.kind=="catch")&&l.kind!="catch"&&!l.arg.empty())throw NovaError("Syntax error (line "+to_string(l.number)+"): unexpected argument");
        out.stop=l.kind;out.arg=l.arg;out.line=l.number;return out;
      }
      Stmt s;s.kind=l.kind;s.arg=l.arg;s.line=l.number;
      if(s.kind=="if"){
        if(s.arg.empty())throw NovaError("Syntax error (line "+to_string(l.number)+"): if requires a condition");
        auto b=parseBlock({"else","end"});s.body=std::move(b.statements);
        if(b.stop=="else"){b=parseBlock({"end"});s.other=std::move(b.statements);}
        if(b.stop!="end")throw NovaError("Syntax error (line "+to_string(l.number)+"): missing end for if");
      }else if(s.kind=="repeat"||s.kind=="for"||s.kind=="function"){
        if(s.arg.empty())throw NovaError("Syntax error (line "+to_string(l.number)+"): "+s.kind+" requires an argument");
        auto b=parseBlock({"end"});s.body=std::move(b.statements);
        if(b.stop!="end")throw NovaError("Syntax error (line "+to_string(l.number)+"): missing end for "+s.kind);
      }else if(s.kind=="try"){
        if(!s.arg.empty())throw NovaError("Syntax error (line "+to_string(l.number)+"): try takes no argument");
        auto b=parseBlock({"catch"});s.body=std::move(b.statements);
        if(b.stop!="catch")throw NovaError("Syntax error (line "+to_string(l.number)+"): try requires catch");
        s.arg=b.arg.empty()?"error":b.arg;
        if(!validName(s.arg))throw NovaError("Syntax error (line "+to_string(l.number)+"): invalid catch variable");
        b=parseBlock({"end"});s.other=std::move(b.statements);
        if(b.stop!="end")throw NovaError("Syntax error (line "+to_string(l.number)+"): missing end for try");
      }else if(s.kind=="match"){
        if(s.arg.empty())throw NovaError("Syntax error (line "+to_string(l.number)+"): match requires an expression");
        auto b=parseBlock({"case"});
        if(!b.statements.empty()||b.stop!="case")throw NovaError("Syntax error (line "+to_string(l.number)+"): match requires case");
        while(b.stop=="case"){
          if(b.arg.empty())throw NovaError("Syntax error (line "+to_string(b.line)+"): case requires a value");
          string condition=b.arg;int cl=b.line;
          b=parseBlock({"case","else","end"});s.cases.push_back({condition,std::move(b.statements)});s.caseLines.push_back(cl);
        }
        if(b.stop=="else"){b=parseBlock({"end"});s.other=std::move(b.statements);}
        if(b.stop!="end")throw NovaError("Syntax error (line "+to_string(l.number)+"): missing end for match");
      }
      out.statements.push_back(std::move(s));
    }
    return out;
  }
public:
  explicit SourceParser(const string& source){istringstream in(source);string raw;int i=0;while(getline(in,raw)){
    ++i;string x=stripComment(raw);if(x.empty())continue;size_t split=x.find_first_of(" \t");string head=x.substr(0,split),arg=split==string::npos?"":trim(x.substr(split+1));auto it=ALIAS.find(head);lines.push_back({i,it==ALIAS.end()?head:it->second,arg});
  }}
  vector<Stmt> parse(){auto b=parseBlock();if(!b.stop.empty())throw NovaError("Syntax error (line "+to_string(b.line)+"): unexpected "+b.stop);return b.statements;}
};

struct Signal {enum Kind{none,returned,breaking,continuing} kind=none;Value value;};
using NovaEnv = unordered_map<string,Value>;
struct Func {vector<string> params;vector<Stmt> body;shared_ptr<NovaEnv> module;string scriptPath;};
static string readFile(const string& filename);
class Runtime {
  friend class NovaBytecodeVM; // VM invokes the same validated native module APIs.
  using Env=NovaEnv;
  Env globals;unordered_map<string,Func> functions;
  NovaSqlite sqlite;
  unordered_map<string,Value> importedModules;
  set<string> importingModules;
  shared_ptr<Env> currentModule;
  string currentScript;
  size_t nextFunctionId=0;
  vector<string> applicationArgs;
#ifdef _WIN32
  NovaWin32Ui gui;
#endif
  static string platformName(){
#ifdef _WIN32
    return "windows";
#elif defined(__APPLE__)
    return "macos";
#else
    return "linux";
#endif
  }
  static std::filesystem::path pathFromUtf8(const string& path) { return std::filesystem::u8path(path); }
  static string utf8Path(const std::filesystem::path& path) { return path.u8string(); }
  static string requireText(const Value& v,int line,const string& name) {
    if(auto p=get_if<string>(&v.data))return *p;
    throw NovaError("Runtime error (line "+to_string(line)+"): "+name+" must be text");
  }
  static int requireHandle(const Value& v,int line) {
    auto p=get_if<shared_ptr<Value::Object>>(&v.data);
    if(!p)throw NovaError("Runtime error (line "+to_string(line)+"): expected UI control");
    auto it=(*p)->find("__nova_ui_handle");
    if(it==(*p)->end()||!it->second.number())throw NovaError("Runtime error (line "+to_string(line)+"): expected UI control");
    return (int)it->second.num(line);
  }
  Value uiValue(int id){return Value::object({{"__nova_ui_handle", Value(id)}});}
  static string appdataPath() {
#ifdef _WIN32
    // Win32 wide-character environment API preserves Cyrillic Windows usernames.
    DWORD count=GetEnvironmentVariableW(L"APPDATA",nullptr,0);
    if(!count||count>32768)throw NovaError("APPDATA is not available");
    std::wstring buf(count,L'\0');
    DWORD actual=GetEnvironmentVariableW(L"APPDATA",&buf[0],count);
    if(actual==0||actual>=count)throw NovaError("Unable to read APPDATA");
    buf.resize(actual);return novaToUtf8(buf);
#else
    const char* x=std::getenv("XDG_CONFIG_HOME");if(x&&*x)return x;
    const char* h=std::getenv("HOME");if(h&&*h)return (std::filesystem::u8path(h)/".config").u8string();
    throw NovaError("HOME / XDG_CONFIG_HOME is not available");
#endif
  }
  static int coordinate(const Value& v,int line){
    double n=v.num(line);
    if(!std::isfinite(n)||std::floor(n)!=n||n<0||n>5000)throw NovaError("Runtime error (line "+to_string(line)+"): UI size/coordinate must be an integer in 0..5000");
    return (int)n;
  }
  static Value::List& requireList(const Value& v,int line,const string& function) {
    auto p=get_if<shared_ptr<Value::List>>(&v.data);
    if(!p)throw NovaError("Runtime error (line "+to_string(line)+"): "+function+" expects a list");
    return **p;
  }
  Value invokeModule(const string& module,const string& method,const vector<Value>& args,Env& env,int line) {
    (void)env;
    auto need=[&](size_t n){if(args.size()!=n)fail("runtime",line,module+"."+method+" expects "+to_string(n)+" argument(s)");};
    if(module=="sqlite") {
      auto dbId=[&](const Value& value)->int{
        auto obj=get_if<shared_ptr<Value::Object>>(&value.data);
        if(!obj)fail("runtime",line,"sqlite expects connection handle");
        auto it=(*obj)->find("__nova_sqlite_handle");
        if(it==(*obj)->end()||!it->second.number())fail("runtime",line,"sqlite expects connection handle");
        return (int)it->second.num(line);
      };
      try {
        if(method=="open"){
          need(1);int id=sqlite.open(requireText(args[0],line,"database filename"));
          return Value::object({{"__nova_sqlite_handle",Value(id)}});
        }
        if(method=="close"){need(1);sqlite.close(dbId(args[0]));return Value();}
        if(method=="query"||method=="execute"){
          if(args.size()<2||args.size()>3)fail("runtime",line,"sqlite."+method+" expects connection, SQL[, parameters]");
          vector<Value> params;
          if(args.size()==3)params=requireList(args[2],line,"sqlite parameters");
          return sqlite.run(dbId(args[0]),requireText(args[1],line,"SQL"),params,method=="query");
        }
      }catch(const std::runtime_error& e){fail("runtime",line,e.what());}
    }
    if(module=="http"){
      try {
        if(method=="get"){need(1);auto result=nova_http::request("GET",requireText(args[0],line,"URL"));
          return Value::object({{"status",Value(result.status)},{"body",Value(result.body)}});
        }
        if(method=="post"){
          if(args.size()<2||args.size()>3)fail("runtime",line,"http.post expects URL, body[, content-type]");
          string type=args.size()==3?requireText(args[2],line,"content type"):"application/json";
          auto result=nova_http::request("POST",requireText(args[0],line,"URL"),requireText(args[1],line,"body"),type);
          return Value::object({{"status",Value(result.status)},{"body",Value(result.body)}});
        }
      }catch(const std::runtime_error& e){fail("runtime",line,e.what());}
    }
    if(module=="system") {
      if(method=="args"){need(0);Value::List a;for(const auto& v:applicationArgs)a.emplace_back(v);return Value::list(std::move(a));}
      if(method=="platform"){need(0);return Value(platformName());}
      if(method=="cwd"){need(0);return Value(utf8Path(std::filesystem::current_path()));}
      if(method=="appdata"){need(0);return Value(appdataPath());}
      if(method=="temp"){need(0);return Value(utf8Path(std::filesystem::temp_directory_path()));}
      if(method=="join"){
        if(args.size()<2)fail("runtime",line,"system.join expects 2 or more arguments");
        std::filesystem::path p=pathFromUtf8(requireText(args[0],line,"path"));
        for(size_t i=1;i<args.size();++i)p/=pathFromUtf8(requireText(args[i],line,"path"));
        return Value(utf8Path(p.lexically_normal()));
      }
    }
    if(module=="storage") {
      try {
      if(method=="exists"){need(1);return Value(std::filesystem::exists(pathFromUtf8(requireText(args[0],line,"path"))));}
      if(method=="mkdir"){need(1);return Value(std::filesystem::create_directories(pathFromUtf8(requireText(args[0],line,"path"))));}
      if(method=="list"){
        need(1);Value::List names;
        for(auto& it:std::filesystem::directory_iterator(pathFromUtf8(requireText(args[0],line,"path")))){
          if(names.size()>=10000)fail("runtime",line,"storage.list limit exceeded");
          names.push_back(Value(utf8Path(it.path().filename())));
        }
        std::sort(names.begin(),names.end(),[](const Value& a,const Value& b){return get<string>(a.data)<get<string>(b.data);});
        return Value::list(std::move(names));
      }
      if(method=="remove"){
        need(1);auto path=pathFromUtf8(requireText(args[0],line,"path"));
        if(std::filesystem::is_directory(path))fail("runtime",line,"storage.remove only removes files, not directories");
        return Value(std::filesystem::remove(path));
      }
      if(method=="rename"||method=="copy"){
        need(2);auto from=pathFromUtf8(requireText(args[0],line,"source"));
        auto to=pathFromUtf8(requireText(args[1],line,"destination"));
        if(std::filesystem::exists(to))fail("runtime",line,"destination already exists");
        if(method=="rename")std::filesystem::rename(from,to);
        else std::filesystem::copy_file(from,to,std::filesystem::copy_options::none);
        return Value(true);
      }
      if(method=="size"){
        need(1);return Value((double)std::filesystem::file_size(pathFromUtf8(requireText(args[0],line,"path"))));
      }
      if(method=="read"){
        need(1);std::ifstream in(pathFromUtf8(requireText(args[0],line,"path")),std::ios::binary);
        if(!in)fail("runtime",line,"cannot open file for reading");
        constexpr size_t limit=16*1024*1024;
        string content;char buffer[8192];
        while(in){in.read(buffer,sizeof(buffer));auto n=in.gcount();if(n>0){
          if(content.size()+(size_t)n>limit)fail("runtime",line,"storage.read exceeds 16 MiB limit");
          content.append(buffer,(size_t)n);
        }}
        if(in.bad())fail("runtime",line,"file read failed");
        if(content.rfind("\xEF\xBB\xBF",0)==0)content.erase(0,3);
        return Value(content);
      }
      if(method=="write"||method=="append"){
        need(2);auto path=pathFromUtf8(requireText(args[0],line,"path"));auto contents=requireText(args[1],line,"contents");
        std::ofstream out(path,std::ios::binary|(method=="append"?std::ios::app:std::ios::trunc));
        if(!out)fail("runtime",line,"cannot open file for writing");
        out.write(contents.data(),(std::streamsize)contents.size());out.close();
        if(!out)fail("runtime",line,"file write failed");
        return Value(true);
      }
      } catch(const std::filesystem::filesystem_error& err) {
        fail("runtime",line,"filesystem error: "+string(err.what()));
      }
    }
    if(module=="json") {
      try {
        if(method=="parse"){need(1);return nova_json::Parser(requireText(args[0],line,"json text")).parse();}
        if(method=="stringify"||method=="pretty"){need(1);return Value(nova_json::Writer(method=="pretty").stringify(args[0]));}
      }catch(const std::runtime_error& e){fail("runtime",line,string("json: ")+e.what());}
    }
    if(module=="text") {
      if(method=="slice"){
        if(args.size()!=2&&args.size()!=3)fail("runtime",line,"text.slice expects text, start[, length]");
        auto chars=utf8Chars(requireText(args[0],line,"text"));
        double x=args[1].num(line);
        if(!isfinite(x)||floor(x)!=x||x<0||x>(double)chars.size())fail("runtime",line,"text.slice invalid start");
        size_t start=(size_t)x,count=chars.size()-start;
        if(args.size()==3){double n=args[2].num(line);if(!isfinite(n)||floor(n)!=n||n<0||n>(double)count)fail("runtime",line,"text.slice invalid length");count=(size_t)n;}
        string out;for(size_t i=start;i<start+count;++i)out+=chars[i];return Value(out);
      }
      if(method=="index_of"){
        need(2);auto source=requireText(args[0],line,"text"),pattern=requireText(args[1],line,"pattern");
        size_t pos=source.find(pattern);if(pos==string::npos)return Value(-1);
        return Value((int)utf8Chars(source.substr(0,pos)).size());
      }
      if(method=="trim"){need(1);return Value(trim(requireText(args[0],line,"text")));}
      if(method=="starts_with"||method=="ends_with"||method=="contains"){
        need(2);auto source=requireText(args[0],line,"text"),pattern=requireText(args[1],line,"pattern");
        if(method=="starts_with")return Value(source.rfind(pattern,0)==0);
        if(method=="ends_with")return Value(source.size()>=pattern.size()&&source.compare(source.size()-pattern.size(),pattern.size(),pattern)==0);
        return Value(source.find(pattern)!=string::npos);
      }
      if(method=="split"){
        need(2);auto source=requireText(args[0],line,"text"),sep=requireText(args[1],line,"separator");
        if(sep.empty())fail("runtime",line,"separator cannot be empty");
        Value::List out;size_t pos=0;
        while(true){auto found=source.find(sep,pos);if(found==string::npos){out.emplace_back(source.substr(pos));break;}
          if(out.size()>=100000)fail("runtime",line,"text.split part limit exceeded");
          out.emplace_back(source.substr(pos,found-pos));pos=found+sep.size();}
        return Value::list(std::move(out));
      }
      if(method=="join"){
        need(2);auto sep=requireText(args[0],line,"separator");auto& a=requireList(args[1],line,"text.join");
        string out;for(const auto& item:a)if(!holds_alternative<string>(item.data))fail("runtime",line,"text.join requires list of text");
        for(size_t i=0;i<a.size();++i){if(i)out+=sep;out+=get<string>(a[i].data);if(out.size()>4*1024*1024)fail("runtime",line,"text.join output too large");}
        return Value(out);
      }
      if(method=="replace"){
        need(3);auto src=requireText(args[0],line,"text"),old=requireText(args[1],line,"old"),replacement=requireText(args[2],line,"new");
        if(old.empty())fail("runtime",line,"text.replace old text cannot be empty");
        string out;size_t pos=0;while(true){auto f=src.find(old,pos);if(f==string::npos){out+=src.substr(pos);break;}
          out+=src.substr(pos,f-pos);out+=replacement;pos=f+old.size();if(out.size()>4*1024*1024)fail("runtime",line,"text.replace output too large");}
        return Value(out);
      }
    }
    if(module=="lists") {
      if(method=="append"){need(2);auto& a=requireList(args[0],line,"lists.append");if(a.size()>=100000)fail("runtime",line,"list size limit exceeded");a.push_back(args[1]);return Value((int)a.size());}
      if(method=="pop"){need(1);auto& a=requireList(args[0],line,"lists.pop");if(a.empty())fail("runtime",line,"cannot pop empty list");Value v=a.back();a.pop_back();return v;}
      if(method=="reverse"){need(1);auto& a=requireList(args[0],line,"lists.reverse");std::reverse(a.begin(),a.end());return args[0];}
      if(method=="sort"){
        need(1);auto& a=requireList(args[0],line,"lists.sort");
        if(!a.empty())for(const auto& item:a){if(item.data.index()!=a[0].data.index()||(!item.number()&&!holds_alternative<string>(item.data)))fail("runtime",line,"lists.sort requires numbers or text of one type");}
        std::sort(a.begin(),a.end(),[](const Value& x,const Value& y){return x.number()?get<double>(x.data)<get<double>(y.data):get<string>(x.data)<get<string>(y.data);});
        return args[0];
      }
    }
    if(module=="ui") {
#ifndef _WIN32
      fail("runtime",line,"native ui is currently available on Windows only");
#else
      if(method=="window"){
        if(args.size()<1||args.size()>3)fail("runtime",line,"ui.window expects title[, width, height]");
        int width=args.size()>=2?coordinate(args[1],line):520;
        int height=args.size()>=3?coordinate(args[2],line):420;
        return uiValue(gui.createWindow(requireText(args[0],line,"title"),width,height));
      }
      if(method=="label"){
        need(2);return uiValue(gui.label(requireHandle(args[0],line),requireText(args[1],line,"text")));
      }
      if(method=="button"){
        need(3);string callback;
        if(auto text=get_if<string>(&args[2].data))callback=*text;
        else if(auto p=get_if<shared_ptr<Value::Object>>(&args[2].data)){
          auto it=(*p)->find("__nova_function");
          if(it!=(*p)->end())callback=requireText(it->second,line,"callback");
        }
        if(callback.empty()||!functions.count(callback))fail("runtime",line,"button callback must be an existing Nova function");
        if(!functions.at(callback).params.empty())fail("runtime",line,"button callback must have zero arguments");
        return uiValue(gui.button(requireHandle(args[0],line),requireText(args[1],line,"text"),callback));
      }
      if(method=="input"){
        need(2);return uiValue(gui.input(requireHandle(args[0],line),requireText(args[1],line,"text")));
      }
      if(method=="textarea"){
        need(2);return uiValue(gui.textarea(requireHandle(args[0],line),requireText(args[1],line,"text")));
      }
      if(method=="checkbox"){
        need(3);if(!holds_alternative<bool>(args[2].data))fail("runtime",line,"ui.checkbox requires boolean checked value");
        return uiValue(gui.checkbox(requireHandle(args[0],line),requireText(args[1],line,"text"),get<bool>(args[2].data)));
      }
      if(method=="select"){
        need(2);const auto& values=requireList(args[1],line,"ui.select");if(values.size()>500)fail("runtime",line,"ui.select has maximum 500 items");
        vector<string> choices;for(const auto& v:values)choices.push_back(requireText(v,line,"choice"));
        return uiValue(gui.select(requireHandle(args[0],line),choices));
      }
      if(method=="menu"){
        need(4);string callback;
        if(auto p=get_if<string>(&args[3].data))callback=*p;
        else if(auto p=get_if<shared_ptr<Value::Object>>(&args[3].data)){auto it=(*p)->find("__nova_function");if(it!=(*p)->end())callback=requireText(it->second,line,"callback");}
        if(callback.empty()||!functions.count(callback)||!functions.at(callback).params.empty())fail("runtime",line,"menu callback must be an existing zero-argument Nova function");
        return uiValue(gui.menu(requireHandle(args[0],line),requireText(args[1],line,"group"),requireText(args[2],line,"text"),callback));
      }
      if(method=="place"){
        need(5);gui.place(requireHandle(args[0],line),coordinate(args[1],line),coordinate(args[2],line),coordinate(args[3],line),coordinate(args[4],line));return Value();
      }
      if(method=="open_file"||method=="save_file"){
        if(args.size()<1||args.size()>2)fail("runtime",line,"ui file dialog expects window[, title]");
        auto name=gui.fileDialog(requireHandle(args[0],line),method=="save_file",args.size()==2?requireText(args[1],line,"title"):"Choose a file");
        return name.empty()?Value():Value(name);
      }
      if(method=="run"){
        need(1);gui.run(requireHandle(args[0],line),[&](const string& callback){invoke(callback,{},globals,line);});return Value();
      }
      if(method=="get"){
        need(1);int id=requireHandle(args[0],line);if(gui.isCheckbox(id))return Value(gui.getChecked(id));return Value(gui.getText(id));
      }
      if(method=="set"){
        need(2);int id=requireHandle(args[0],line);
        if(gui.isCheckbox(id)){
          if(!holds_alternative<bool>(args[1].data))fail("runtime",line,"checkbox.set requires boolean");
          gui.setChecked(id,get<bool>(args[1].data));
        }else gui.setText(id,requireText(args[1],line,"text"));return Value();
      }
      if(method=="close"){
        need(1);gui.close(requireHandle(args[0],line));return Value();
      }
      if(method=="alert"||method=="confirm"){
        need(2);
        auto title=requireText(args[0],line,"title"),message=requireText(args[1],line,"message");
        return Value(method=="alert"?gui.alert(title,message):gui.confirm(title,message));
      }
#endif
    }
    fail("runtime",line,"unknown API: "+module+"."+method);
  }

  long long steps=0,maxSteps=100000;
  int callDepth=0;
  string lang="en";
  static string message(const string& type,const string& lang){
    if(lang=="uk")return type=="runtime"?"Помилка виконання":type=="name"?"Невідоме ім’я":"Синтаксична помилка";
    if(lang=="pl")return type=="runtime"?"Błąd wykonania":type=="name"?"Nieznana nazwa":"Błąd składni";
    return type=="runtime"?"Runtime error":type=="name"?"Unknown name":"Syntax error";
  }
  [[noreturn]] void fail(const string& type,int line,const string& detail)const{throw NovaError(message(type,lang)+" (line "+to_string(line)+"): "+detail);}
  static int indexFrom(const Value& v,int line,int size){double d=v.num(line);if(!isfinite(d)||floor(d)!=d||d>static_cast<double>(LLONG_MAX)/2||d<static_cast<double>(LLONG_MIN)/2)throw NovaError("Runtime error (line "+to_string(line)+"): index must be a valid integer");long long n=(long long)d;if(n<0)n+=size;if(n<0||n>=size)throw NovaError("Runtime error (line "+to_string(line)+"): index out of range");return (int)n;}
  void tick(int line){if(++steps>maxSteps)fail("runtime",line,"execution step limit exceeded ("+to_string(maxSteps)+")");}
  static pair<string,vector<string>> functionHeader(const string& arg,int line){
    auto a=arg.find('('),b=arg.rfind(')');if(a==string::npos||b==string::npos||b!=arg.size()-1)throw NovaError("Syntax error (line "+to_string(line)+"): expected function name(args)");
    string name=trim(arg.substr(0,a));if(!validName(name))throw NovaError("Syntax error (line "+to_string(line)+"): invalid function name");
    vector<string> params;set<string> used;istringstream in(arg.substr(a+1,b-a-1));string p;
    while(getline(in,p,',')){p=trim(p);if(!validName(p)||!used.insert(p).second)throw NovaError("Syntax error (line "+to_string(line)+"): invalid parameter");params.push_back(p);}
    return {name,params};
  }
  static size_t assignmentIndex(const string& arg){
    int depth=0;char quote=0;bool esc=false;for(size_t i=0;i<arg.size();++i){char c=arg[i];
      if(quote){if(esc)esc=false;else if(c=='\\')esc=true;else if(c==quote)quote=0;}
      else if(c=='\''||c=='"')quote=c;
      else if(c=='('||c=='['||c=='{')++depth;
      else if(c==')'||c==']'||c=='}')--depth;
      else if(c=='='&&depth==0&&(i==0||string("=<>!").find(arg[i-1])==string::npos)&&(i+1==arg.size()||arg[i+1]!='='))return i;
    }return string::npos;
  }
  Value invoke(const string& fn,const vector<Value>& args,Env& env,int line){
    auto need=[&](size_t n){if(args.size()!=n)fail("runtime",line,fn+" expects "+to_string(n)+" argument(s)");};
    if(fn=="len"){need(1);auto& v=args[0];if(auto p=get_if<string>(&v.data))return Value((int)utf8Chars(*p).size());if(auto p=get_if<shared_ptr<Value::List>>(&v.data))return Value((int)(*p)->size());if(auto p=get_if<shared_ptr<Value::Object>>(&v.data))return Value((int)(*p)->size());fail("runtime",line,"len expects text, list or object");}
    if(fn=="str"){need(1);return Value(args[0].repr());}
    if(fn=="int"||fn=="float"){
      need(1);double d=0;
      if(args[0].number())d=args[0].num(line);
      else if(auto p=get_if<string>(&args[0].data)){try{size_t n=0;d=stod(*p,&n);if(n!=p->size())throw invalid_argument("extra");}catch(...){fail("runtime",line,"cannot convert text to number");}}
      else if(auto p=get_if<bool>(&args[0].data))d=*p?1:0;
      else fail("runtime",line,"cannot convert to number");
      if(!isfinite(d))fail("runtime",line,"non-finite number");
      return Value(fn=="int"?trunc(d):d);
    }
    if(fn=="abs"){need(1);return Value(fabs(args[0].num(line)));}
    if(fn=="round"){need(1);return Value(nearbyint(args[0].num(line)));}
    if(fn=="min"||fn=="max"){
      if(args.empty())fail("runtime",line,"expected at least one value");
      double d=args[0].num(line);
      for(size_t i=1;i<args.size();++i)d=fn=="min"?min(d,args[i].num(line)):max(d,args[i].num(line));
      return Value(d);
    }
    if(fn=="sum"){
      need(1);auto p=get_if<shared_ptr<Value::List>>(&args[0].data);if(!p)fail("runtime",line,"sum expects list");double v=0;for(auto& it:**p)v+=it.num(line);return Value(v);
    }
    if(fn=="range"){
      if(args.empty()||args.size()>3)fail("runtime",line,"range expects 1–3 arguments");
      double start=args.size()==1?0:args[0].num(line),stop=args.size()==1?args[0].num(line):args[1].num(line),step=args.size()==3?args[2].num(line):1;
      if(!isfinite(start)||!isfinite(stop)||!isfinite(step)||step==0||floor(start)!=start||floor(stop)!=stop||floor(step)!=step)fail("runtime",line,"range needs integer arguments and nonzero step");
      Value::List out;for(double n=start;step>0?n<stop:n>stop;n+=step){if(out.size()>=100000)fail("runtime",line,"range exceeds 100000 items");out.push_back(Value(n));}return Value::list(std::move(out));
    }
    if(fn=="ask"||fn=="запитай"||fn=="zapytaj"){
      if(args.size()>1)fail("runtime",line,"ask expects 0 or 1 arguments");
      if(!args.empty())cout<<args[0].repr()<<flush;
      string answer;getline(cin,answer);return Value(answer);
    }
    auto it=functions.find(fn);if(it==functions.end())fail("name",line,fn);
    if(it->second.params.size()!=args.size())fail("runtime",line,"wrong argument count for "+fn);
    if(++callDepth>256){--callDepth;fail("runtime",line,"function recursion depth exceeded (256)");}
    struct CallDepthGuard {int& d;~CallDepthGuard(){--d;}} guard{callDepth};
    // Module functions read their own module scope. Calls from the main file keep legacy semantics.
    Env local=it->second.module?*it->second.module:env;
    for(size_t i=0;i<args.size();++i)local[it->second.params[i]]=args[i];
    auto savedModule=currentModule;auto savedScript=currentScript;
    currentModule=it->second.module;currentScript=it->second.scriptPath;
    Signal signal;
    try{signal=exec(it->second.body,local);}
    catch(...){currentModule=savedModule;currentScript=savedScript;throw;}
    currentModule=savedModule;currentScript=savedScript;
    if(signal.kind==Signal::breaking||signal.kind==Signal::continuing)fail("runtime",line,"break/continue outside loop");
    return signal.kind==Signal::returned?signal.value:Value();
  }
  Value eval(const E& e,Env& env,int line){
    auto ev=[&](size_t i){return eval(e->items.at(i),env,line);};
    if(e->kind=="literal")return e->literal;
    if(e->kind=="name") {auto it=env.find(e->text);if(it!=env.end())return it->second;
      if(functions.count(e->text))return Value::object({{"__nova_function",Value(e->text)}});
      fail("name",line,e->text);}
    if(e->kind=="list") {Value::List v;for(auto& item:e->items)v.push_back(eval(item,env,line));return Value::list(std::move(v));}
    if(e->kind=="object") {Value::Object v;for(auto& pair:e->pairs){Value k=eval(pair.first,env,line);if(!holds_alternative<string>(k.data))fail("runtime",line,"object key must be text");v[get<string>(k.data)]=eval(pair.second,env,line);}return Value::object(std::move(v));}
    if(e->kind=="attr") {
      Value base=ev(0);if(auto p=get_if<shared_ptr<Value::Object>>(&base.data)){auto it=(*p)->find(e->text);if(it!=(*p)->end())return it->second;}
      if(e->text=="length"){
        if(auto p=get_if<string>(&base.data))return Value((int)utf8Chars(*p).size());
        if(auto p=get_if<shared_ptr<Value::List>>(&base.data))return Value((int)(*p)->size());
      }fail("name",line,e->text);
    }
    if(e->kind=="index") {Value base=ev(0),idx=ev(1);
      if(auto p=get_if<shared_ptr<Value::List>>(&base.data))return (*p)->at(indexFrom(idx,line,(int)(*p)->size()));
      if(auto p=get_if<string>(&base.data)){auto chars=utf8Chars(*p);int i=indexFrom(idx,line,(int)chars.size());return Value(chars[i]);}
      if(auto p=get_if<shared_ptr<Value::Object>>(&base.data)) {if(!holds_alternative<string>(idx.data))fail("runtime",line,"object index must be text");auto it=(*p)->find(get<string>(idx.data));if(it==(*p)->end())fail("name",line,"unknown object key");return it->second;}
      fail("runtime",line,"index requires list, text or object");
    }
    if(e->kind=="call") {
      auto name=e->items[0];
      vector<Value> a;for(size_t i=1;i<e->items.size();++i)a.push_back(ev(i));
      if(name->kind=="name"){
        auto it=env.find(name->text);
        if(it!=env.end())if(auto p=get_if<shared_ptr<Value::Object>>(&it->second.data)){
          auto fn=(*p)->find("__nova_function");
          if(fn!=(*p)->end())return invoke(requireText(fn->second,line,"function"),a,env,line);
        }
        return invoke(name->text,a,env,line);
      }
      if(name->kind=="attr"){
        auto object=eval(name->items[0],env,line);
        if(auto p=get_if<shared_ptr<Value::Object>>(&object.data)){
          auto mod=(*p)->find("__nova_module");
          if(mod!=(*p)->end())return invokeModule(requireText(mod->second,line,"module"),name->text,a,env,line);
          auto member=(*p)->find(name->text);
          if(member!=(*p)->end())if(auto fnObj=get_if<shared_ptr<Value::Object>>(&member->second.data)){
            auto fn=(*fnObj)->find("__nova_function");
            if(fn!=(*fnObj)->end())return invoke(requireText(fn->second,line,"function"),a,env,line);
          }
          auto ui=(*p)->find("__nova_ui_handle");
          if(ui!=(*p)->end()){
            if(name->text=="get")return invokeModule("ui","get",{object},env,line);
            if(name->text=="set"){
              if(a.size()!=1)fail("runtime",line,"control.set expects 1 argument");
              return invokeModule("ui","set",{object,a[0]},env,line);
            }
          }
        }
      }
      fail("runtime",line,"unsupported function or method call");
    }
    if(e->kind=="unary") {Value v=ev(0);if(e->text=="not")return Value(!v.truth());if(e->text=="-")return Value(-v.num(line));return Value(v.num(line));}
    if(e->kind=="binary"){
      Value x=ev(0);string op=e->text;if(op=="and")return x.truth()?ev(1):x;if(op=="or")return x.truth()?x:ev(1);
      Value y=ev(1);
      if(op=="==")return Value(equalValue(x,y));
      if(op=="!=")return Value(!equalValue(x,y));
      if((op=="<"||op==">"||op=="<="||op==">=")&&holds_alternative<string>(x.data)&&holds_alternative<string>(y.data)){
        auto& a=get<string>(x.data),&b=get<string>(y.data);return Value(op=="<"?a<b:op==">"?a>b:op=="<="?a<=b:a>=b);
      }
      if(op=="+"&&holds_alternative<string>(x.data)&&holds_alternative<string>(y.data))return Value(get<string>(x.data)+get<string>(y.data));
      if(op=="+"&&holds_alternative<shared_ptr<Value::List>>(x.data)&&holds_alternative<shared_ptr<Value::List>>(y.data)){
        Value::List a=*get<shared_ptr<Value::List>>(x.data);auto& b=*get<shared_ptr<Value::List>>(y.data);a.insert(a.end(),b.begin(),b.end());return Value::list(std::move(a));
      }
      double a=x.num(line),b=y.num(line);
      if(op=="+")return Value(a+b);
      if(op=="-")return Value(a-b);
      if(op=="*")return Value(a*b);
      if(op=="<")return Value(a<b);
      if(op==">")return Value(a>b);
      if(op=="<=")return Value(a<=b);
      if(op==">=")return Value(a>=b);
      if((op=="/"||op=="//"||op=="%")&&b==0)fail("runtime",line,"division by zero");
      if(op=="/")return Value(a/b);
      if(op=="//")return Value(floor(a/b));
      if(op=="%")return Value(a-floor(a/b)*b);
      if(op=="**")return Value(pow(a,b));
    }
    fail("syntax",line,"unsupported expression");
  }
  Value expression(const string& s,Env& env,int line){return eval(ExprParser(s,line).parse(),env,line);}
  Value iterable(const Value& v,int line){
    if(holds_alternative<shared_ptr<Value::List>>(v.data))return v;
    if(auto p=get_if<shared_ptr<Value::Object>>(&v.data)){Value::List a;for(auto& item:**p)a.emplace_back(item.first);return Value::list(std::move(a));}
    if(auto p=get_if<string>(&v.data)){Value::List a;for(auto& c:utf8Chars(*p))a.emplace_back(c);return Value::list(std::move(a));}
    fail("runtime",line,"for requires a list, object or text");
  }
  Value importUserModule(const string& spec,int line){
    string filename,alias;
    if(spec.empty())fail("syntax",line,"import expects a module name");
    if(spec[0]=='\"'||spec[0]=='\''){
      size_t end=spec.find(spec[0],1);
      if(end==string::npos)fail("syntax",line,"import path must be quoted");
      filename=spec.substr(1,end-1);
      string rest=trim(spec.substr(end+1));
      if(rest.rfind("as ",0)!=0)fail("syntax",line,"import path requires 'as name'");
      alias=trim(rest.substr(3));
      if(filename.size()<6||filename.substr(filename.size()-5)!=".nova")fail("syntax",line,"import path must end with .nova");
    }else {
      if(spec.find(' ')!=string::npos)fail("syntax",line,"use 'import name' or 'import \"file.nova\" as name'");
      alias=spec;filename=spec+".nova";
    }
    if(!validName(alias))fail("syntax",line,"invalid import alias");
    if(filename.empty()||filename.size()>512||filename.find('\\')!=string::npos||filename.find(':')!=string::npos)fail("runtime",line,"invalid module path");
    auto relative=std::filesystem::u8path(filename);
    // Windows also has root-relative paths such as /tmp/file.nova, for which is_absolute() is false.
    if(relative.is_absolute() || relative.has_root_name() || relative.has_root_directory())
      fail("runtime",line,"module paths must be relative");
    for(const auto& part:relative)if(part==".."||part==".")fail("runtime",line,"module path cannot contain . or ..");
    auto base=std::filesystem::u8path(currentScript).parent_path();
    auto path=(base/relative).lexically_normal();
    // Generic separators are essential: embedded module paths use '/' on both Windows and POSIX.
    string key=path.generic_u8string();
    auto cached=importedModules.find(key);if(cached!=importedModules.end())return cached->second;
    if(importingModules.count(key))fail("runtime",line,"cyclic module import: "+key);
    if(importedModules.size()+importingModules.size()>=128)fail("runtime",line,"module limit (128) exceeded");
    string source;
#ifdef NOVA_EMBEDDED_APP
    bool found=false;
    for(size_t i=0;i<NOVA_EMBEDDED_MODULE_COUNT;++i){
      if(key==NOVA_EMBEDDED_MODULES[i].name){
        source.assign(reinterpret_cast<const char*>(NOVA_EMBEDDED_MODULES[i].bytes),NOVA_EMBEDDED_MODULES[i].length);
        found=true;break;
      }
    }
    if(!found)fail("runtime",line,"module was not embedded in EXE: "+key);
#else
    try{source=readFile(key);}catch(const NovaError& e){fail("runtime",line,e.what());}
#endif
    if(source.size()>1024*1024)fail("runtime",line,"module exceeds 1 MiB: "+key);
    importingModules.insert(key);
    auto savedModule=currentModule;auto savedScript=currentScript;
    auto moduleEnv=make_shared<Env>();
    currentModule=moduleEnv;currentScript=key;
    try {
      auto tree=SourceParser(Runtime::withoutBom(source)).parse();
      auto signal=exec(tree,*moduleEnv);
      if(signal.kind!=Signal::none)fail("runtime",line,"control flow outside function in module: "+key);
      Value::Object exports;
      for(const auto& item:*moduleEnv)if(item.first.rfind("__nova_",0)!=0)exports[item.first]=item.second;
      Value value=Value::object(std::move(exports));
      importedModules.emplace(key,value);
      importingModules.erase(key);currentModule=savedModule;currentScript=savedScript;
      return value;
    }catch(...){importingModules.erase(key);currentModule=savedModule;currentScript=savedScript;throw;}
  }
  Signal exec(const vector<Stmt>& stmts,Env& env){
    for(auto& s:stmts){tick(s.line);const string& k=s.kind;
      if(k=="let"){
        auto ix=assignmentIndex(s.arg);if(ix==string::npos)fail("syntax",s.line,"expected let name = expression");
        auto name=trim(s.arg.substr(0,ix));if(!validName(name))fail("syntax",s.line,"invalid variable name");env[name]=expression(trim(s.arg.substr(ix+1)),env,s.line);
      }else if(k=="show")cout<<expression(s.arg,env,s.line).repr()<<'\n';
      else if(k=="if") {Signal v=exec(expression(s.arg,env,s.line).truth()?s.body:s.other,env);if(v.kind!=Signal::none)return v;}
      else if(k=="repeat"){
        string arg=s.arg;for(auto suffix:{" times"," разів"," razy"})if(arg.size()>strlen(suffix)&&arg.compare(arg.size()-strlen(suffix),strlen(suffix),suffix)==0){arg=trim(arg.substr(0,arg.size()-strlen(suffix)));break;}
        double count=expression(arg,env,s.line).num(s.line);if(!isfinite(count)||floor(count)!=count||count<0||count>100000)fail("runtime",s.line,"repeat requires an integer from 0 to 100000");
        for(int j=0;j<(int)count;++j){tick(s.line);Signal v=exec(s.body,env);if(v.kind==Signal::breaking)break;if(v.kind==Signal::continuing)continue;if(v.kind!=Signal::none)return v;}
      }else if(k=="for"){
        auto at=s.arg.find(' ');if(at==string::npos)fail("syntax",s.line,"expected for name in expression");string var=s.arg.substr(0,at);if(!validName(var))fail("syntax",s.line,"invalid for variable");
        string rest=trim(s.arg.substr(at+1));auto sep=rest.find(' ');if(sep==string::npos)fail("syntax",s.line,"expected in/в/w");string word=rest.substr(0,sep);if(word!="in"&&word!="в"&&word!="w")fail("syntax",s.line,"expected in/в/w");
        auto seq=iterable(expression(trim(rest.substr(sep+1)),env,s.line),s.line);auto& items=*get<shared_ptr<Value::List>>(seq.data);
        if(items.size()>100000)fail("runtime",s.line,"for iteration limit exceeded");
        for(auto& item:items){tick(s.line);env[var]=item;Signal v=exec(s.body,env);if(v.kind==Signal::breaking)break;if(v.kind==Signal::continuing)continue;if(v.kind!=Signal::none)return v;}
      }else if(k=="match"){
        auto subject=expression(s.arg,env,s.line);bool chosen=false;
        for(size_t i=0;i<s.cases.size();++i)if(equalValue(subject,expression(s.cases[i].first,env,s.caseLines[i]))){Signal v=exec(s.cases[i].second,env);if(v.kind!=Signal::none)return v;chosen=true;break;}
        if(!chosen){Signal v=exec(s.other,env);if(v.kind!=Signal::none)return v;}
      }else if(k=="try"){
        try{Signal v=exec(s.body,env);if(v.kind!=Signal::none)return v;}
        catch(const NovaError& error){env[s.arg]=Value::object({{"message",Value(string(error.what()))}});Signal v=exec(s.other,env);if(v.kind!=Signal::none)return v;}
      }else if(k=="function"){
        auto h=functionHeader(s.arg,s.line);if(currentModule){
          string key="__nova_module_function_"+to_string(++nextFunctionId);
          functions[key]={std::move(h.second),s.body,currentModule,currentScript};
          env[h.first]=Value::object({{"__nova_function",Value(key)}});
        }else functions[h.first]={std::move(h.second),s.body,nullptr,currentScript};
      }else if(k=="return")return {Signal::returned,s.arg.empty()?Value():expression(s.arg,env,s.line)};
      else if(k=="break")return {Signal::breaking,{}};
      else if(k=="continue")return {Signal::continuing,{}};
      else if(k=="assert") {if(!expression(s.arg,env,s.line).truth())fail("runtime",s.line,"Assertion failed: "+s.arg);}
      else if(k=="raise")fail("runtime",s.line,expression(s.arg,env,s.line).repr());
      else if(k=="import"){
        if(s.arg=="storage"||s.arg=="system"||s.arg=="ui"||s.arg=="json"||s.arg=="text"||s.arg=="lists"||s.arg=="sqlite"||s.arg=="http")
          env[s.arg]=Value::object({{"__nova_module",Value(s.arg)}});
        else {
          // Import local .nova files relative to the importing file, not the current working directory.
          string alias=s.arg;
          if(!alias.empty()&&(alias[0]=='\"'||alias[0]=='\'')){
            size_t end=alias.find(alias[0],1);
            if(end==string::npos)fail("syntax",s.line,"invalid import path");
            string rest=trim(alias.substr(end+1));
            if(rest.rfind("as ",0)!=0)fail("syntax",s.line,"import requires 'as name'");
            alias=trim(rest.substr(3));
          }
          if(!validName(alias))fail("syntax",s.line,"invalid import alias");
          env[alias]=importUserModule(s.arg,s.line);
        }
      }
      else if(k=="type")fail("runtime",s.line,"type is not supported by Nova Native 0.4");
      else {
        string statement=k+(s.arg.empty()?"":" "+s.arg);auto ix=assignmentIndex(statement);
        if(ix==string::npos){expression(statement,env,s.line);continue;}
        string lhs=trim(statement.substr(0,ix));auto val=expression(trim(statement.substr(ix+1)),env,s.line);
        // Assign variables, object fields, and list/object indices.
        E target=ExprParser(lhs,s.line).parse();
        if(target->kind=="name") {auto it=env.find(target->text);if(it==env.end())fail("name",s.line,target->text);it->second=val;}
        else if(target->kind=="attr") {Value base=eval(target->items[0],env,s.line);auto p=get_if<shared_ptr<Value::Object>>(&base.data);if(!p)fail("runtime",s.line,"property assignment requires an object");(**p)[target->text]=val;}
        else if(target->kind=="index") {Value base=eval(target->items[0],env,s.line),index=eval(target->items[1],env,s.line);
          if(auto p=get_if<shared_ptr<Value::List>>(&base.data))(**p)[indexFrom(index,s.line,(int)(*p)->size())]=val;
          else if(auto p=get_if<shared_ptr<Value::Object>>(&base.data)){if(!holds_alternative<string>(index.data))fail("runtime",s.line,"object index must be text");(**p)[get<string>(index.data)]=val;}
          else fail("runtime",s.line,"index assignment requires a list or object");
        } else fail("syntax",s.line,"invalid assignment target");
      }
    }return {};
  }
public:
  Runtime(string lng="en",long long max=100000,vector<string> appArgs={}):applicationArgs(std::move(appArgs)),maxSteps(max),lang(std::move(lng)){}
  static string withoutBom(const string& source){
    return source.rfind("\xEF\xBB\xBF",0)==0?source.substr(3):source;
  }
  void run(const string& source,const string& sourceFilename=""){
    currentScript=sourceFilename.empty()?"main.nova":sourceFilename;
    auto tree=SourceParser(withoutBom(source)).parse();auto signal=exec(tree,globals);
    if(signal.kind!=Signal::none)fail("runtime",1,"return/break/continue outside function or loop");
  }
  void check(const string& source){(void)SourceParser(withoutBom(source)).parse();}
};
#include "nova_vm.hpp"
#include "nova_package.hpp"
static string readFile(const string& filename){
  std::ifstream f(std::filesystem::u8path(filename),ios::binary);
  if(!f)throw NovaError("Cannot open file: "+filename);
  ostringstream out;out<<f.rdbuf();
  if(f.bad())throw NovaError("Cannot read file: "+filename);
  string result=out.str();
  if(result.rfind("\xEF\xBB\xBF",0)==0)result.erase(0,3);
  return result;
}
static void usage(){cout<<"Nova Native 0.9 (Windows-first, C++17, self-hosted GUI bytecode)\n"
  <<"Usage:\n  nova-native run file.nova [--lang uk|pl|en] [--max-steps N] [-- app_args...]\n"
  <<"  nova-native check file.nova\n  nova-native exec file.nbc [--max-steps N] [-- app_args...]\n"
  <<"  nova-native pack file.nbc App.exe [--runtime nova-native.exe] [--force]\n"
  <<"  nova-native inspect App.exe\n  nova-native unpack App.exe output.nbc [--force]\n  nova-native --version\n"
  <<"Modules: storage, system, json, text, lists, sqlite, http, ui (Win32 only), local .nova imports.\n"
  <<"Remaining gaps: type, closures, full Nova 1.2 compatibility.\n";}
static int novaMain(const vector<string>& args) {
  try{
#ifdef NOVA_EMBEDDED_BYTECODE_APP
    long long maxSteps=100000;vector<string> appArgs;bool forward=false;
    for(size_t i=1;i<args.size();++i){
      if(forward){appArgs.push_back(args[i]);continue;}
      if(args[i]=="--"){forward=true;continue;}
      if(args[i]=="--max-steps"&&i+1<args.size()){
        try{maxSteps=stoll(args[++i]);}catch(...){throw NovaError("Invalid --max-steps value");}
      }else if(args[i]=="--help"||args[i]=="-h"){
        cout<<"Nova standalone bytecode application [--max-steps N] [-- app_args...]\n";return 0;
      }else {forward=true;appArgs.push_back(args[i]);}
    }
    if(maxSteps<1||maxSteps>100000000)throw NovaError("--max-steps must be 1..100000000");
    NovaBytecodeVM(maxSteps,appArgs).run(string(reinterpret_cast<const char*>(NOVA_BYTECODE_BYTES),NOVA_BYTECODE_LENGTH));
    return 0;
#elif defined(NOVA_EMBEDDED_APP)
    string lang="en";long long maxSteps=100000;vector<string> appArgs;bool forward=false;
    for(size_t i=1;i<args.size();++i){const string& arg=args[i];
      if(forward){appArgs.push_back(arg);continue;}
      if(arg=="--"){forward=true;continue;}
      if(arg=="--lang"&&i+1<args.size())lang=args[++i];
      else if(arg=="--max-steps"&&i+1<args.size()){try{maxSteps=stoll(args[++i]);}catch(...){throw NovaError("Invalid --max-steps value");}}
      else if(arg=="--help"||arg=="-h"){cout<<"Nova standalone application [--lang uk|pl|en] [--max-steps N] [-- app_args...]\n";return 0;}
      else {forward=true;appArgs.push_back(arg);}
    }
    if(lang!="en"&&lang!="uk"&&lang!="pl")throw NovaError("--lang must be en, uk or pl");
    if(maxSteps<1||maxSteps>100000000)throw NovaError("--max-steps must be 1..100000000");
    Runtime runtime(lang,maxSteps,appArgs);
    runtime.run(string(reinterpret_cast<const char*>(NOVA_EMBEDDED_CODE),NOVA_EMBEDDED_LENGTH),"main.nova");
    return 0;
#else
    // A packaged runtime executes the embedded bytecode directly, with no compiler.
    // Check the executable footer before processing CLI commands.
    nova_package::Package portable;
    if(nova_package::read(nova_package::selfExecutable(args),portable)){
      long long maxSteps=100000;vector<string> appArgs;bool forward=false;
      for(size_t i=1;i<args.size();++i){
        if(forward){appArgs.push_back(args[i]);continue;}
        if(args[i]=="--"){forward=true;continue;}
        if(args[i]=="--help"||args[i]=="-h"){
          cout<<"Nova portable application [--max-steps N] [-- app_args...]\n";return 0;
        }
        if(args[i]=="--max-steps"&&i+1<args.size()){
          try{maxSteps=stoll(args[++i]);}catch(...){throw NovaError("Invalid --max-steps value");}
        }else {forward=true;appArgs.push_back(args[i]);}
      }
      if(maxSteps<1||maxSteps>100000000)throw NovaError("--max-steps must be 1..100000000");
      NovaBytecodeVM(maxSteps,appArgs).run(portable.payload);
      return 0;
    }
    if(args.size()<2){usage();return 2;}
    if(args[1]=="pack"){
      if(args.size()<4)throw NovaError("Usage: nova-native pack input.nbc output [--runtime runner] [--force]");
      bool force=false;std::string runtime;
      for(size_t i=4;i<args.size();++i){
        if(args[i]=="--force")force=true;
        else if(args[i]=="--runtime"&&i+1<args.size())runtime=args[++i];
        else throw NovaError("Unknown pack option: "+args[i]);
      }
      if(runtime.empty())runtime=nova_package::selfExecutable(args).u8string();
      nova_package::pack(std::filesystem::u8path(args[2]),std::filesystem::u8path(args[3]),
                         std::filesystem::u8path(runtime),force);
      return 0;
    }
    if(args[1]=="inspect"){
      if(args.size()!=3)throw NovaError("Usage: nova-native inspect application");
      nova_package::printInfo(std::filesystem::u8path(args[2]));return 0;
    }
    if(args[1]=="unpack"){
      if(args.size()!=4&&(args.size()!=5||args[4]!="--force"))
        throw NovaError("Usage: nova-native unpack application output.nbc [--force]");
      auto destination=std::filesystem::u8path(args[3]);
      if(destination.extension()!=".nbc")throw NovaError("Unpack output must end in .nbc");
      nova_package::checkNotSame(std::filesystem::u8path(args[2]),destination);
      nova_package::Package packaged;
      if(!nova_package::read(std::filesystem::u8path(args[2]),packaged))
        throw NovaError("Not a Nova portable executable");
      nova_package::validateBytecode(packaged.payload);
      nova_package::save(destination,packaged.payload,args.size()==5);
      cout<<"UNPACKED: "<<destination.u8string()<<"\n";return 0;
    }
    if(args.size()<2){usage();return 2;}
    string mode=args[1],file,lang="en";long long maxSteps=100000;vector<string> appArgs;bool forward=false;
    if(mode=="--help"||mode=="-h"||mode=="--version"){usage();return 0;}
    if(mode!="run"&&mode!="check"&&mode!="exec"){mode="run";file=args[1];}
    for(size_t i=2;i<args.size();++i){const string& arg=args[i];
      if(forward){appArgs.push_back(arg);continue;}
      if(arg=="--"){forward=true;continue;}
      if(arg=="--lang"&&i+1<args.size())lang=args[++i];
      else if(arg=="--max-steps"&&i+1<args.size()){try{maxSteps=stoll(args[++i]);}catch(...){throw NovaError("Invalid --max-steps value");}}
      else if(file.empty())file=arg;
      else throw NovaError("Unknown argument: "+arg);
    }
    if(file.empty()||file.size()<(mode=="exec"?4u:5u)||file.substr(file.size()-(mode=="exec"?4:5))!=(mode=="exec"?".nbc":".nova"))throw NovaError(mode=="exec"?"Expected a .nbc bytecode file":"Expected a .nova source file");
    if(lang!="en"&&lang!="uk"&&lang!="pl")throw NovaError("--lang must be en, uk or pl");
    if(maxSteps<1||maxSteps>100000000)throw NovaError("--max-steps must be 1..100000000");
    string src=readFile(file);
    if(mode=="exec") NovaBytecodeVM(maxSteps,appArgs).run(src);
    else {
      Runtime runtime(lang,maxSteps,appArgs);
      if(mode=="check"){runtime.check(src);cout<<"OK: syntax blocks valid (expressions checked when executed)\n";}
      else runtime.run(src,std::filesystem::absolute(std::filesystem::u8path(file)).u8string());
    }
    return 0;
#endif
  }catch(const exception& e){
#if defined(_WIN32) && ((defined(NOVA_EMBEDDED_APP) && defined(NOVA_WINDOWS_GUI)) || defined(NOVA_PORTABLE_GUI))
    try {auto text=novaToWide(e.what());MessageBoxW(nullptr,text.c_str(),L"Nova: Application error",MB_OK|MB_ICONERROR);}
    catch(...) {MessageBoxA(nullptr,e.what(),"Nova: Application error",MB_OK|MB_ICONERROR);}
#else
    cerr<<"Nova Native: "<<e.what()<<'\n';
#endif
    return 1;
  }
}
#if defined(_WIN32) && ((defined(NOVA_EMBEDDED_APP) && defined(NOVA_WINDOWS_GUI)) || defined(NOVA_PORTABLE_GUI))
int WINAPI wWinMain(HINSTANCE,HINSTANCE,LPWSTR,int) {
  std::vector<std::string> args;
  int count=0;LPWSTR* words=CommandLineToArgvW(GetCommandLineW(),&count);
  if(!words){MessageBoxW(nullptr,L"Unable to read arguments",L"Nova",MB_ICONERROR);return 2;}
  try{for(int i=0;i<count;++i)args.push_back(novaToUtf8(words[i]));}
  catch(const std::exception& e){LocalFree(words);MessageBoxA(nullptr,e.what(),"Nova",MB_ICONERROR);return 2;}
  LocalFree(words);return novaMain(args);
}
#endif
#if !defined(_WIN32) || !defined(NOVA_PORTABLE_GUI)
int main(int argc,char** argv) {
  vector<string> args;
#ifdef _WIN32
  // CommandLineToArgvW retains Unicode paths that char** argv may lose in the active Windows code page.
  SetConsoleOutputCP(CP_UTF8);SetConsoleCP(CP_UTF8);
  int count=0;LPWSTR* words=CommandLineToArgvW(GetCommandLineW(),&count);
  if(!words){cerr<<"Unable to read Windows command line\n";return 2;}
  try {for(int i=0;i<count;++i)args.push_back(novaToUtf8(words[i]));}
  catch(const exception& e){LocalFree(words);cerr<<e.what()<<'\n';return 2;}
  LocalFree(words);
#else
  for(int i=0;i<argc;++i)args.emplace_back(argv[i]);
#endif
  return novaMain(args);
}

#endif // console main omitted for Nova portable Windows GUI runner
