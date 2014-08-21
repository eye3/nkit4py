/*
   Copyright 2014 Boris T. Darchiev (boris.darchiev@gmail.com)

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#ifndef VX_PYTHON_VAR_BUILDER_H
#define VX_PYTHON_VAR_BUILDER_H

#include "nkit/vx.h"
#include "nkit/types.h"
#include "nkit/tools.h"
#include "nkit/logger_brief.h"
#include <Python.h>
#include <string>

namespace nkit
{
  class PythonPolicy
  {
  private:
    friend class VarBuilder<PythonPolicy>;

    typedef PyObject* type;

    PythonPolicy()
      : object_(NULL)
      , dt_module_(NULL)
      , dt_(NULL)
      , utcfromtimestamp_(NULL)
      , strptime_(NULL)
    {
      datetime_init();
    }

    PythonPolicy(const PythonPolicy & from)
      : object_(from.object_)
      , dt_module_(from.dt_module_)
      , dt_(from.dt_)
      , utcfromtimestamp_(from.utcfromtimestamp_)
      , strptime_(from.strptime_)
    {
      Py_XINCREF(object_);
      Py_XINCREF(dt_module_);
      Py_XINCREF(dt_);
      Py_XINCREF(utcfromtimestamp_);
      Py_XINCREF(strptime_);
    }

    PythonPolicy & operator = (const PythonPolicy & from)
    {
      if (this != &from)
      {
        Py_CLEAR(object_);
        Py_CLEAR(dt_module_);
        Py_CLEAR(dt_);
        Py_CLEAR(utcfromtimestamp_);
        Py_CLEAR(strptime_);
        object_ = from.object_;
        dt_module_ = from.dt_module_;
        dt_ = from.dt_;
        utcfromtimestamp_ = from.utcfromtimestamp_;
        strptime_ = from.strptime_;
        Py_XINCREF(object_);
        Py_XINCREF(dt_module_);
        Py_XINCREF(dt_);
        Py_XINCREF(utcfromtimestamp_);
        Py_XINCREF(strptime_);
      }
      return *this;
    }

    ~PythonPolicy()
    {
      datetime_clear();
      Py_CLEAR(object_);
    }

    void _InitAsBoolean( std::string const & value )
    {
      int64_t i = nkit::bool_cast(value);

      Py_CLEAR(object_);
      object_ = PyBool_FromLong(i);
      assert(object_);
    }

    void _InitAsInteger( std::string const & value )
    {
      int64_t i = !value.empty() ? NKIT_STRTOLL( value.c_str(), NULL, 10 ) : 0;

      Py_CLEAR(object_);
      object_ = PyLong_FromLong(i);
      assert(object_);
    }

    void _InitAsString( std::string const & value)
    {
      Py_CLEAR(object_);
      object_ = PyString_FromString(value.c_str());
      assert(object_);
    }

    void _InitAsUndefined()
    {
      Py_CLEAR(object_);
      Py_INCREF(Py_None);
      object_ = Py_None;
    }

    void _InitAsFloatFormat( std::string const & value, const char * format )
    {
      double d(0.0);
      if (!value.empty())
      {
        if (0 == NKIT_SSCANF(value.c_str(), format, &d))
          d = 0.0;
      }

      Py_CLEAR(object_);
      object_ = PyFloat_FromDouble(d);
      assert(object_);
    }

    void _InitAsDatetimeFormat( std::string const & value,
        const char * format )
    {
      if( value.empty() )
      {
        Py_CLEAR(object_);
        object_ = PyObject_CallFunction( utcfromtimestamp_, (char*)"i", 0 );
        assert(object_);
        return;
      }

      PyObject * tmp =
        PyObject_CallFunction( strptime_, (char*)"ss", value.c_str(), format );

      if( NULL == tmp )
      {
        _InitAsUndefined();
        return;
      }

      Py_CLEAR(object_);
      object_ = tmp;
    }

    void _InitAsList()
    {
      Py_CLEAR(object_);
      object_ = PyList_New(0);
      assert(object_);
    }

    void _InitAsDict()
    {
      Py_CLEAR(object_);
      object_ = PyDict_New();
      assert(object_);
    }

    void _ListCheck()
    {
      assert(PyList_CheckExact(object_));
    }

    void _DictCheck()
    {
      assert(PyDict_CheckExact(object_));
    }

    void _AppendToList( type const & obj )
    {
      int result = PyList_Append( object_, obj );
      assert(-1 != result);
      NKIT_FORCE_USED(result)
    }

    void _SetDictKeyValue( std::string const & key, type const & var )
    {
      int result = PyDict_SetItemString( object_, key.c_str(), var );
      assert(-1 != result);
      NKIT_FORCE_USED(result)
    }

    void datetime_init()
    {
      dt_module_ = PyImport_ImportModule("datetime");
      assert(dt_module_);

      dt_ = PyObject_GetAttrString(dt_module_, "datetime");
      assert(dt_);

      utcfromtimestamp_ = PyObject_GetAttrString( dt_, "utcfromtimestamp" );
      assert(utcfromtimestamp_);

      strptime_ = PyObject_GetAttrString(dt_, "strptime");
      assert(strptime_);
    }

    void datetime_clear()
    {
      Py_CLEAR(strptime_);
      Py_CLEAR(utcfromtimestamp_);
      Py_CLEAR(dt_);
      Py_CLEAR(dt_module_);
    }

    type const & get() const
    {
      return object_;
    }

  private:
    type object_;

    PyObject * dt_module_;
    PyObject * dt_;
    PyObject * utcfromtimestamp_;
    PyObject * strptime_;
  };

  typedef VarBuilder<PythonPolicy> PythonVarBuilder;

  namespace
  {
    const char STATEMENTS[] =
            "import json\n"
            "from datetime import *\n"
            "import sys\n"
            "sys.path.append( './python' )\n"
            "from datetime_json_encoder import DatetimeEncoder\n"
            "json_obj = json.dumps( obj, indent=2, "
            "                ensure_ascii=False, cls=DatetimeEncoder )\n";

  } // namespace

  inline std::string pyobj_to_json(PythonVarBuilder::type const & obj)
  {
    PyObject * main_module = PyImport_AddModule("__main__");
    PyObject * globals = PyModule_GetDict(main_module);

    PyObject * locals = PyDict_New();

    int res = PyDict_SetItemString(locals, "obj", obj);
    if (-1 == res)
    {
      //CINFO("Coudn't set dict key");
      throw "Could not set dict key";
    }

    PyObject * run = PyRun_String(STATEMENTS, Py_file_input, globals, locals);
    if ( NULL == run)
    {
      //CINFO("Coudn't run code statements");
      throw "Could not run code statements";
    }
    Py_DECREF(run);

    char * str = PyString_AsString(PyDict_GetItemString(locals, "json_obj"));
    std::string result(str);
    Py_DECREF(locals);

    return result;
  }

} // namespace nkit

#endif // VX_PYTHON_VAR_BUILDER_H
