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
#include "nkit/types.h"
#include "nkit/tools.h"
#include "nkit/logger_brief.h"
#include "nkit/xml2var.h"
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
  static PyObject * main_module_;
  static PyObject * dt_module_;
  static PyObject * dt_;
  static PyObject * fromtimestamp_;
  static PyObject * traceback_module_;
  static PyObject * traceback_dict_;
  static PyObject * traceback_format_exception_;
  static PyObject * traceback_format_exception_only_;
  static PyObject * json_module_;
  static PyObject * json_dumps_;
  static PyObject * string_module_;
  static PyObject * string_dict_;
  static PyObject * string_join_fields_;
  static PyObject * datetime_json_encoder_;

  bool unicode_to_string(PyObject * unicode, std::string * out,
      std::string * error)
  {
    if (PyUnicode_CheckExact(unicode))
    {
      PyObject * tmp = PyUnicode_AsUTF8String(unicode);
      out->assign(PyString_AsString(tmp));
      Py_DECREF(tmp);
    }
    else if (PyString_CheckExact(unicode))
      out->assign(PyString_AsString(unicode));
    else
    {
      *error = "Expected unicode or string";
      return false;
    }

    return true;
  }

  PyObject * py_fromtimestamp(time_t  timestamp)
  {
    return PyObject_CallFunction(fromtimestamp_, (char*)"i", timestamp);
  }

  bool pyobj_to_json(PyObject * obj, std::string * out, std::string * error)
  {
    PyObject * args = PyTuple_New(1);
    Py_INCREF(obj);
    PyTuple_SetItem(args, 0, obj);

    PyObject * kw = PyDict_New();

    PyObject * indent = PyInt_FromSize_t(2);
    PyDict_SetItemString(kw, "indent", indent);
    Py_DECREF(indent);

    PyObject * ensure_ascii = PyBool_FromLong(0);
    PyDict_SetItemString(kw, "ensure_ascii", ensure_ascii);
    Py_DECREF(ensure_ascii);

    PyDict_SetItemString(kw, "cls", datetime_json_encoder_);

    PyObject * result = PyObject_Call(json_dumps_, args, kw);
    bool ret = unicode_to_string(result, out, error);
    Py_DECREF(result);
    Py_DECREF(args);
    Py_DECREF(kw);

    return ret;
  }

  class PythonPolicy: Uncopyable
  {
  private:
    friend class VarBuilder<PythonPolicy>;

    typedef PyObject* type;

    PythonPolicy(const detail::Options & options)
      : object_(NULL)
      , options_(options)
    {}

//    PythonPolicy(const PythonPolicy & from)
//      : object_(from.object_)
//      , options_(from.options_)
//    {
//      Py_XINCREF(object_);
//    }
//
//    PythonPolicy & operator = (const PythonPolicy & from)
//    {
//      if (this != &from)
//      {
//        Py_CLEAR(object_);
//        object_ = from.object_;
//        Py_XINCREF(object_);
//      }
//      return *this;
//    }
//
    ~PythonPolicy()
    {
      Py_CLEAR(object_);
    }

    void InitAsBoolean( std::string const & value )
    {
      int32_t i = nkit::bool_cast(value);

      Py_CLEAR(object_);
      object_ = PyBool_FromLong(i);
      assert(object_);
    }

    void InitAsInteger( std::string const & value )
    {
      int64_t i = !value.empty() ? NKIT_STRTOLL( value.c_str(), NULL, 10 ) : 0;

      Py_CLEAR(object_);
      object_ = NKIT_PYTHON_LONG_FROM_INT64(i);
      assert(object_);
    }

    void InitAsString( std::string const & value)
    {
      Py_CLEAR(object_);
      if (options_.unicode_)
        object_ = PyUnicode_FromStringAndSize(value.data(), value.size());
      else
        object_ = PyString_FromStringAndSize(value.data(), value.size());
      assert(object_);
    }

    void InitAsUndefined()
    {
      Py_CLEAR(object_);
      Py_INCREF(Py_None);
      object_ = Py_None;
    }

    void InitAsFloatFormat( std::string const & value, const char * format )
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

    void InitAsDatetimeFormat( std::string const & value,
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
        InitAsUndefined();
        return;
      }

      time_t time = mktime(&_tm);
      Py_CLEAR(object_);
      object_ = py_fromtimestamp(time);
      assert(object_);
    }

    void InitAsList()
    {
      Py_CLEAR(object_);
      object_ = PyList_New(0);
      assert(object_);
    }

    void InitAsDict()
    {
      Py_CLEAR(object_);
      object_ = PyDict_New();
      assert(object_);
    }

    void ListCheck()
    {
      assert(PyList_CheckExact(object_));
    }

    void DictCheck()
    {
      assert(PyDict_CheckExact(object_));
    }

    void AppendToList( type const & obj )
    {
      int result = PyList_Append( object_, obj );
      assert(-1 != result);
      NKIT_FORCE_USED(result)
    }

    void SetDictKeyValue( std::string const & key, type const & var )
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
    const detail::Options & options_;
  };

  typedef VarBuilder<PythonPolicy> PythonVarBuilder;

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
  SharedPtrHolder< nkit::Xml2VarBuilder< VarBuilder > > * holder_;
};

//------------------------------------------------------------------------------
bool parse_dict(PyObject * dict, std::string * out, std::string * error)
{
  if (PyDict_CheckExact(dict))
    return nkit::pyobj_to_json(dict, out, error);
  else
    return nkit::unicode_to_string(dict, out, error);
}

//------------------------------------------------------------------------------
static PyObject* CreatePythonXml2VarBuilder(
    PyTypeObject * type, PyObject * args, PyObject *)
{
  PyObject * dict1 = NULL;
  PyObject * dict2 = NULL;
  int result = PyArg_ParseTuple(args, "O|O", &dict1, &dict2);
  if(!result)
  {
    PyErr_SetString(PythonXml2VarBuilderError,
        "Expected one or two arguments:"
        " 1) mappings or 2) options and mappings");
    return NULL;
  }

  PyObject * options_dict = dict2 ? dict1 : NULL;
  PyObject * mapping_dict = dict2 ? dict2 : dict1;

  std::string options, error;
  if (!options_dict)
    options = "{}";
  else if (!parse_dict(options_dict, &options, &error))
  {
    PyErr_SetString( PythonXml2VarBuilderError,
        ("Options parameter must be JSON-string or dictionary: " +
        error).c_str());
    return NULL;
  }

  if (options.empty())
  {
    PyErr_SetString(
        PythonXml2VarBuilderError,
        "Options parameter must be dict or JSON object" );
    return NULL;
  }

  std::string mappings;
  if (!parse_dict(mapping_dict, &mappings, &error))
  {
    PyErr_SetString( PythonXml2VarBuilderError,
        ("Mappings parameter must be JSON-string or dictionary: " +
        error).c_str());
    return NULL;
  }

  if(mappings.empty())
  {
    PyErr_SetString(
        PythonXml2VarBuilderError,
        "Mappings parameter must be dict or JSON object" );
    return NULL;
  }

  PythonXml2VarBuilder * self =
      (PythonXml2VarBuilder *)type->tp_alloc( type, 0 );
  if (!self)
  {
    PyErr_SetString(PythonXml2VarBuilderError, "Low memory");
    return NULL;
  }

  nkit::Xml2VarBuilder< VarBuilder >::Ptr builder =
      nkit::Xml2VarBuilder< VarBuilder >::Create(options, mappings, &error);
  if(!builder)
  {
    PyErr_SetString( PythonXml2VarBuilderError, error.c_str() );
    return NULL;
  }
  self->holder_ =
      new SharedPtrHolder< nkit::Xml2VarBuilder< VarBuilder > >(builder);

  return (PyObject *)self;
}

//------------------------------------------------------------------------------
static void DeletePythonXml2VarBuilder(PyObject * self)
{
  SharedPtrHolder< nkit::Xml2VarBuilder< VarBuilder > > * ptr =
        ((PythonXml2VarBuilder *)self)->holder_;
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
        PythonXml2VarBuilderError, "Parameter must not be empty string" );
    return NULL;
  }

  nkit::Xml2VarBuilder< VarBuilder >::Ptr builder =
      ((PythonXml2VarBuilder *)self)->holder_->ptr_;

  std::string error("");
  if(!builder->Feed( request, size, false, &error ))
  {
    PyErr_SetString( PythonXml2VarBuilderError, error.c_str() );
    return NULL;
  }

  Py_RETURN_NONE;
}

//------------------------------------------------------------------------------
static PyObject * get_method( PyObject * self, PyObject * args )
{
  const char* mapping_name = NULL;
  int result = PyArg_ParseTuple( args, "s", &mapping_name );
  if(!result)
  {
    PyErr_SetString( PythonXml2VarBuilderError, "Expected string argument" );
    return NULL;
  }
  if( !mapping_name || !*mapping_name )
  {
    PyErr_SetString(
        PythonXml2VarBuilderError, "Mapping name must not be empty" );
    return NULL;
  }

  nkit::Xml2VarBuilder< VarBuilder >::Ptr builder =
      ((PythonXml2VarBuilder *)self)->holder_->ptr_;

  PyObject * item = builder->var(mapping_name);
  Py_INCREF(item);
  return item;
}

//------------------------------------------------------------------------------
static PyObject * end_method( PyObject * self, PyObject * /*args*/ )
{
  nkit::Xml2VarBuilder< VarBuilder >::Ptr builder =
        ((PythonXml2VarBuilder *)self)->holder_->ptr_;

  std::string empty("");
  std::string error("");
  if(!builder->Feed( empty.c_str(), empty.size(), true, &error ))
  {
    PyErr_SetString( PythonXml2VarBuilderError, error.c_str() );
    return NULL;
  }

  nkit::StringList mapping_names(builder->mapping_names());

  PyObject * result = PyDict_New();
  nkit::StringList::const_iterator mapping_name = mapping_names.begin(),
      end = mapping_names.end();
  for (; mapping_name != end; ++mapping_name)
  {
    PyObject * item = builder->var(*mapping_name);
    PyDict_SetItemString(result, mapping_name->c_str(), item);
  }
  return result;
}

//------------------------------------------------------------------------------
static PyMethodDef xml2var_methods[] =
{
  { "feed", feed_method, METH_VARARGS, "Usage: builder.feed()\n"
    "Invoke \"Feed\" function\n"
    "Returns None\n" },
  { "get", get_method, METH_VARARGS, "Usage: builder.get()\n"
	"Returns result by mapping name\n" },
  { "end", end_method, METH_VARARGS, "Usage: builder.end()\n"
	"Returns Dict: results for all mappings\n" },
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
  std::string get_python_error()
  {
    std::string error;

    PyObject *exc_obj = NULL;
    PyObject *exc_str = NULL;
    PyObject *exc_traceback = NULL;
    PyObject *lines = NULL;
    PyObject *exception_string = NULL;

    if (PyErr_Occurred() == NULL)
    return std::string("");

    PyErr_Fetch(&exc_obj, &exc_str, &exc_traceback);

    if (exc_obj == NULL)
    {
      error.append("Fatal error in 'GetPythonError': ExcObj == NULL");
    }
    else if (exc_str == NULL)
    {
      error.append("Fatal error in 'GetPythonError': ExcStr == NULL");
    }
    else
    {
      if (exc_traceback == NULL)
        lines = PyObject_CallFunction(
          traceback_format_exception_only_,
            const_cast<char*>("OO"), exc_obj, exc_str);
      else
        lines = PyObject_CallFunction(
          traceback_format_exception_,
          const_cast<char*>("OOO"), exc_obj, exc_str, exc_traceback);

      if (lines == NULL)
      {
        error.append("traceback.formatexception error");
      }
      else
      {
        exception_string = PyObject_CallFunction(
            string_join_fields_, const_cast<char*>("Os"), lines, "");

        if (exception_string == NULL)
          error.append("string.joinfields error");
        else
          error.append(PyString_AsString(exception_string));
      }
    }

    Py_XDECREF(lines);
    Py_XDECREF(exception_string);
    Py_XDECREF(exc_obj);
    Py_XDECREF(exc_str);
    Py_XDECREF(exc_traceback);

    PyErr_Clear();

    return std::string(error);
  }

  std::string PythonPolicy::ToString() const
  {
    std::string ret, error;
    try
    {
      nkit::pyobj_to_json(object_, &ret, &error);
      return ret;
    }
    catch (...)
    {
      return std::string("");
    }
  }

  const char DATETIME_JSON_ENCODER_CLASS[] =
        "class DatetimeJSONEncoder(nkit_tmp_json.JSONEncoder):\n"
        "    _datetime = nkit_tmp_datetime\n"
        "    _traceback = nkit_tmp_traceback\n"
        "    _json = nkit_tmp_json\n"
        "    def default( self, obj ):\n"
        "        try:\n"
        "            if isinstance(obj, DatetimeJSONEncoder._datetime.datetime):\n"
        "                return obj.strftime(\"%Y-%m-%d %H:%M:%S\")\n"
        "            if isinstance(obj, DatetimeJSONEncoder._datetime.date):\n"
        "                return obj.strftime(\"%Y-%m-%d\")\n"
        "            if isinstance(obj, DatetimeJSONEncoder._datetime.time):\n"
        "                return obj.strftime(\"%H:%M:%S\")\n"
        "        except Exception:\n"
        "            # text = DatetimeJSONEncoder._traceback.format_exc()\n"
        "            return ''\n"
        "        return DatetimeJSONEncoder._json.JSONEncoder.default( self, obj )\n"
        ;

} // namespace nkit

//------------------------------------------------------------------------------
PyMODINIT_FUNC initnkit4py(void)
{
  if( -1 == PyType_Ready(&PythonXml2VarBuilderType) )
    return;

  PyObject * module = Py_InitModule("nkit4py", ModuleMethods);
  if( NULL == module )
    return;

  PythonXml2VarBuilderError =
      PyErr_NewException( (char *)"Xml2VarBuilder.Error", NULL, NULL );

  Py_INCREF(PythonXml2VarBuilderError);
  PyModule_AddObject( module, "Error", PythonXml2VarBuilderError );

  Py_INCREF(&PythonXml2VarBuilderType);
  PyModule_AddObject( module,
      "Xml2VarBuilder", (PyObject *)&PythonXml2VarBuilderType );

  nkit::traceback_module_ = PyImport_ImportModule("traceback");
  assert(nkit::traceback_module_);
  Py_INCREF(nkit::traceback_module_);

  nkit::traceback_dict_ = PyModule_GetDict(nkit::traceback_module_);
  Py_INCREF(nkit::traceback_dict_);

  nkit::traceback_format_exception_ =
      PyDict_GetItemString(nkit::traceback_dict_, "format_exception");
  Py_INCREF(nkit::traceback_format_exception_);

  nkit::traceback_format_exception_only_ =
      PyDict_GetItemString(nkit::traceback_dict_, "format_exception_only");
  Py_INCREF(nkit::traceback_format_exception_only_);

  nkit::string_module_ = PyImport_ImportModule("string");
  assert(nkit::string_module_);
  Py_INCREF(nkit::string_module_);

  nkit::string_dict_ = PyModule_GetDict(nkit::string_module_);
  Py_INCREF(nkit::string_dict_);

  nkit::string_join_fields_ =
      PyDict_GetItemString(nkit::string_dict_, "joinfields");
  Py_INCREF(nkit::string_join_fields_);

  nkit::json_module_ = PyImport_ImportModule("json");
  assert(nkit::json_module_);
  Py_INCREF(nkit::json_module_);

  nkit::json_dumps_ = PyObject_GetAttrString(nkit::json_module_, "dumps");
  assert(nkit::json_dumps_);
  Py_INCREF(nkit::json_dumps_);

  nkit::dt_module_ = PyImport_ImportModule("datetime");
  assert(nkit::dt_module_);
  Py_INCREF(nkit::dt_module_);

  nkit::dt_ = PyObject_GetAttrString(nkit::dt_module_, "datetime");
  assert(nkit::dt_);
  Py_INCREF(nkit::dt_);

  nkit::fromtimestamp_ = PyObject_GetAttrString(nkit::dt_, "fromtimestamp");
  assert(nkit::fromtimestamp_);
  Py_INCREF(nkit::fromtimestamp_);

  // class DatetimeJSONEncoder
  nkit::main_module_ = PyImport_AddModule("__main__");
  Py_INCREF(nkit::main_module_);
  PyObject * globals = PyModule_GetDict(nkit::main_module_);
  PyObject * locals = PyDict_New();
  PyDict_SetItemString(globals, "nkit_tmp_datetime", nkit::dt_module_);
  PyDict_SetItemString(globals, "nkit_tmp_traceback", nkit::traceback_module_);
  PyDict_SetItemString(globals, "nkit_tmp_json", nkit::json_module_);
  PyObject * run = PyRun_String(nkit::DATETIME_JSON_ENCODER_CLASS,
      Py_file_input, globals, locals);
  if ( NULL == run)
  {
    Py_DECREF(locals);
    CERR("Could not run code statements");
    return;
  }

  nkit::datetime_json_encoder_ =
      PyDict_GetItemString(locals, "DatetimeJSONEncoder");
  assert(nkit::datetime_json_encoder_);
  Py_INCREF(nkit::datetime_json_encoder_);
  PyModule_AddObject(module, "DatetimeJSONEncoder",
      nkit::datetime_json_encoder_);

  PyDict_DelItemString(globals, "nkit_tmp_datetime");
  PyDict_DelItemString(globals, "nkit_tmp_traceback");
  PyDict_DelItemString(globals, "nkit_tmp_json");
  Py_DECREF(locals);
  Py_DECREF(run);
}
