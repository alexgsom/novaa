// Nova experimental bytecode VM v1, extended with named functions and call frames.
// C++ is the stage-0 VM; the source-to-bytecode compiler is authored in Nova.
// JSON opcodes are validated before execution, and every instruction counts toward the step budget.
class NovaBytecodeVM {
  struct Function {size_t entry; std::vector<std::string> parameters;};
  struct Iterator {std::vector<Value> values;size_t offset=0;};
  using IteratorMap=std::unordered_map<std::string,Iterator>;
  struct Frame {size_t returnPc,stackBase;std::unordered_map<std::string,Value> locals;IteratorMap iterations;};
  long long maxSteps;
  Runtime moduleRuntime;
  NovaEnv moduleEnv;
  static bool standardModule(const std::string& name){
    return name=="storage"||name=="system"||name=="json"||name=="text"||name=="lists"||name=="sqlite"||name=="http"||name=="ui";
  }

  std::unordered_map<std::string,Value> globals;
  IteratorMap globalIterations;
  std::unordered_map<std::string,Function> functions;
  std::vector<Frame> frames;
  std::vector<Value> stack;
  static std::runtime_error error(size_t pc,const std::string& why) {
    return std::runtime_error("Bytecode error (instruction " + std::to_string(pc) + "): " + why);
  }
  static const Value::Object& obj(const Value& v) {
    auto x=std::get_if<std::shared_ptr<Value::Object>>(&v.data);
    if(!x)throw std::runtime_error("Bytecode header must be an object");
    return **x;
  }
  static const Value::List& list(const Value& v) {
    auto x=std::get_if<std::shared_ptr<Value::List>>(&v.data);
    if(!x)throw std::runtime_error("Bytecode instructions must be an array");
    return **x;
  }
  static const std::string& str(const Value& v,size_t pc) {
    auto x=std::get_if<std::string>(&v.data);
    if(!x)throw error(pc,"expected text operand");
    return *x;
  }
  Value pop(size_t pc) {
    if(stack.empty())throw error(pc,"stack underflow");
    Value v=stack.back();stack.pop_back();return v;
  }
  void push(const Value& v,size_t pc) {
    if(stack.size()>=10000)throw error(pc,"stack limit exceeded");
    stack.push_back(v);
  }
  static double number(const Value& v,size_t pc) {
    if(!v.number())throw error(pc,"expected numeric value");
    return std::get<double>(v.data);
  }
  static size_t destination(const Value& operand,size_t pc,size_t codeSize) {
    if(!operand.number())throw error(pc,"jump must have numeric destination");
    double target=std::get<double>(operand.data);
    if(!std::isfinite(target)||std::floor(target)!=target||target<0||target>(double)codeSize)
      throw error(pc,"jump target out of bounds");
    return (size_t)target;
  }
  static size_t argumentCount(const Value& operand,size_t pc) {
    if(!operand.number())throw error(pc,"argument count must be a number");
    double n=std::get<double>(operand.data);
    if(!std::isfinite(n)||std::floor(n)!=n||n<0||n>64)
      throw error(pc,"argument count must be 0..64");
    return (size_t)n;
  }
  static Value toInteger(const Value& val,size_t pc) {
    double n=0;
    if(val.number())n=std::get<double>(val.data);
    else if(auto s=std::get_if<std::string>(&val.data)){
      try{size_t consumed=0;n=std::stod(*s,&consumed);if(consumed!=s->size())throw std::invalid_argument("extra");}
      catch(...){throw error(pc,"cannot convert text to integer");}
    }else throw error(pc,"cannot convert to integer");
    if(!std::isfinite(n))throw error(pc,"non-finite integer");
    return Value(std::trunc(n));
  }
  Value builtinCall(const std::string& name,const std::vector<Value>& args,size_t pc) {
    if(name=="range"){
      if(args.empty()||args.size()>3)throw error(pc,"range expects 1..3 integer arguments");
      std::vector<long long> ints;
      for(const Value& v:args){
        double n=number(v,pc);
        if(!std::isfinite(n)||std::floor(n)!=n||n<-100000000||n>100000000)
          throw error(pc,"range expects bounded integer arguments");
        ints.push_back((long long)n);
      }
      long long start=0,stop=ints[0],step=1;
      if(ints.size()>=2){start=ints[0];stop=ints[1];}
      if(ints.size()==3)step=ints[2];
      if(step==0)throw error(pc,"range step cannot be zero");
      Value::List out;
      for(long long x=start;step>0?x<stop:x>stop;x+=step){
        if(out.size()>=100000)throw error(pc,"range result exceeds element limit");
        out.emplace_back((double)x);
      }
      return Value::list(std::move(out));
    }
    if(args.size()!=1)throw error(pc,"builtin "+name+" expects one argument");
    const Value& v=args[0];
    if(name=="str")return Value(v.repr());
    if(name=="int")return toInteger(v,pc);
    if(name=="float"){
      if(v.number())return v;
      if(auto s=std::get_if<std::string>(&v.data)){
        try {size_t consumed=0;double n=std::stod(*s,&consumed);if(consumed!=s->size()||!std::isfinite(n))throw std::invalid_argument("extra");return Value(n);}
        catch(...) {throw error(pc,"cannot convert text to float");}
      }
      throw error(pc,"cannot convert to float");
    }
    if(name=="len"){
      if(auto s=std::get_if<std::string>(&v.data))return Value((int)utf8Chars(*s).size());
      if(auto l=std::get_if<std::shared_ptr<Value::List>>(&v.data))return Value((int)(*l)->size());
      if(auto o=std::get_if<std::shared_ptr<Value::Object>>(&v.data))return Value((int)(*o)->size());
      throw error(pc,"len expects text, list, or object");
    }
    throw error(pc,"unknown builtin: "+name);
  }
public:
  explicit NovaBytecodeVM(long long limit, std::vector<std::string> args={}):maxSteps(limit),moduleRuntime("en",limit,std::move(args)){}
  void run(const std::string& contents) {
    if(contents.size()>4*1024*1024)throw std::runtime_error("Bytecode file exceeds 4 MiB");
    globals.clear();globalIterations.clear();functions.clear();frames.clear();stack.clear();
    Value document=nova_json::Parser(contents).parse();
    const auto& root=obj(document);
    auto get=[&](const std::string& key)->const Value&{
      auto it=root.find(key);if(it==root.end())throw std::runtime_error("Bytecode header missing "+key);return it->second;
    };
    if(str(get("format"),0)!="nova-bytecode")throw std::runtime_error("Invalid bytecode format");
    if(!get("version").number()||std::get<double>(get("version").data)!=1)
      throw std::runtime_error("Unsupported bytecode version");
    const auto& code=list(get("instructions"));
    if(code.size()>100000)throw std::runtime_error("Bytecode exceeds instruction limit");
    static const std::set<std::string> bare={"print","add","sub","mul","div","mod","neg","eq","ne","lt","le","gt","ge","not","and","or","to_str","to_int","repeat_count","halt","pop","ret","assert","raise","dup","index","set_index"};
    static const std::set<std::string> unary={"push","load","store","jump","jump_false","jump_true","iter_start","iter_end","import","get_attr","set_attr"};
    for(size_t pc=0;pc<code.size();++pc){
      const auto& ins=list(code[pc]);
      if(ins.empty())throw error(pc,"empty instruction");
      const auto& op=str(ins[0],pc);
      size_t required=bare.count(op)?1:unary.count(op)?2:(op=="call"||op=="make_list"||op=="make_object")?(op=="call"?3:2):(op=="function"||op=="iter_next")?4:0;
      if(!required||ins.size()!=required)throw error(pc,"unknown opcode or incorrect operand count: "+op);
      if(op=="push"){
        if(std::holds_alternative<std::shared_ptr<Value::List>>(ins[1].data)||
           std::holds_alternative<std::shared_ptr<Value::Object>>(ins[1].data))throw error(pc,"push supports scalar literals only");
      }
      if(op=="load"||op=="store"||op=="iter_start"||op=="iter_end"||op=="import"||op=="get_attr"||op=="set_attr")str(ins[1],pc);
      if(op=="iter_next"){str(ins[1],pc);str(ins[2],pc);destination(ins[3],pc,code.size());}
      if(op=="jump"||op=="jump_false"||op=="jump_true")destination(ins[1],pc,code.size());
      if(op=="call"){str(ins[1],pc);argumentCount(ins[2],pc);}
      if(op=="make_list"||op=="make_object")argumentCount(ins[1],pc);
      if(op=="function"){
        const auto& name=str(ins[1],pc);
        if(name.empty()||functions.count(name))throw error(pc,"duplicate or empty function name");
        const auto& params=list(ins[2]);
        if(params.size()>64)throw error(pc,"too many function parameters");
        std::vector<std::string> names;std::set<std::string> used;
        for(const auto& arg:params){const auto& n=str(arg,pc);if(n.empty()||!used.insert(n).second)throw error(pc,"duplicate or empty parameter");names.push_back(n);}
        size_t after=destination(ins[3],pc,code.size());
        if(after<=pc+1)throw error(pc,"empty or invalid function body");
        functions.emplace(name,Function{pc+1,std::move(names)});
      }
    }
    long long steps=0;size_t pc=0;
    execute(code,pc,steps);
  }
private:
  // This same VM executes both the initial program and Win32 event callbacks.
  // The shared counter prevents callbacks from resetting the execution budget.
  void execute(const Value::List& code,size_t& pc,long long& steps) {
    while(pc<code.size()){
      if(++steps>maxSteps)throw error(pc,"execution step limit exceeded");
      const auto& ins=list(code[pc]);const std::string& op=str(ins[0],pc);
      if(op=="halt")break;
      if(op=="function"){pc=destination(ins[3],pc,code.size());continue;}
      if(op=="push"){push(ins[1],pc);++pc;continue;}
      if(op=="dup"){
        if(stack.empty())throw error(pc,"stack underflow");
        Value v=stack.back();push(v,pc);++pc;continue;
      }
      if(op=="import"){
        const auto& name=str(ins[1],pc);
        if(!standardModule(name))throw error(pc,"unsupported bytecode import: "+name);
        globals[name]=Value::object({{"__nova_module",Value(name)}});
        ++pc;continue;
      }
      if(op=="load"){
        const auto& name=str(ins[1],pc);const Value* val=nullptr;
        if(!frames.empty()) {auto it=frames.back().locals.find(name);if(it!=frames.back().locals.end())val=&it->second;}
        if(!val){auto it=globals.find(name);if(it!=globals.end())val=&it->second;}
        if(!val){
          auto fn=functions.find(name);
          if(fn!=functions.end()){
            push(Value::object({{"__nova_function",Value(name)}}),pc);
            ++pc;continue;
          }
          throw error(pc,"undefined variable: "+name);
        }
        push(*val,pc);++pc;continue;
      }
      if(op=="store"){
        const auto& name=str(ins[1],pc);Value val=pop(pc);
        auto& vars=frames.empty()?globals:frames.back().locals;
        if(vars.size()>=10000&&!vars.count(name))throw error(pc,"variable limit exceeded");
        vars[name]=val;++pc;continue;
      }
      if(op=="iter_start"){
        const auto& name=str(ins[1],pc);Value source=pop(pc);
        Iterator state;
        if(auto ptr=std::get_if<std::shared_ptr<Value::List>>(&source.data))state.values=**ptr;
        else if(auto ptr=std::get_if<std::shared_ptr<Value::Object>>(&source.data)){
          for(const auto& item:**ptr)state.values.emplace_back(item.first);
        }else if(auto ptr=std::get_if<std::string>(&source.data)){
          for(const auto& ch:utf8Chars(*ptr))state.values.emplace_back(ch);
        }else throw error(pc,"for expects list, object, or text");
        if(state.values.size()>100000)throw error(pc,"iteration exceeds element limit");
        auto& iter=frames.empty()?globalIterations:frames.back().iterations;
        iter[name]=std::move(state);++pc;continue;
      }
      if(op=="iter_next"){
        const auto& name=str(ins[1],pc);const auto& variable=str(ins[2],pc);
        auto& iter=frames.empty()?globalIterations:frames.back().iterations;
        auto it=iter.find(name);if(it==iter.end())throw error(pc,"iterator not found: "+name);
        if(it->second.offset>=it->second.values.size()){
          pc=destination(ins[3],pc,code.size());continue;
        }
        Value item=it->second.values[it->second.offset++];
        auto& vars=frames.empty()?globals:frames.back().locals;
        if(vars.size()>=10000&&!vars.count(variable))throw error(pc,"variable limit exceeded");
        vars[variable]=item;++pc;continue;
      }
      if(op=="iter_end"){
        auto& iter=frames.empty()?globalIterations:frames.back().iterations;
        iter.erase(str(ins[1],pc));++pc;continue;
      }
      if(op=="make_list"){
        size_t count=argumentCount(ins[1],pc);
        if(count>stack.size())throw error(pc,"stack underflow in list literal");
        Value::List values(count);
        for(size_t i=count;i>0;--i)values[i-1]=pop(pc);
        push(Value::list(std::move(values)),pc);++pc;continue;
      }
      if(op=="make_object"){
        size_t count=argumentCount(ins[1],pc);
        if(count>stack.size()/2)throw error(pc,"stack underflow in object literal");
        Value::Object fields;
        for(size_t i=0;i<count;++i){
          Value v=pop(pc),key=pop(pc);
          if(!std::holds_alternative<std::string>(key.data))throw error(pc,"object key must be text");
          fields[std::get<std::string>(key.data)]=v;
        }
        push(Value::object(std::move(fields)),pc);++pc;continue;
      }
      if(op=="get_attr"){
        Value base=pop(pc);const auto& name=str(ins[1],pc);
        auto object=std::get_if<std::shared_ptr<Value::Object>>(&base.data);
        if(!object)throw error(pc,"property read expects an object");
        auto it=(*object)->find(name);
        if(it==(*object)->end())throw error(pc,"missing property: "+name);
        push(it->second,pc);++pc;continue;
      }
      if(op=="set_attr"){
        Value value=pop(pc),base=pop(pc);const auto& name=str(ins[1],pc);
        auto object=std::get_if<std::shared_ptr<Value::Object>>(&base.data);
        if(!object)throw error(pc,"property assignment expects an object");
        (**object)[name]=value;++pc;continue;
      }
      if(op=="set_index"){
        Value value=pop(pc),subscript=pop(pc),base=pop(pc);
        if(auto object=std::get_if<std::shared_ptr<Value::Object>>(&base.data)){
          (**object)[str(subscript,pc)]=value;++pc;continue;
        }
        if(auto seq=std::get_if<std::shared_ptr<Value::List>>(&base.data)){
          double raw=number(subscript,pc);
          if(!std::isfinite(raw)||std::floor(raw)!=raw||raw<-100000000||raw>100000000)
            throw error(pc,"index must be a valid integer");
          long long i=(long long)raw;if(i<0)i+=(long long)(*seq)->size();
          if(i<0||i>=(long long)(*seq)->size())throw error(pc,"index out of range");
          (**seq)[(size_t)i]=value;++pc;continue;
        }
        throw error(pc,"index assignment expects a list or object");
      }
      if(op=="index"){
        Value subscript=pop(pc),base=pop(pc);
        if(auto object=std::get_if<std::shared_ptr<Value::Object>>(&base.data)){
          const auto& key=str(subscript,pc);
          auto it=(*object)->find(key);
          if(it==(*object)->end())throw error(pc,"missing object key: "+key);
          push(it->second,pc);++pc;continue;
        }
        double raw=number(subscript,pc);
        if(!std::isfinite(raw)||std::floor(raw)!=raw||raw<-100000000||raw>100000000)
          throw error(pc,"index must be a valid integer");
        if(auto listValue=std::get_if<std::shared_ptr<Value::List>>(&base.data)){
          long long i=(long long)raw;if(i<0)i+=(long long)(*listValue)->size();
          if(i<0||i>=(long long)(*listValue)->size())throw error(pc,"index out of range");
          push((*listValue)->at((size_t)i),pc);++pc;continue;
        }
        if(auto textValue=std::get_if<std::string>(&base.data)){
          auto chars=utf8Chars(*textValue);long long i=(long long)raw;
          if(i<0)i+=(long long)chars.size();
          if(i<0||i>=(long long)chars.size())throw error(pc,"index out of range");
          push(Value(chars[(size_t)i]),pc);++pc;continue;
        }
        throw error(pc,"indexing requires list, object or text");
      }
      if(op=="call"){
        const auto& name=str(ins[1],pc);size_t count=argumentCount(ins[2],pc);
        if(count>stack.size())throw error(pc,"stack underflow in call");
        std::vector<Value> args(count);
        for(size_t i=count;i>0;--i)args[i-1]=pop(pc);
        if(name=="str"||name=="int"||name=="float"||name=="len"||name=="range"){
          push(builtinCall(name,args,pc),pc);++pc;continue;
        }
        auto dot=name.find('.');
        if(dot!=std::string::npos){
          auto mod=name.substr(0,dot),method=name.substr(dot+1);
          // UI-control object sugar: label.set("Hello") / input.get().
          // The compiler emits dotted calls and the VM resolves the receiver.
          if(!standardModule(mod)&&globals.count("ui")&&(method=="get"||method=="set")){
            const Value* receiver=nullptr;
            if(!frames.empty()){
              auto it=frames.back().locals.find(mod);
              if(it!=frames.back().locals.end())receiver=&it->second;
            }
            if(!receiver){auto it=globals.find(mod);if(it!=globals.end())receiver=&it->second;}
            if(receiver){
              if((method=="get"&&!args.empty())||(method=="set"&&args.size()!=1))
                throw error(pc,"UI get/set argument count mismatch");
              std::vector<Value> params;params.push_back(*receiver);
              params.insert(params.end(),args.begin(),args.end());
              try {push(moduleRuntime.invokeModule("ui",method,params,moduleEnv,0),pc);}
              catch(const std::exception& e){throw error(pc,e.what());}
              ++pc;continue;
            }
          }
          if(!standardModule(mod)||!globals.count(mod)||method.empty()||method.find('.')!=std::string::npos)
            throw error(pc,"module not imported or unsupported method: "+name);
          if(mod=="ui"){
#ifndef _WIN32
            throw error(pc,"native ui is currently available on Windows only");
#else
            auto callbackName=[&](const Value& v)->std::string{
              if(auto text=std::get_if<std::string>(&v.data))return *text;
              if(auto object=std::get_if<std::shared_ptr<Value::Object>>(&v.data)){
                auto it=(*object)->find("__nova_function");
                if(it!=(*object)->end())return str(it->second,pc);
              }
              throw error(pc,"UI callback must be a Nova function or its name");
            };
            auto checkedCallback=[&](const Value& v)->std::string{
              std::string cb=callbackName(v);
              auto it=functions.find(cb);
              if(it==functions.end()||!it->second.parameters.empty())
                throw error(pc,"UI callback must be a declared zero-argument Nova function: "+cb);
              return cb;
            };
            if(method=="button"){
              if(args.size()!=3)throw error(pc,"ui.button expects window, text, callback");
              auto cb=checkedCallback(args[2]);
              int id=moduleRuntime.gui.button(Runtime::requireHandle(args[0],0),Runtime::requireText(args[1],0,"text"),cb);
              push(moduleRuntime.uiValue(id),pc);++pc;continue;
            }
            if(method=="menu"){
              if(args.size()!=4)throw error(pc,"ui.menu expects window, group, text, callback");
              auto cb=checkedCallback(args[3]);
              int id=moduleRuntime.gui.menu(Runtime::requireHandle(args[0],0),
                Runtime::requireText(args[1],0,"group"),Runtime::requireText(args[2],0,"text"),cb);
              push(moduleRuntime.uiValue(id),pc);++pc;continue;
            }
            if(method=="run"){
              if(args.size()!=1)throw error(pc,"ui.run expects one window");
              int window=Runtime::requireHandle(args[0],0);
              moduleRuntime.gui.run(window,[&](const std::string& callback){
                auto it=functions.find(callback);
                if(it==functions.end()||!it->second.parameters.empty())
                  throw error(pc,"UI callback no longer exists: "+callback);
                const size_t stackBefore=stack.size(),framesBefore=frames.size();
                // A sentinel return address exits the nested VM execution on ret.
                frames.push_back(Frame{code.size(),stackBefore,{}, {}});
                size_t callbackPc=it->second.entry;
                try{
                  execute(code,callbackPc,steps);
                  if(frames.size()!=framesBefore||stack.size()!=stackBefore+1)
                    throw error(pc,"UI callback did not return normally");
                  stack.pop_back();
                }catch(...){
                  // Win32 closes the window and propagates callback exceptions.
                  frames.resize(framesBefore);stack.resize(stackBefore);throw;
                }
              });
              push(Value(),pc);++pc;continue;
            }
#endif
          }
          try {push(moduleRuntime.invokeModule(mod,method,args,moduleEnv,0),pc);}
          catch(const std::exception& e){throw error(pc,e.what());}
          ++pc;continue;
        }
        auto it=functions.find(name);if(it==functions.end())throw error(pc,"undefined function: "+name);
        if(it->second.parameters.size()!=count)throw error(pc,"incorrect argument count for "+name);
        if(frames.size()>=128)throw error(pc,"call depth limit exceeded");
        Frame frame{pc+1,stack.size(),{}, {}};
        for(size_t i=0;i<count;++i)frame.locals[it->second.parameters[i]]=args[i];
        frames.push_back(std::move(frame));pc=it->second.entry;continue;
      }
      if(op=="ret"){
        if(frames.empty())throw error(pc,"return outside a function");
        Value result=pop(pc);
        Frame frame=std::move(frames.back());frames.pop_back();
        if(stack.size()<frame.stackBase)throw error(pc,"corrupt call stack");
        stack.resize(frame.stackBase);push(result,pc);pc=frame.returnPc;continue;
      }
      if(op=="jump"){pc=destination(ins[1],pc,code.size());continue;}
      if(op=="jump_false"||op=="jump_true"){
        bool truth=pop(pc).truth();
        pc=((op=="jump_false"&&!truth)||(op=="jump_true"&&truth))?destination(ins[1],pc,code.size()):pc+1;
        continue;
      }
      if(op=="print"){std::cout<<pop(pc).repr()<<'\n';++pc;continue;}
      if(op=="pop"){pop(pc);++pc;continue;}
      if(op=="to_str"){push(Value(pop(pc).repr()),pc);++pc;continue;}
      if(op=="to_int"){push(toInteger(pop(pc),pc),pc);++pc;continue;}
      if(op=="assert"){
        if(!pop(pc).truth())throw error(pc,"assertion failed");
        ++pc;continue;
      }
      if(op=="raise")throw error(pc,"raised: "+pop(pc).repr());
      if(op=="repeat_count"){
        Value val=pop(pc);double n=number(val,pc);
        if(!std::isfinite(n)||std::floor(n)!=n||n<0||n>100000)
          throw error(pc,"repeat requires an integer from 0 to 100000");
        push(val,pc);++pc;continue;
      }
      if(op=="not"){push(Value(!pop(pc).truth()),pc);++pc;continue;}
      if(op=="neg"){auto v=pop(pc);push(Value(-number(v,pc)),pc);++pc;continue;}
      Value right=pop(pc),left=pop(pc);
      if(op=="and"){push(Value(left.truth()&&right.truth()),pc);}
      else if(op=="or"){push(Value(left.truth()||right.truth()),pc);}
      else if(op=="eq"){push(Value(equalValue(left,right)),pc);}
      else if(op=="ne"){push(Value(!equalValue(left,right)),pc);}
      else if(op=="add"&&std::holds_alternative<std::string>(left.data)&&std::holds_alternative<std::string>(right.data)){
        push(Value(std::get<std::string>(left.data)+std::get<std::string>(right.data)),pc);
      } else {
        if((op=="lt"||op=="gt"||op=="le"||op=="ge")&&
          std::holds_alternative<std::string>(left.data)&&std::holds_alternative<std::string>(right.data)) {
          const auto& a=std::get<std::string>(left.data);const auto& b=std::get<std::string>(right.data);
          push(Value(op=="lt"?a<b:op=="gt"?a>b:op=="le"?a<=b:a>=b),pc);
        }else{
          double a=number(left,pc),b=number(right,pc);
          if(op=="add")push(Value(a+b),pc);
          else if(op=="sub")push(Value(a-b),pc);
          else if(op=="mul")push(Value(a*b),pc);
          else if(op=="div"||op=="mod"){
            if(b==0)throw error(pc,"division by zero");
            push(Value(op=="div"?a/b:a-std::floor(a/b)*b),pc);
          }
          else if(op=="lt")push(Value(a<b),pc);
          else if(op=="le")push(Value(a<=b),pc);
          else if(op=="gt")push(Value(a>b),pc);
          else if(op=="ge")push(Value(a>=b),pc);
          else throw error(pc,"invalid operation");
        }
      }
      ++pc;
    }
  }
};
