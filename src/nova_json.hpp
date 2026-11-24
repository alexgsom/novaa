// Nova Native JSON module, C++17 only. Included after Value is defined.
// Explicit byte/depth limits; no third-party dependencies or Python.
#pragma once
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <limits>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>

namespace nova_json {
static constexpr size_t MAX_BYTES = 4 * 1024 * 1024;
static constexpr int MAX_DEPTH = 64;
static void addUtf8(std::string& out, uint32_t cp) {
  if(cp<=0x7f)out.push_back((char)cp);
  else if(cp<=0x7ff){out.push_back((char)(0xc0|(cp>>6)));out.push_back((char)(0x80|(cp&63)));}
  else if(cp<=0xffff){out.push_back((char)(0xe0|(cp>>12)));out.push_back((char)(0x80|((cp>>6)&63)));out.push_back((char)(0x80|(cp&63)));}
  else if(cp<=0x10ffff){out.push_back((char)(0xf0|(cp>>18)));out.push_back((char)(0x80|((cp>>12)&63)));out.push_back((char)(0x80|((cp>>6)&63)));out.push_back((char)(0x80|(cp&63)));}
  else throw std::runtime_error("JSON invalid Unicode codepoint");
}
static void validateUtf8(const std::string& text) {
  for(size_t i=0;i<text.size();){
    auto c=(unsigned char)text[i++];if(c<0x80)continue;
    int n=c>=0xc2&&c<=0xdf?1:c>=0xe0&&c<=0xef?2:c>=0xf0&&c<=0xf4?3:0;
    if(!n||i+(size_t)n>text.size())throw std::runtime_error("JSON invalid UTF-8");
    uint32_t cp=c&((1u<<(6-n))-1);
    for(int j=0;j<n;++j){auto x=(unsigned char)text[i++];if((x&0xc0)!=0x80)throw std::runtime_error("JSON invalid UTF-8");cp=(cp<<6)|(x&0x3f);}
    if((n==1&&cp<0x80)||(n==2&&cp<0x800)||(n==3&&cp<0x10000)||(cp>=0xd800&&cp<=0xdfff)||cp>0x10ffff)throw std::runtime_error("JSON invalid UTF-8");
  }
}
class Parser {
  const std::string& s;size_t i=0,nodes=0;
  [[noreturn]] void bad(const std::string& why)const {throw std::runtime_error("JSON at byte "+std::to_string(i+1)+": "+why);}
  void ws(){while(i<s.size()&&(s[i]==' '||s[i]=='\r'||s[i]=='\n'||s[i]=='\t'))++i;}
  bool take(char c){ws();if(i<s.size()&&s[i]==c){++i;return true;}return false;}
  void expect(char c){if(!take(c))bad(std::string("expected '")+c+"'");}
  int hex4(){if(i+4>s.size())bad("incomplete \\u escape");int x=0;for(int j=0;j<4;++j){char c=s[i++];x<<=4;if(c>='0'&&c<='9')x+=c-'0';else if(c>='a'&&c<='f')x+=c-'a'+10;else if(c>='A'&&c<='F')x+=c-'A'+10;else bad("invalid \\u escape");}return x;}
  std::string str(){
    expect('"');std::string result;
    while(i<s.size()){
      unsigned char c=(unsigned char)s[i++];if(c=='"'){validateUtf8(result);return result;}
      if(c<0x20)bad("unescaped control character");
      if(c!='\\'){result.push_back((char)c);continue;}
      if(i==s.size())bad("incomplete escape");
      char e=s[i++];switch(e){
        case '"':result+='"';break;case '\\':result+='\\';break;case '/':result+='/';break;
        case 'b':result+='\b';break;case 'f':result+='\f';break;case 'n':result+='\n';break;case 'r':result+='\r';break;case 't':result+='\t';break;
        case 'u':{
          int cp=hex4();if(cp>=0xd800&&cp<=0xdbff){
            if(i+2>s.size()||s.compare(i,2,"\\u")!=0)bad("missing low surrogate");
            i+=2;
            int low=hex4();if(low<0xdc00||low>0xdfff)bad("invalid low surrogate");
            cp=0x10000+((cp-0xd800)<<10)+(low-0xdc00);
          }else if(cp>=0xdc00&&cp<=0xdfff)bad("unexpected low surrogate");
          addUtf8(result,(uint32_t)cp);break;
        }
        default:bad("invalid escape");
      }
    }
    bad("unterminated string");
  }
  Value value(int depth){
    if(depth>MAX_DEPTH)bad("nesting limit exceeded");
    if(++nodes>200000)bad("node limit exceeded");
    ws();
    if(i>=s.size())bad("unexpected end");
    char c=s[i];
    if(c=='"')return Value(str());
    if(c=='['){++i;Value::List list;if(take(']'))return Value::list(std::move(list));while(true){list.push_back(value(depth+1));if(take(']'))break;expect(',');}return Value::list(std::move(list));}
    if(c=='{'){++i;Value::Object obj;if(take('}'))return Value::object(std::move(obj));while(true){ws();if(i==s.size()||s[i]!='"')bad("object key must be text");auto k=str();expect(':');auto v=value(depth+1);obj[std::move(k)]=std::move(v);if(take('}'))break;expect(',');}return Value::object(std::move(obj));}
    if(s.compare(i,4,"true")==0){i+=4;return Value(true);}
    if(s.compare(i,5,"false")==0){i+=5;return Value(false);}
    if(s.compare(i,4,"null")==0){i+=4;return Value();}
    if(c=='-'||(c>='0'&&c<='9')){
      size_t start=i;if(s[i]=='-')++i;
      if(i>=s.size())bad("invalid number");
      if(s[i]=='0')++i;else if(s[i]>='1'&&s[i]<='9'){while(i<s.size()&&s[i]>='0'&&s[i]<='9')++i;}else bad("invalid number");
      if(i<s.size()&&s[i]=='.'){++i;size_t x=i;while(i<s.size()&&s[i]>='0'&&s[i]<='9')++i;if(i==x)bad("invalid decimal");}
      if(i<s.size()&&(s[i]=='e'||s[i]=='E')){++i;if(i<s.size()&&(s[i]=='+'||s[i]=='-'))++i;size_t x=i;while(i<s.size()&&s[i]>='0'&&s[i]<='9')++i;if(i==x)bad("invalid exponent");}
      double number=0;try{number=std::stod(s.substr(start,i-start));}catch(...){bad("invalid number");}
      if(!std::isfinite(number))bad("non-finite number");
      return Value(number);
    }
    bad("unexpected character");
  }
public:
  explicit Parser(const std::string& text):s(text){if(s.size()>MAX_BYTES)throw std::runtime_error("JSON input exceeds 4 MiB");}
  Value parse(){auto v=value(0);ws();if(i!=s.size())bad("trailing data");return v;}
};
static void escapeString(const std::string& s,std::string& out){
  validateUtf8(s);out+='"';const char* digits="0123456789abcdef";
  for(unsigned char c:s){switch(c){
    case '"':out+="\\\"";break;case '\\':out+="\\\\";break;case '\b':out+="\\b";break;case '\f':out+="\\f";break;
    case '\n':out+="\\n";break;case '\r':out+="\\r";break;case '\t':out+="\\t";break;
    default:if(c<0x20){out+="\\u00";out+=digits[c>>4];out+=digits[c&15];}else out+=(char)c;
  }}out+='"';
}
class Writer {
  std::string out;std::set<const void*> visiting;bool pretty=false;
  void append(const std::string& s){out+=s;if(out.size()>MAX_BYTES)throw std::runtime_error("JSON output exceeds 4 MiB");}
  void indent(int n){if(pretty)append("\n"+std::string((size_t)(n*2),' '));}
  void write(const Value& v,int depth){
    if(depth>MAX_DEPTH)throw std::runtime_error("JSON nesting limit exceeded");
    if(std::holds_alternative<std::monostate>(v.data))append("null");
    else if(auto p=std::get_if<bool>(&v.data))append(*p?"true":"false");
    else if(auto p=std::get_if<double>(&v.data)){
      if(!std::isfinite(*p))throw std::runtime_error("JSON requires finite numbers");
      std::ostringstream ss;ss<<std::setprecision(17)<<*p;append(ss.str());
    }else if(auto p=std::get_if<std::string>(&v.data)){
      std::string s;escapeString(*p,s);append(s);
    }else if(auto p=std::get_if<std::shared_ptr<Value::List>>(&v.data)){
      const void* key=p->get();if(!visiting.insert(key).second)throw std::runtime_error("JSON cyclic list");
      append("[");for(size_t i=0;i<(*p)->size();++i){if(i)append(",");if(pretty)indent(depth+1);write((*p)->at(i),depth+1);}if(pretty&&!(*p)->empty())indent(depth);append("]");visiting.erase(key);
    }else{
      const auto& obj=std::get<std::shared_ptr<Value::Object>>(v.data);const void* key=obj.get();
      if(!visiting.insert(key).second)throw std::runtime_error("JSON cyclic object");
      append("{");bool first=true;for(auto& item:*obj){if(!first)append(",");first=false;if(pretty)indent(depth+1);std::string k;escapeString(item.first,k);append(k);append(pretty?": ":":");write(item.second,depth+1);}if(pretty&&!obj->empty())indent(depth);append("}");visiting.erase(key);
    }
  }
public:
  explicit Writer(bool formatted):pretty(formatted){}
  std::string stringify(const Value& v){write(v,0);return out;}
};
} // namespace nova_json
