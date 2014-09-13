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

#include "Python.h"
#include "nkit/vx.h"
#include "nkit/types.h"
#include "nkit/tools.h"
#include "nkit/logger_brief.h"
#include <Python.h>
#include <string>

#if ((PY_MAJOR_VERSION == 2) && (PY_MINOR_VERSION <=4))
#define NKIT_PYTHON_OLDER_THEN_2_5
#endif


#if ((PY_MAJOR_VERSION == 2) && (PY_MINOR_VERSION <= 5))
#define NKIT_PYTHON_OLDER_THEN_2_6
#endif

#if NKIT_PYTHON_OLDER_THEN_2_6
#define NKIT_PYTHON_LONG_FROM_INT64(v) PyLong_FromLong(static_cast<long>(v))
#else
#define NKIT_PYTHON_LONG_FROM_INT64(v) PyLong_FromLongLong(v)
#endif


namespace nkit
{
  PyObject * py_strptime(const char * value, const char * format);
  PyObject * py_utcfromtimestamp(time_t  timestamp);
  PyObject * py_fromtimestamp(time_t  timestamp);

  class PythonPolicy
  {
  private:
    friend class VarBuilder<PythonPolicy>;

    typedef PyObject* type;

    PythonPolicy()
      : object_(NULL)
    {}

    PythonPolicy(const PythonPolicy & from)
      : object_(from.object_)
    {
      Py_XINCREF(object_);
    }

    PythonPolicy & operator = (const PythonPolicy & from)
    {
      if (this != &from)
      {
        Py_CLEAR(object_);
        object_ = from.object_;
        Py_XINCREF(object_);
      }
      return *this;
    }

    ~PythonPolicy()
    {
      Py_CLEAR(object_);
    }

    void _InitAsBoolean( std::string const & value )
    {
      int32_t i = nkit::bool_cast(value);

      Py_CLEAR(object_);
      object_ = PyBool_FromLong(i);
      assert(object_);
    }

    void _InitAsInteger( std::string const & value )
    {
      int64_t i = !value.empty() ? NKIT_STRTOLL( value.c_str(), NULL, 10 ) : 0;

      Py_CLEAR(object_);
      object_ = NKIT_PYTHON_LONG_FROM_INT64(i);
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
#if defined(_WIN32) || defined(_WIN64)
      struct tm _tm =
        { 0, 0, 0, 0, 0, 0, 0, 0, 0};
#else
      struct tm _tm =
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
#endif
      if (value.empty() || NKIT_STRPTIME(value.c_str(), format, &_tm) == NULL)
      {
        _InitAsUndefined();
        return;
      }

      time_t time = mktime(&_tm);
      Py_CLEAR(object_);
      object_ = py_fromtimestamp(time);
      assert(object_);
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

    type const & get() const
    {
      return object_;
    }

    std::string ToString() const;

  private:
    type object_;

  };

  typedef VarBuilder<PythonPolicy> PythonVarBuilder;

  std::string pyobj_to_json(PythonVarBuilder::type const & obj);

} // namespace nkit

//------------------------------------------------------------------------------
typedef nkit::PythonVarBuilder VarBuilder;

/// exception
static PyObject * PythonXml2VarBuilderError;

//------------------------------------------------------------------------------
template<typename T>
struct SharedPtrHolder
{
  SharedPtrHolder(NKIT_SHARED_PTR(T) & ptr) : ptr_(ptr) {}
  NKIT_SHARED_PTR(T) ptr_;
};

//------------------------------------------------------------------------------
struct PythonXml2VarBuilder
{
  PyObject_HEAD;
  SharedPtrHolder< nkit::Xml2VarBuilder< VarBuilder > > * gen_;
};

//------------------------------------------------------------------------------
static PyObject* CreatePythonXml2VarBuilder(
    PyTypeObject * type, PyObject * args, PyObject *)
{
  const char* target_spec = NULL;
  int result = PyArg_ParseTuple( args, "s", &target_spec );
  if(!result)
  {
    PyErr_SetString( PythonXml2VarBuilderError, "Expected string arguments" );
    return NULL;
  }
  if( !target_spec || !*target_spec )
  {
    PyErr_SetString(
        PythonXml2VarBuilderError, "Parameter string must not be empty" );
    return NULL;
  }

  PythonXml2VarBuilder * self =
      (PythonXml2VarBuilder *)type->tp_alloc( type, 0 );

  if( NULL != self )
  {
    std::string error("");
    nkit::Xml2VarBuilder< VarBuilder >::Ptr builder =
        nkit::Xml2VarBuilder< VarBuilder >::Create( target_spec, &error );
    if(!builder)
    {
      PyErr_SetString( PythonXml2VarBuilderError, error.c_str() );
      return NULL;
    }
    self->gen_ =
        new SharedPtrHolder< nkit::Xml2VarBuilder< VarBuilder > >(builder);
  }

  return (PyObject *)self;
}

//------------------------------------------------------------------------------
static void DeletePythonXml2VarBuilder(PyObject * self)
{
  SharedPtrHolder< nkit::Xml2VarBuilder< VarBuilder > > * ptr =
        ((PythonXml2VarBuilder *)self)->gen_;
  if (ptr)
    delete ptr;
  self->ob_type->tp_free(self);
}

//------------------------------------------------------------------------------
static PyObject * feed_method( PyObject * self, PyObject * args )
{
  const char* request = NULL;
  Py_ssize_t size = 0;
  int result = PyArg_ParseTuple( args, "s#", &request, &size );
  if(!result)
  {
    PyErr_SetString( PythonXml2VarBuilderError, "Expected string arguments" );
    return NULL;
  }
  if( !request || !*request || !size )
  {
    PyErr_SetString(
        PythonXml2VarBuilderError, "Parameter string must not be empty" );
    return NULL;
  }

  nkit::Xml2VarBuilder< VarBuilder >::Ptr builder =
      ((PythonXml2VarBuilder *)self)->gen_->ptr_;

  std::string error("");
  if(!builder->Feed( request, size, false, &error ))
  {
    PyErr_SetString( PythonXml2VarBuilderError, error.c_str() );
    return NULL;
  }

  Py_RETURN_NONE;
}

//------------------------------------------------------------------------------
static PyObject * end_method( PyObject * self, PyObject * /*args*/ )
{
  nkit::Xml2VarBuilder< VarBuilder >::Ptr builder =
        ((PythonXml2VarBuilder *)self)->gen_->ptr_;

  std::string empty("");
  std::string error("");
  if(!builder->Feed( empty.c_str(), empty.size(), true, &error ))
  {
    PyErr_SetString( PythonXml2VarBuilderError, error.c_str() );
    return NULL;
  }

  PyObject * result = builder->var();
  Py_XINCREF(result);
  return result;
}

//------------------------------------------------------------------------------
static PyMethodDef xml2var_methods[] =
{
  { "feed", feed_method, METH_VARARGS, "Usage: builder.feed()\n"
    "Invoke \"Feed\" function\n"
    "Returns None\n" },
  { "end", end_method, METH_VARARGS, "Usage: builder.end()\n"
    "Invoke \"End\" function\n"
    "Returns PyObject\n" },
  { NULL, NULL, 0, NULL } /* Sentinel */
};

//------------------------------------------------------------------------------
static PyTypeObject PythonXml2VarBuilderType =
{
  PyVarObject_HEAD_INIT(NULL, 0)
  "nkit4py.Xml2VarBuilder", /*tp_name*/
  sizeof(PythonXml2VarBuilder), /*tp_basicsize*/
  0, /*tp_itemsize*/
  DeletePythonXml2VarBuilder, /*tp_dealloc*/
  0, /*tp_print*/
  0, /*tp_getattr*/
  0, /*tp_setattr*/
  0, /*tp_compare*/
  0, /*tp_repr*/
  0, /*tp_as_number*/
  0, /*tp_as_sequence*/
  0, /*tp_as_mapping*/
  0, /*tp_hash */
  0, /*tp_call*/
  0, /*tp_str*/
  0, /*tp_getattro*/
  0, /*tp_setattro*/
  0, /*tp_as_buffer*/
  Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
  "XML to object or list converter and filter", /* tp_doc */
  0,//tp_traverse
  0,//tp_clear,
  0,//tp_richcompare,
  0,//tp_weaklistoffset,
  0,//tp_iter,
  0,//tp_iternext,
  xml2var_methods,//tp_methods,
  0,//tp_members,
  0,//tp_getset,
  0,//tp_base,
  0,//tp_dict,
  0,//tp_descr_get,
  0,//tp_descr_set,
  0,//tp_dictoffset,
  0,//tp_init,
  0,//tp_alloc,
  CreatePythonXml2VarBuilder,//tp_new,
};

//------------------------------------------------------------------------------
static PyMethodDef ModuleMethods[] =
{
  { NULL, NULL, 0, NULL } /* Sentinel */
};

#ifndef PyMODINIT_FUNC  /* declarations for DLL import/export */
#define PyMODINIT_FUNC void
#endif

//------------------------------------------------------------------------------
namespace nkit
{
  static PyObject * dt_module_;
  static PyObject * dt_;
  static PyObject * utcfromtimestamp_;
  static PyObject * fromtimestamp_;
  static PyObject * strptime_;

  PyObject * py_utcfromtimestamp(time_t  timestamp)
  {
    return PyObject_CallFunction(utcfromtimestamp_, (char*)"i", timestamp);
  }

  PyObject * py_fromtimestamp(time_t  timestamp)
  {
    return PyObject_CallFunction(fromtimestamp_, (char*)"i", timestamp);
  }

  PyObject * py_strptime(const char * value, const char * format)
  {
    return PyObject_CallFunction(strptime_, (char*)"ss", value, format);
  }

  const char STATEMENTS[] =
            "import json\n"
            "from datetime import *\n"
            "import sys\n"
            "sys.path.append( './python' )\n"
            "from datetime_json_encoder import DatetimeEncoder\n"
            "json_obj = json.dumps( obj, indent=2, "
            "                ensure_ascii=False, cls=DatetimeEncoder )\n";


  std::string pyobj_to_json(PythonVarBuilder::type const & obj)
  {
    PyObject * main_module = PyImport_AddModule("__main__");
    PyObject * globals = PyModule_GetDict(main_module);

    PyObject * locals = PyDict_New();

    int res = PyDict_SetItemString(locals, "obj", obj);
    if (-1 == res)
    {
      Py_DECREF(locals);
      throw "Could not set dict key";
    }

    PyObject * run = PyRun_String(STATEMENTS, Py_file_input, globals, locals);
    if ( NULL == run)
    {
      Py_DECREF(locals);
      throw "Could not run code statements";
    }
    Py_DECREF(run);

    char * str = PyString_AsString(PyDict_GetItemString(locals, "json_obj"));
    std::string result(str);
    Py_DECREF(locals);

    return result;
  }

  std::string PythonPolicy::ToString() const
  {
    return pyobj_to_json(object_);
  }

} // namespace nkit

//------------------------------------------------------------------------------
PyMODINIT_FUNC initnkit4py(void)
{
  if( -1 == PyType_Ready(&PythonXml2VarBuilderType) )
    return;

  PyObject * module = Py_InitModule( "nkit4py", ModuleMethods );
  if( NULL == module )
    return;

  PythonXml2VarBuilderError =
      PyErr_NewException( (char *)"Xml2VarBuilder.Error", NULL, NULL );
  Py_INCREF(PythonXml2VarBuilderError);
  PyModule_AddObject( module, "Error", PythonXml2VarBuilderError );

  Py_INCREF(&PythonXml2VarBuilderType);
  PyModule_AddObject( module,
      "Xml2VarBuilder", (PyObject *)&PythonXml2VarBuilderType );

  nkit::dt_module_ = PyImport_ImportModule("datetime");
  assert(nkit::dt_module_);

  nkit::dt_ = PyObject_GetAttrString(nkit::dt_module_, "datetime");
  assert(nkit::dt_);

  nkit::utcfromtimestamp_ = PyObject_GetAttrString(nkit::dt_, "utcfromtimestamp");
  assert(nkit::utcfromtimestamp_);

  nkit::fromtimestamp_ = PyObject_GetAttrString(nkit::dt_, "fromtimestamp");
  assert(nkit::fromtimestamp_);

  nkit::strptime_ = PyObject_GetAttrString(nkit::dt_, "strptime");
  assert(nkit::strptime_);
}
