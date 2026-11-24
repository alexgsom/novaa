// Nova Native portable package format v1 (C++17, no Python, compiler-free packaging).
// Container = unmodified native runtime + Nova bytecode JSON + 32-byte footer.
// Footer: 16-byte magic, 64-bit little-endian payload length, 64-bit FNV-1a checksum.
// The checksum detects accidental corruption; it is NOT a signature/authentication scheme.
#pragma once
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#ifdef __linux__
#include <unistd.h>
#endif

namespace nova_package {
namespace fs=std::filesystem;
static constexpr const char* MAGIC="NOVA_BYTECODE_08"; // exactly 16 bytes
static constexpr size_t FOOTER=32;
static constexpr size_t MAX_PAYLOAD=4u*1024u*1024u;
struct Package { std::string payload; uint64_t checksum=0; uint64_t runnerSize=0; };
static uint64_t hash(const std::string& s) {
  uint64_t h=14695981039346656037ULL;
  for(unsigned char ch:s){h^=ch;h*=1099511628211ULL;}
  return h;
}
static void writeU64(std::ostream& out,uint64_t n){
  for(int i=0;i<8;++i)out.put((char)((n>>(8*i))&0xff));
}
static uint64_t readU64(const char* ptr){
  uint64_t n=0;for(int i=0;i<8;++i)n|=uint64_t((unsigned char)ptr[i])<<(8*i);return n;
}
static bool read(const fs::path& executable,Package& result) {
  std::ifstream f(executable,std::ios::binary|std::ios::ate);
  if(!f)throw NovaError("Cannot inspect executable: "+executable.u8string());
  auto end=f.tellg();
  if(end<0)throw NovaError("Cannot determine executable size");
  uint64_t total=(uint64_t)end;
  if(total<FOOTER)return false;
  f.seekg((std::streamoff)(total-FOOTER));
  std::array<char,FOOTER> footer{};
  if(!f.read(footer.data(),FOOTER))throw NovaError("Cannot read Nova package footer");
  if(std::memcmp(footer.data(),MAGIC,16)!=0)return false;
  uint64_t length=readU64(footer.data()+16), checksum=readU64(footer.data()+24);
  if(length==0||length>MAX_PAYLOAD||length>total-FOOTER)
    throw NovaError("Invalid Nova package length (corrupt or unsupported executable)");
  f.seekg((std::streamoff)(total-FOOTER-length));
  result.payload.resize((size_t)length);
  if(!f.read(&result.payload[0],(std::streamsize)length))throw NovaError("Cannot read embedded Nova bytecode");
  if(hash(result.payload)!=checksum)throw NovaError("Nova package checksum mismatch (corrupt executable)");
  result.checksum=checksum;
  result.runnerSize=total-FOOTER-length;
  return true;
}
static fs::path selfExecutable(const std::vector<std::string>& args){
#ifdef _WIN32
  std::wstring path(260,L'\0');
  for(;;){
    DWORD len=GetModuleFileNameW(nullptr,&path[0],(DWORD)path.size());
    if(len==0)throw NovaError("Cannot find the running Nova executable");
    if(len<path.size()){path.resize(len);return fs::path(path);}
    if(path.size()>=32768)throw NovaError("Windows executable path too long");
    path.resize(path.size()*2);
  }
#elif defined(__linux__)
  try{return fs::read_symlink("/proc/self/exe");}
  catch(const fs::filesystem_error&){
    if(!args.empty())return fs::absolute(fs::u8path(args[0]));
    throw NovaError("Cannot locate Nova runtime executable");
  }
#else
  if(!args.empty())return fs::absolute(fs::u8path(args[0]));
  throw NovaError("Specify --runtime path for this platform");
#endif
}
static bool validateBytecode(const std::string& data){
  if(data.empty()||data.size()>MAX_PAYLOAD)throw NovaError("Nova bytecode must be between 1 byte and 4 MiB");
  Value root=nova_json::Parser(data).parse();
  auto p=std::get_if<std::shared_ptr<Value::Object>>(&root.data);
  if(!p)throw NovaError("Bytecode must be a JSON object");
  const auto& obj=**p;
  auto format=obj.find("format"),version=obj.find("version"),code=obj.find("instructions");
  if(format==obj.end()||!std::holds_alternative<std::string>(format->second.data)||
      std::get<std::string>(format->second.data)!="nova-bytecode")throw NovaError("Invalid Nova bytecode format");
  if(version==obj.end()||!version->second.number()||std::get<double>(version->second.data)!=1)
    throw NovaError("Unsupported Nova bytecode version");
  if(code==obj.end()||!std::holds_alternative<std::shared_ptr<Value::List>>(code->second.data))
    throw NovaError("Bytecode instructions must be a list");
  // Reject malformed opcodes before creating a runnable file. The VM repeats
  // its stricter checks (operand types, jump bounds, function definitions).
  const auto& instructions=*std::get<std::shared_ptr<Value::List>>(code->second.data);
  if(instructions.size()>100000)throw NovaError("Bytecode instruction limit exceeded");
  static const std::set<std::string> noArg={"print","add","sub","mul","div","mod","neg","eq","ne","lt","le","gt","ge","not","and","or","to_str","to_int","repeat_count","halt","pop","ret","assert","raise","dup","index","set_index"};
  static const std::set<std::string> oneArg={"push","load","store","jump","jump_false","jump_true","iter_start","iter_end","import","get_attr","set_attr"};
  for(size_t i=0;i<instructions.size();++i){
    const auto* entry=std::get_if<std::shared_ptr<Value::List>>(&instructions[i].data);
    if(!entry||(*entry)->empty()||!std::holds_alternative<std::string>((**entry)[0].data))
      throw NovaError("Invalid bytecode instruction at index "+std::to_string(i));
    const auto& op=std::get<std::string>((**entry)[0].data);
    size_t expected=noArg.count(op)?1:oneArg.count(op)?2:
        (op=="call"?3:(op=="make_list"||op=="make_object")?2:
        (op=="function"||op=="iter_next")?4:0);
    if(expected==0||(*entry)->size()!=expected)
      throw NovaError("Unknown opcode or argument count at instruction "+std::to_string(i)+": "+op);
  }
  return true;
}
static std::string platform(const fs::path& runtime){
  std::ifstream f(runtime,std::ios::binary);
  if(!f)throw NovaError("Cannot open runtime: "+runtime.u8string());
  unsigned char signature[4]{};
  if(!f.read((char*)signature,4))throw NovaError("Runtime is too short");
  if(signature[0]=='M'&&signature[1]=='Z'){
    f.seekg(0x3c);
    char offset[4]{};
    if(!f.read(offset,4))throw NovaError("Invalid Windows PE header");
    uint32_t peOffset=(uint32_t)(unsigned char)offset[0]|
      ((uint32_t)(unsigned char)offset[1]<<8)|((uint32_t)(unsigned char)offset[2]<<16)|
      ((uint32_t)(unsigned char)offset[3]<<24);
    auto size=fs::file_size(runtime);
    if(peOffset<64||peOffset>size||size-peOffset<6)throw NovaError("Invalid Windows PE offset");
    // Validate PE/COFF + PE32+ Optional Header rather than trusting 'MZ' and
    // the PE signature alone. A signed PE must be signed *after* packaging.
    f.seekg(peOffset);char pe[24]{};
    if(!f.read(pe,24)||std::memcmp(pe,"PE\0\0",4)!=0)throw NovaError("Not a Windows PE executable");
    uint16_t machine=(unsigned char)pe[4]|((uint16_t)(unsigned char)pe[5]<<8);
    if(machine!=0x8664)throw NovaError("Nova portable Windows runtime must be x64 (AMD64)");
    uint16_t sectionCount=(unsigned char)pe[6]|((uint16_t)(unsigned char)pe[7]<<8);
    uint16_t optionalSize=(unsigned char)pe[20]|((uint16_t)(unsigned char)pe[21]<<8);
    uint16_t characteristics=(unsigned char)pe[22]|((uint16_t)(unsigned char)pe[23]<<8);
    if(sectionCount==0||sectionCount>96||optionalSize<70||optionalSize>4096||
       !(characteristics&0x0002)||(characteristics&0x2000)||
       (uint64_t)peOffset+24+optionalSize>size)
      throw NovaError("Invalid Windows PE executable header");
    char option[70]{};
    if(!f.read(option,70))throw NovaError("Truncated Windows PE optional header");
    uint16_t peMagic=(unsigned char)option[0]|((uint16_t)(unsigned char)option[1]<<8);
    uint16_t subsystem=(unsigned char)option[68]|((uint16_t)(unsigned char)option[69]<<8);
    if(peMagic!=0x020b||(subsystem!=2&&subsystem!=3))
      throw NovaError("Nova runtime must be a PE32+ console or Windows GUI EXE");
    return "Windows PE x64";
  }
  if(signature[0]==0x7f&&signature[1]=='E'&&signature[2]=='L'&&signature[3]=='F')return "Linux ELF";
  throw NovaError("Runtime must be a Windows PE or Linux ELF executable");
}
static void checkNotSame(const fs::path& from,const fs::path& to){
  std::error_code ec;
  if(fs::exists(from)&&fs::exists(to)&&fs::equivalent(from,to,ec)&&!ec)
    throw NovaError("Output must not overwrite input or runtime");
}
static void save(const fs::path& output,const std::string& payload,bool force){
  if(fs::exists(output)&&!force)throw NovaError("Output exists; use --force to replace it");
  fs::path temp=output;temp+=".nova-writing";
  if(fs::exists(temp))throw NovaError("Temporary output exists: "+temp.u8string());
  try {
    {
      std::ofstream out(temp,std::ios::binary|std::ios::trunc);
      if(!out)throw NovaError("Cannot create output: "+temp.u8string());
      out.write(payload.data(),(std::streamsize)payload.size());
      if(!out)throw NovaError("Cannot write output");
    }
    if(force&&fs::exists(output))fs::remove(output);
    fs::rename(temp,output);
  }catch(...){std::error_code ec;fs::remove(temp,ec);throw;}
}
static void pack(const fs::path& input,const fs::path& output,const fs::path& runtime,bool force){
  checkNotSame(runtime,output);checkNotSame(input,output);
  std::string os=platform(runtime);
  if(os=="Linux ELF"&&output.extension()==".exe")
    throw NovaError("Cannot create Windows EXE from Linux ELF; supply a prebuilt Windows --runtime");
  if(os=="Windows PE x64"&&output.extension()!=".exe")
    throw NovaError("Windows output must end with .exe");
  if(fs::file_size(runtime)>128u*1024u*1024u)
    throw NovaError("Runtime exceeds 128 MiB size limit");
  // Make accidental packing of a non-Nova binary far less likely.
  {
    std::ifstream binary(runtime,std::ios::binary);
    std::string haystack((std::istreambuf_iterator<char>(binary)),std::istreambuf_iterator<char>());
    if(haystack.find(MAGIC)==std::string::npos)
      throw NovaError("Provided --runtime is not Nova Native 0.8 (package marker not found)");
  }
  Package existing;
  if(read(runtime,existing))throw NovaError("Runtime is already packaged; use a clean nova-native binary");
  if(input.extension()!=".nbc")throw NovaError("Pack input must end in .nbc");
  std::string payload=readFile(input.u8string());
  validateBytecode(payload);
  if(fs::exists(output)&&!force)throw NovaError("Output exists; use --force to replace it");
  fs::path temp=output;temp+=".nova-writing";
  if(fs::exists(temp))throw NovaError("Temporary output exists: "+temp.u8string());
  try{
    fs::copy_file(runtime,temp,fs::copy_options::none);
    {
      std::ofstream out(temp,std::ios::binary|std::ios::app);
      if(!out)throw NovaError("Cannot append to runtime");
      out.write(payload.data(),(std::streamsize)payload.size());
      out.write(MAGIC,16);
      writeU64(out,(uint64_t)payload.size());
      writeU64(out,hash(payload));
      out.flush();
      if(!out)throw NovaError("Failed writing Nova package");
    }
    Package verified;
    if(!read(temp,verified)||verified.payload!=payload)throw NovaError("Nova package integrity verification failed");
    if(force&&fs::exists(output))fs::remove(output);
    fs::rename(temp,output);
  }catch(...){std::error_code ec;fs::remove(temp,ec);throw;}
  std::cout<<"PACKED: "<<output.u8string()<<" ("<<os<<", "<<payload.size()<<" byte bytecode)\n";
}
static void printInfo(const fs::path& input){
  Package p;
  if(!read(input,p))throw NovaError("Not a Nova portable executable: "+input.u8string());
  validateBytecode(p.payload);
  std::cout<<"Nova Portable 0.8\nPlatform: "<<platform(input)<<"\nRuntime bytes: "<<p.runnerSize
     <<"\nBytecode bytes: "<<p.payload.size()<<"\nChecksum FNV64: "<<std::hex<<p.checksum<<std::dec<<"\nIntegrity: OK\n";
}
} // namespace nova_package
