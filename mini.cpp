
// include chugin header
#include "chugin.h"
#include "eval.h"

// general includes
#include <iostream>
#include <string>

// declaration of chugin constructor
CK_DLL_CTOR(mini_ctor);
// declaration of chugin desctructor
CK_DLL_DTOR(mini_dtor);

// parse methods
CK_DLL_MFUN(mini_parse);
CK_DLL_MFUN(mini_parse_cycle);
CK_DLL_MFUN(mini_parse_arc);

// for chugins extending UGen, this is mono synthesis function for 1 sample
CK_DLL_TICK(mini_tick);

// this is a special offset reserved for chugin internal data
t_CKINT mini_data_offset = 0;


class mini {
public:
  // constructor
  mini(t_CKFLOAT fs) { m_param = 0; }

  // parse a mini-notation string and return formatted event list
  std::string parse(const std::string &input) { return parse_string(input); }

  // parse with explicit arc (for arbitrary time windows)
  std::string parse(const std::string &input, double start, double end) { return parse_string(input, start, end); }

private:
  // instance data
  t_CKFLOAT m_param;
};

//-----------------------------------------------------------------------------
// info function: ChucK calls this when loading/probing the chugin
// NOTE: please customize these info fields below; they will be used for
// chugins loading, probing, and package management and documentation
//-----------------------------------------------------------------------------
CK_DLL_INFO(mini) {
  // the version string of this chugin, e.g., "v1.2.1"
  QUERY->setinfo(QUERY, CHUGIN_INFO_CHUGIN_VERSION, "");
  // the author(s) of this chugin, e.g., "Alice Baker & Carl Donut"
  QUERY->setinfo(QUERY, CHUGIN_INFO_AUTHORS, "");
  // text description of this chugin; what is it? what does it do? who is it
  // for?
  QUERY->setinfo(QUERY, CHUGIN_INFO_DESCRIPTION, "");
  // (optional) URL of the homepage for this chugin
  QUERY->setinfo(QUERY, CHUGIN_INFO_URL, "");
  // (optional) contact email
  QUERY->setinfo(QUERY, CHUGIN_INFO_EMAIL, "");
}

//-----------------------------------------------------------------------------
// query function: ChucK calls this when loading the chugin
// modify this function to define this chugin's API and language extensions
//-----------------------------------------------------------------------------
CK_DLL_QUERY(mini) {
  // generally, don't change this...
  QUERY->setname(QUERY, "mini");

  // ---------------------------------
  QUERY->begin_class(QUERY, "mini", "Object");

  // register default constructor
  QUERY->add_ctor(QUERY, mini_ctor);
  // NOTE constructors can be overloaded like any other functions,
  // each overloaded constructor begins with `QUERY->add_ctor()`
  // followed by a sequence of `QUERY->add_arg()`

  // register the destructor (probably no need to change)
  QUERY->add_dtor(QUERY, mini_dtor);

  // parse method: takes a string, returns a string
  QUERY->add_mfun(QUERY, mini_parse, "string", "parse");
  QUERY->add_arg(QUERY, "string", "input");

  // parse with arc: takes a string and start/end floats, returns a string
  QUERY->add_mfun(QUERY, mini_parse_arc, "string", "parse");
  QUERY->add_arg(QUERY, "string", "input");
  QUERY->add_arg(QUERY, "float", "start");
  QUERY->add_arg(QUERY, "float", "end");

  // this reserves a variable in the ChucK internal class to store
  // referene to the c++ class we defined above
  mini_data_offset = QUERY->add_mvar(QUERY, "int", "@m_data", false);

  // ------------------------------------------------------------------------
  // end the class definition
  // IMPORTANT: this MUST be called to each class definition!
  // ------------------------------------------------------------------------
  QUERY->end_class(QUERY);

  // wasn't that a breeze?
  return TRUE;
}

// implementation for the default constructor
CK_DLL_CTOR(mini_ctor) {
  // get the offset where we'll store our internal c++ class pointer
  OBJ_MEMBER_INT(SELF, mini_data_offset) = 0;

  // instantiate our internal c++ class representation
  mini *m_obj = new mini(API->vm->srate(VM));

  // store the pointer in the ChucK object member
  OBJ_MEMBER_INT(SELF, mini_data_offset) = (t_CKINT)m_obj;
}

// implementation for the destructor
CK_DLL_DTOR(mini_dtor) {
  // get our c++ class pointer
  mini *m_obj = (mini *)OBJ_MEMBER_INT(SELF, mini_data_offset);
  // clean up (this macro tests for NULL, deletes, and zeros out the variable)
  CK_SAFE_DELETE(m_obj);
  // set the data field to 0
  OBJ_MEMBER_INT(SELF, mini_data_offset) = 0;
}

// implementation for parse method
CK_DLL_MFUN(mini_parse) {
  // get our c++ class pointer
  mini *m_obj = (mini *)OBJ_MEMBER_INT(SELF, mini_data_offset);

  // get the input string argument
  Chuck_String *ck_str = GET_NEXT_STRING(ARGS);
  std::string input = ck_str ? API->object->str(ck_str) : "";

  // parse and get result string
  std::string result = m_obj->parse(input);

  // return as ChucK string
  RETURN->v_string = API->object->create_string(VM, result.c_str(), false);
}

// implementation for arc-aware parse method
CK_DLL_MFUN(mini_parse_arc) {
  mini *m_obj = (mini *)OBJ_MEMBER_INT(SELF, mini_data_offset);

  Chuck_String *ck_str = GET_NEXT_STRING(ARGS);
  std::string input = ck_str ? API->object->str(ck_str) : "";
  t_CKFLOAT start = GET_NEXT_FLOAT(ARGS);
  t_CKFLOAT end = GET_NEXT_FLOAT(ARGS);

  std::string result = m_obj->parse(input, (double)start, (double)end);

  RETURN->v_string = API->object->create_string(VM, result.c_str(), false);
}
