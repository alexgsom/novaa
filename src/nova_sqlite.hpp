// Nova Native: dynamically loaded SQLite backend (no SQLite build-time dependency).
// Windows: place a trusted sqlite3.dll beside nova-native.exe / the packaged EXE.
// Linux: system libsqlite3.so.0. SQLite itself is public domain.
#pragma once
#include <cstring>
#include <map>
#include <memory>
#include <string>
#include <vector>
#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif
struct sqlite3;
struct sqlite3_stmt;
class NovaSqlite {
  void* library=nullptr;
  int nextId=1;
  std::map<int, sqlite3*> connections;
  using SqliteOpen = int(*)(const char*, sqlite3**, int, const char*);
  using SqliteClose = int(*)(sqlite3*);
  using SqliteMessage = const char*(*)(sqlite3*);
  using SqlitePrepare = int(*)(sqlite3*, const char*, int, sqlite3_stmt**, const char**);
  using SqliteFinalize = int(*)(sqlite3_stmt*);
  using SqliteStep = int(*)(sqlite3_stmt*);
  using SqliteBindDouble = int(*)(sqlite3_stmt*, int, double);
  using SqliteBindText = int(*)(sqlite3_stmt*, int, const char*, int, void(*)(void*));
  using SqliteBindNull = int(*)(sqlite3_stmt*, int);
  using SqliteBindInt64 = int(*)(sqlite3_stmt*, int, long long);
  using SqliteColumnCount = int(*)(sqlite3_stmt*);
  using SqliteColumnName = const char*(*)(sqlite3_stmt*,int);
  using SqliteColumnType = int(*)(sqlite3_stmt*,int);
  using SqliteColumnText = const unsigned char*(*)(sqlite3_stmt*,int);
  using SqliteColumnBytes = int(*)(sqlite3_stmt*,int);
  using SqliteColumnDouble = double(*)(sqlite3_stmt*,int);
  using SqliteColumnInt64 = long long(*)(sqlite3_stmt*,int);
  using SqliteChanges = int(*)(sqlite3*);
  using SqliteBusyTimeout = int(*)(sqlite3*,int);
  using SqliteBindCount = int(*)(sqlite3_stmt*);
  SqliteOpen openV2=nullptr; SqliteClose closeDb=nullptr; SqliteMessage errorMessage=nullptr;
  SqlitePrepare prepare=nullptr; SqliteFinalize finalize=nullptr;SqliteStep step=nullptr;
  SqliteBindDouble bindDouble=nullptr;SqliteBindText bindText=nullptr;SqliteBindNull bindNull=nullptr;
  SqliteBindInt64 bindInt64=nullptr;SqliteColumnCount columnCount=nullptr;SqliteColumnName columnName=nullptr;
  SqliteColumnType columnType=nullptr;SqliteColumnText columnText=nullptr;SqliteColumnBytes columnBytes=nullptr;
  SqliteColumnDouble columnDouble=nullptr;SqliteColumnInt64 columnInt64=nullptr;SqliteChanges changes=nullptr;
  SqliteBusyTimeout busyTimeout=nullptr;SqliteBindCount bindCount=nullptr;
  template<class T> T symbol(const char* name) {
#ifdef _WIN32
    auto proc=GetProcAddress(static_cast<HMODULE>(library),name);
#else
    auto proc=dlsym(library,name);
#endif
    if(!proc)throw std::runtime_error(std::string("Missing SQLite function: ")+name);
    return reinterpret_cast<T>(proc);
  }
  void load(){
    if(library)return;
#ifdef _WIN32
    // Do not search the working directory or arbitrary PATH directories for DLLs.
    // Windows 10/11 search only the executable's directory for a trusted sqlite3.dll.
    library=static_cast<void*>(LoadLibraryExW(L"sqlite3.dll",nullptr,LOAD_LIBRARY_SEARCH_APPLICATION_DIR));
#else
    library=dlopen("libsqlite3.so.0",RTLD_NOW|RTLD_LOCAL);
    if(!library)library=dlopen("libsqlite3.so",RTLD_NOW|RTLD_LOCAL);
#endif
    if(!library)throw std::runtime_error("SQLite library missing: put sqlite3.dll beside the EXE (Windows) or install libsqlite3 (Linux)");
    try {
      openV2=symbol<SqliteOpen>("sqlite3_open_v2"); closeDb=symbol<SqliteClose>("sqlite3_close");
      errorMessage=symbol<SqliteMessage>("sqlite3_errmsg");prepare=symbol<SqlitePrepare>("sqlite3_prepare_v2");
      finalize=symbol<SqliteFinalize>("sqlite3_finalize");step=symbol<SqliteStep>("sqlite3_step");
      bindDouble=symbol<SqliteBindDouble>("sqlite3_bind_double");bindText=symbol<SqliteBindText>("sqlite3_bind_text");
      bindNull=symbol<SqliteBindNull>("sqlite3_bind_null");bindInt64=symbol<SqliteBindInt64>("sqlite3_bind_int64");
      columnCount=symbol<SqliteColumnCount>("sqlite3_column_count");columnName=symbol<SqliteColumnName>("sqlite3_column_name");
      columnType=symbol<SqliteColumnType>("sqlite3_column_type");columnText=symbol<SqliteColumnText>("sqlite3_column_text");
      columnBytes=symbol<SqliteColumnBytes>("sqlite3_column_bytes");columnDouble=symbol<SqliteColumnDouble>("sqlite3_column_double");
      columnInt64=symbol<SqliteColumnInt64>("sqlite3_column_int64");changes=symbol<SqliteChanges>("sqlite3_changes");
      busyTimeout=symbol<SqliteBusyTimeout>("sqlite3_busy_timeout");bindCount=symbol<SqliteBindCount>("sqlite3_bind_parameter_count");
    }catch(...){unload();throw;}
  }
  void unload(){
    for(auto& [_,db]:connections)closeDb(db);
    connections.clear();
    if(library){
#ifdef _WIN32
      FreeLibrary(static_cast<HMODULE>(library));
#else
      dlclose(library);
#endif
    }
    library=nullptr;
  }
  sqlite3* lookup(int id){auto it=connections.find(id);if(it==connections.end())throw std::runtime_error("Invalid or closed SQLite connection");return it->second;}
  static constexpr int OK=0, ROW=100, DONE=101, INTEGER=1,FLOAT=2,TEXT=3,BLOB=4,SQLNULL=5;
public:
  NovaSqlite()=default;
  NovaSqlite(const NovaSqlite&)=delete;NovaSqlite& operator=(const NovaSqlite&)=delete;
  ~NovaSqlite(){if(library)unload();}
  int open(const std::string& filename){
    load();sqlite3* db=nullptr;
    // SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE; input uses UTF-8 even on Windows.
    int rc=openV2(filename.c_str(),&db,0x2|0x4,nullptr);
    if(rc!=OK){std::string msg=db?errorMessage(db):"cannot allocate SQLite connection";if(db)closeDb(db);throw std::runtime_error("sqlite.open: "+msg);}
    busyTimeout(db,3000);
    int id=nextId++;connections.emplace(id,db);return id;
  }
  void close(int id){sqlite3* db=lookup(id);int rc=closeDb(db);if(rc!=OK)throw std::runtime_error("sqlite.close: "+std::string(errorMessage(db)));connections.erase(id);}
  Value run(int id,const std::string& sql,const std::vector<Value>& params,bool query){
    if(sql.size()>1024*1024)throw std::runtime_error("SQL exceeds 1 MiB limit");
    sqlite3* db=lookup(id);sqlite3_stmt* stmt=nullptr;const char* tail=nullptr;
    int rc=prepare(db,sql.c_str(),-1,&stmt,&tail);
    if(rc!=OK)throw std::runtime_error("sqlite.prepare: "+std::string(errorMessage(db)));
    if(!stmt)throw std::runtime_error("Empty SQL query");
    struct Finalize {NovaSqlite* owner;sqlite3_stmt* stmt;~Finalize(){owner->finalize(stmt);}} guard{this,stmt};
    while(tail&&*tail&&std::isspace((unsigned char)*tail))++tail;
    if(tail&&*tail)throw std::runtime_error("Only one SQL statement is allowed per call");
    if(bindCount(stmt)!=(int)params.size())throw std::runtime_error("SQL parameter count mismatch");
    for(size_t i=0;i<params.size();++i){const Value& v=params[i];int at=(int)i+1;
      if(std::holds_alternative<std::monostate>(v.data))rc=bindNull(stmt,at);
      else if(auto b=std::get_if<bool>(&v.data))rc=bindInt64(stmt,at,*b?1:0);
      else if(auto n=std::get_if<double>(&v.data)){
        if(!std::isfinite(*n))throw std::runtime_error("Non-finite SQL parameter");
        rc=bindDouble(stmt,at,*n);
      }
      else if(auto s=std::get_if<std::string>(&v.data)){
        if(s->size()>4*1024*1024)throw std::runtime_error("SQL text parameter exceeds 4 MiB");
        // SQLITE_TRANSIENT (-1): sqlite3 copies bytes immediately.
        rc=bindText(stmt,at,s->c_str(),(int)s->size(),reinterpret_cast<void(*)(void*)>(-1));
      }else throw std::runtime_error("SQL parameter must be text, number, bool or null");
      if(rc!=OK)throw std::runtime_error("sqlite.bind: "+std::string(errorMessage(db)));
    }
    Value::List results;size_t bytes=0;
    while((rc=step(stmt))==ROW){
      if(!query)continue;
      if(results.size()>=10000)throw std::runtime_error("sqlite.query row limit (10000) exceeded");
      Value::Object row;
      int columns=columnCount(stmt);
      for(int c=0;c<columns;++c){
        const char* key=columnName(stmt,c);
        int type=columnType(stmt,c);
        Value val;
        if(type==INTEGER)val=Value((double)columnInt64(stmt,c));
        else if(type==FLOAT)val=Value(columnDouble(stmt,c));
        else if(type==TEXT){int count=columnBytes(stmt,c);if(count<0||count>4*1024*1024)throw std::runtime_error("SQLite text cell exceeds 4 MiB");
          bytes+=(size_t)count;if(bytes>16*1024*1024)throw std::runtime_error("SQLite result exceeds 16 MiB");
          auto p=columnText(stmt,c);
          if(!p&&count>0)throw std::runtime_error("SQLite returned invalid text");
          val=Value(std::string(p?reinterpret_cast<const char*>(p):"",count));
        }else if(type==BLOB)throw std::runtime_error("SQLite BLOB values are not supported yet");
        row[key?key:""]=val;
      }
      results.push_back(Value::object(std::move(row)));
    }
    if(rc!=DONE)throw std::runtime_error("sqlite.step: "+std::string(errorMessage(db)));
    return query?Value::list(std::move(results)):Value(changes(db));
  }
};
