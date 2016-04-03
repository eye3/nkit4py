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

#include <py3c.h>
#include "datetime.h"
#include "nkit/types.h"
#include "nkit/tools.h"
#include "nkit/logger_brief.h"
#include "nkit/xml2var.h"
#include "nkit/var2xml.h"
#include <string>

#if ((PY_MAJOR_VERSION == 2) && (PY_MINOR_VERSION <= 5))
#define NKIT_PYTHON_OLDER_THEN_2_6
#endif

#if ((PY_MAJOR_VERSION == 2) && (PY_MINOR_VERSION == 6))
#define NKIT_PYTHON_26
#endif

#if NKIT_PYTHON_OLDER_THEN_2_6
#error "Python version older then 2.6 does not supported"
#endif

namespace nkit
{
  //----------------------------------------------------------------------------
  static PyObject * main_module_;
  static PyObject * dt_module_;
  static PyObject * dt_;
  static PyObject * collections_module_;
  static PyObject * ordered_dict_;
  static PyObject * ordered_dict_set_item_;
  static PyObject * fromtimestamp_;
  static PyObject * traceback_module_;
  static PyObject * traceback_dict_;
  static PyObject * traceback_format_exception_;
  static PyObject * traceback_format_exception_only_;
  static PyObject * json_module_;
  static PyObject * json_dumps_;
  static PyObject * string_module_;
  static PyObject * string_dict_;
  static PyObject * datetime_json_encoder_;

  //----------------------------------------------------------------------------
  bool py_to_string(PyObject * unicode, std::string * out,
      std::string * error)
  {
    if (PyStr_Check(unicode))
    {
      PyObject * tmp = PyStr_AsUTF8String(unicode);
      const char * str = PyBytes_AS_STRING(tmp);
      out->assign(str);
      Py_DECREF(tmp);
    }
    else if (PyBytes_Check(unicode))
    {
      out->assign(PyBytes_AsString(unicode));
    }
    else if (PyFloat_Check(unicode))
    {
      out->assign(string_cast(PyFloat_AsDouble(unicode)));
    }
    else if (PyNumber_Check(unicode))
    {
      out->assign(string_cast(
              static_cast<int64_t>(PyNumber_AsSsize_t(unicode, NULL))));
    }
    else
    {
      PyObject * tmp = PyObject_Str(unicode);
      if (!tmp)
      {
        *error = "Could not represent variable to string";
        return false;
      }

      bool ret = py_to_string(tmp, out,error);
      Py_DECREF(tmp);
      return ret;
    }

    return true;
  }

  //----------------------------------------------------------------------------
  PyObject * py_fromtimestamp(time_t  timestamp)
  {
    return PyObject_CallFunction(fromtimestamp_, (char*)"i", timestamp);
  }

  //----------------------------------------------------------------------------
  std::string py_strftime(const PyObject * data, const std::string & format)
  {
    std::string ret, error;
    PyObject * unicode =
            PyObject_CallMethod(const_cast<PyObject *>(data),
                    const_cast<char *>("strftime"),
                    const_cast<char *>("s"), format.c_str());
    py_to_string(unicode, &ret, &error);
    Py_DECREF(unicode);
    return ret;
  }

  //----------------------------------------------------------------------------
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
    bool ret = py_to_string(result, out, error);
    Py_DECREF(result);
    Py_DECREF(args);
    Py_DECREF(kw);

    return ret;
  }

  //----------------------------------------------------------------------------
  class PythonBuilderPolicy: Uncopyable
  {
  private:
    friend class VarBuilder<PythonBuilderPolicy>;

    typedef PyObject* type;

    static const type & GetUndefined()
    {
      static PyObject * WarningWorkaround = Py_None;
      Py_INCREF(Py_None);
      return WarningWorkaround;
    }

    PythonBuilderPolicy(const detail::Options & options)
      : object_(NULL)
      , options_(options)
    {}

    ~PythonBuilderPolicy()
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
      object_ = PyLong_FromLongLong(i);
      assert(object_);
    }

    void InitAsString( std::string const & value)
    {
      Py_CLEAR(object_);
      if (options_.unicode_)
        object_ = PyUnicode_FromStringAndSize(value.data(), value.size());
      else
        object_ = PyBytes_FromStringAndSize(value.data(), value.size());
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
      if (options_.ordered_dict_ && ordered_dict_)
        object_ = PyObject_CallObject(ordered_dict_, NULL);
      else
        object_ = PyDict_New();
      assert(object_);
    }

    void ListCheck()
    {
      assert(PyList_Check(object_));
    }

    void DictCheck()
    {
      assert(PyDict_Check(object_));
    }

    void AppendToList( type const & var )
    {
      int result = PyList_Append( object_, var );
      assert(-1 != result);
      NKIT_FORCE_USED(result)
    }

    void AppendToDictKeyList( std::string const & key, type const & var )
    {
      type value = PyDict_GetItemString(object_, key.c_str());
      if (value && PyList_Check(value))
        PyList_Append(value, var);
      else
      {
        type list = PyList_New(1);
        Py_INCREF(var);
        PyList_SET_ITEM(list, 0, var);
        SetDictKeyValue(key, list);
        Py_CLEAR(list);
      }
    }

    void SetDictKeyValue( std::string const & key, type const & var )
    {
      if (options_.ordered_dict_ && ordered_dict_)
      {
        PyObject * pkey;
        if (options_.unicode_)
          pkey = PyUnicode_FromStringAndSize(key.data(), key.size());
        else
          pkey = PyBytes_FromStringAndSize(key.data(), key.size());

        PyObject * result = PyObject_CallFunction(ordered_dict_set_item_,
            const_cast<char*>("OOO"), object_, pkey, var);

        assert(result);
        Py_CLEAR(result);
        Py_CLEAR(pkey);
      }
      else
      {
        int result = PyDict_SetItemString( object_, key.c_str(), var );
        assert(-1 != result);
        NKIT_FORCE_USED(result)
      }
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

  typedef VarBuilder<PythonBuilderPolicy> PythonVarBuilder;
  typedef StructXml2VarBuilder<PythonVarBuilder> MapXml2PythonBuilder;
  typedef AnyXml2VarBuilder<PythonVarBuilder> AnyXml2PythonBuilder;

  ////--------------------------------------------------------------------------
  struct PythonReaderPolicy
  {
    typedef PyObject * type;

    //--------------------------------------------------------------------------
    struct DictConstIterator
    {
      DictConstIterator()
        : data_(NULL)
        , iter_(NULL)
        , pos_(-1)
        , key_(NULL)
        , value_(NULL)
      {}

      DictConstIterator(PyObject * data)
        : data_(data)
        , iter_(PyObject_GetIter(data))
        , pos_(-1)
        , key_(NULL)
        , value_(NULL)
      {
        Fetch();
      }

      void Fetch()
      {
        ++pos_;

        Py_XDECREF(key_);
        key_ = NULL;

        if (!iter_)
        {
          pos_ = -1;
          key_ = NULL;
          value_ = NULL;
          return;
        }

        key_ = PyIter_Next(iter_);
        if (!key_)
        {
          pos_ = -1;
          key_ = NULL;
          value_ = NULL;
          return;
        }

        value_ = PyDict_GetItem(data_, key_);
      }

      ~DictConstIterator()
      {
        Py_XDECREF(key_);
        key_ = NULL;
        Py_XDECREF(iter_);
        iter_ = NULL;
      }

      bool operator != (const DictConstIterator & another)
      {
        return pos_ != another.pos_;
      }

      DictConstIterator & operator++()
      {
        Fetch();
        return *this;
      }

      std::string first() const
      {
        if (!key_)
          return S_EMPTY_;

        std::string ret, err;
        if (unlikely(!nkit::py_to_string(key_, &ret, &err)))
          return S_EMPTY_;
        return ret;
      }

      PyObject * second() const
      {
        if (!value_)
          return Py_None;
        return value_;
      }

      PyObject * data_;
      PyObject * iter_;
      Py_ssize_t pos_;
      PyObject * key_;
      PyObject * value_;
    };

    //--------------------------------------------------------------------------
    struct ListConstIterator
    {
      ListConstIterator()
        : list_(NULL)
        , size_(0)
        , pos_(-1)
      {}

      ListConstIterator(PyObject * list)
        : list_(PySequence_Fast(list, "Not a sequence"))
        , size_(list_ != NULL ? PySequence_Size(list_) : 0)
        , pos_(size_ > 0 ? 0: -1)
      {}

      ListConstIterator(const ListConstIterator & copy)
        : list_(copy.list_)
        , size_(copy.size_)
        , pos_(copy.pos_)
      {
        Py_XINCREF(list_);
      }

      ListConstIterator & operator = (const ListConstIterator & copy)
      {
        list_ = copy.list_;
        size_ = copy.size_;
        pos_ = copy.pos_;
        Py_XINCREF(list_);
        return *this;
      }

      ~ListConstIterator()
      {
        Py_XDECREF(list_);
      }

      bool operator != (const ListConstIterator & another)
      {
        return pos_ != another.pos_;
      }

      ListConstIterator & operator++()
      {
        if (++pos_ >= size_)
          pos_ = -1;
        return *this;
      }

      PyObject * value() const
      {
        if (unlikely(pos_ >= size_ || pos_ < 0))
          return Py_None;
        return PySequence_Fast_GET_ITEM(list_, pos_);
      }

      PyObject * list_;
      Py_ssize_t size_;
      Py_ssize_t pos_;
    };

    //--------------------------------------------------------------------------
    static DictConstIterator begin_d(const PyObject * data)
    {
      return DictConstIterator(const_cast<PyObject *>(data));
    }

    static DictConstIterator end_d(const PyObject * data)
    {
      return DictConstIterator();
    }

    static ListConstIterator begin_l(const PyObject * data)
    {
      return ListConstIterator(const_cast<PyObject *>(data));
    }

    static ListConstIterator end_l(const PyObject * data)
    {
      return ListConstIterator();
    }

    static std::string First(const DictConstIterator & it)
    {
      return it.first();
    }

    static PyObject * Second(const DictConstIterator & it)
    {
      return it.second();
    }

    static PyObject * Value(const ListConstIterator & it)
    {
      return it.value();
    }

    static bool IsList(const PyObject * data)
    {
      bool ret = PyList_Check(const_cast<PyObject *>(data)) ||
              PyTuple_Check(const_cast<PyObject *>(data)) ||
              PySet_Check(const_cast<PyObject *>(data)) ;
      return ret;
    }

    static bool IsDict(const PyObject * data)
    {
      bool ret = PyDict_Check(const_cast<PyObject *>(data));
      return ret;
    }

    static bool IsString(const PyObject * data)
    {
      return PyStr_Check(data) || PyBytes_Check(data);
    }

    static bool IsFloat(const PyObject * data)
    {
      bool ret = PyFloat_Check(const_cast<PyObject *>(data));
      return ret;
    }

    static bool IsDateTime(const PyObject * data)
    {
      bool ret = PyDateTime_Check(const_cast<PyObject *>(data));
      return ret;
    }

    static bool IsBool(const PyObject * data)
    {
      bool ret = PyBool_Check(const_cast<PyObject *>(data));
      return ret;
    }

    static std::string GetString(const PyObject * data)
    {
      std::string ret, err;
      py_to_string(const_cast<PyObject *>(data), &ret, &err);
      return ret;
    }

    static std::string GetStringAsDateTime(const PyObject * data,
            const std::string & format)
    {
      return py_strftime(data, format);
    }

    static std::string GetStringAsFloat(const PyObject * data,
            size_t precision)
    {
      return string_cast(PyFloat_AsDouble(const_cast<PyObject *>(data)),
              precision);
    }

    static const std::string & GetStringAsBool(const PyObject * data,
        const std::string & true_format, const std::string & false_format)
    {
      return data == Py_True ? true_format: false_format;
    }

    static PyObject * GetByKey(const PyObject * data, const std::string & key,
            bool * found)
    {
      PyObject * ret =
    		  PyDict_GetItemString(const_cast<PyObject *>(data), key.c_str());
      *found = ret != NULL;
      if (*found)
        return ret;
      else
        return Py_None;
    }
  };

  typedef Var2XmlConverter<PythonReaderPolicy> Python2XmlConverter;

} // namespace nkit

////----------------------------------------------------------------------------
/// exception
static PyObject * Nkit4PyError;

////----------------------------------------------------------------------------
bool parse_dict(PyObject * dict, std::string * out, std::string * error)
{
  if (PyDict_Check(dict))
  {
    return nkit::pyobj_to_json(dict, out, error);
  }
  else
  {
    return nkit::py_to_string(dict, out, error);
  }
}

////----------------------------------------------------------------------------
template<typename T>
struct SharedPtrHolder
{
  SharedPtrHolder(NKIT_SHARED_PTR(T) & ptr) : ptr_(ptr) {}
  NKIT_SHARED_PTR(T) ptr_;
};

////----------------------------------------------------------------------------
struct MapXml2PythonBuilderData
{
  PyObject_HEAD;
  SharedPtrHolder<nkit::MapXml2PythonBuilder> * holder_;
};

////----------------------------------------------------------------------------
struct AnyXml2PythonBuilderData
{
  PyObject_HEAD;
  SharedPtrHolder<nkit::AnyXml2PythonBuilder> * holder_;
};

////----------------------------------------------------------------------------
static PyObject* CreateMapXml2VarBuilder(
    PyTypeObject * type, PyObject * args, PyObject *)
{
  PyObject * dict1 = NULL;
  PyObject * dict2 = NULL;
  int result = PyArg_ParseTuple(args, "O|O", &dict1, &dict2);
  if(!result)
  {
    PyErr_SetString(Nkit4PyError,
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
    PyErr_SetString( Nkit4PyError,
        ("Options parameter must be JSON-string or dictionary: " +
        error).c_str());
    return NULL;
  }

  if (options.empty())
  {
    PyErr_SetString(
        Nkit4PyError,
        "Options parameter must be dict or JSON object" );
    return NULL;
  }

  std::string mappings;
  if (!parse_dict(mapping_dict, &mappings, &error))
  {
    PyErr_SetString( Nkit4PyError,
        ("Mappings parameter must be JSON-string or dictionary: " +
        error).c_str());
    return NULL;
  }

  if(mappings.empty())
  {
    PyErr_SetString(
        Nkit4PyError,
        "Mappings parameter must be dict or JSON object" );
    return NULL;
  }

  MapXml2PythonBuilderData * self =
      (MapXml2PythonBuilderData *)type->tp_alloc( type, 0 );
  if (!self)
  {
    PyErr_SetString(Nkit4PyError, "Low memory");
    return NULL;
  }

  nkit::MapXml2PythonBuilder::Ptr builder =
      nkit::MapXml2PythonBuilder::Create(options, mappings, &error);
  if(!builder)
  {
    PyErr_SetString( Nkit4PyError, error.c_str() );
    return NULL;
  }
  self->holder_ =
      new SharedPtrHolder< nkit::MapXml2PythonBuilder >(builder);

  return (PyObject *)self;
}

////----------------------------------------------------------------------------
static void DeleteMapXml2VarBuilder(PyObject * self)
{
  SharedPtrHolder< nkit::MapXml2PythonBuilder > * ptr =
        ((MapXml2PythonBuilderData *)self)->holder_;
  if (ptr)
    delete ptr;
  self->ob_type->tp_free(self);
}

////----------------------------------------------------------------------------
static PyObject * map_feed_method( PyObject * self, PyObject * args )
{
  const char* request = NULL;
  Py_ssize_t size = 0;
  int result = PyArg_ParseTuple( args, "s#", &request, &size );
  if(!result)
  {
    PyErr_SetString( Nkit4PyError, "Expected string arguments" );
    return NULL;
  }
  if( !request || !*request || !size )
  {
    PyErr_SetString(
        Nkit4PyError, "Parameter must not be empty string" );
    return NULL;
  }

  nkit::MapXml2PythonBuilder::Ptr builder =
      ((MapXml2PythonBuilderData *)self)->holder_->ptr_;

  std::string error("");
  if(!builder->Feed( request, size, false, &error ))
  {
    PyErr_SetString( Nkit4PyError, error.c_str() );
    return NULL;
  }

  Py_RETURN_NONE;
}

////----------------------------------------------------------------------------
static PyObject * map_get_method( PyObject * self, PyObject * args )
{
  const char* mapping_name = NULL;
  int result = PyArg_ParseTuple( args, "s", &mapping_name );
  if(!result)
  {
    PyErr_SetString( Nkit4PyError, "Expected string argument" );
    return NULL;
  }
  if( !mapping_name || !*mapping_name )
  {
    PyErr_SetString(
        Nkit4PyError, "Mapping name must not be empty" );
    return NULL;
  }

  nkit::MapXml2PythonBuilder::Ptr builder =
      ((MapXml2PythonBuilderData *)self)->holder_->ptr_;

  PyObject * item = builder->var(mapping_name);
  Py_INCREF(item);
  return item;
}

////------------------------------------------------------------------------------
static PyObject * map_end_method( PyObject * self, PyObject * /*args*/ )
{
  nkit::MapXml2PythonBuilder::Ptr builder =
          ((MapXml2PythonBuilderData *)self)->holder_->ptr_;

  std::string empty("");
  std::string error("");
  if(!builder->Feed( empty.c_str(), empty.size(), true, &error ))
  {
    PyErr_SetString( Nkit4PyError, error.c_str() );
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

////----------------------------------------------------------------------------
static PyMethodDef map_xml2var_methods[] =
{
  { "feed", map_feed_method, METH_VARARGS, "Usage: builder.feed()\n"
          "Invoke \"Feed\" function\n"
          "Returns None\n" },
  { "get", map_get_method, METH_VARARGS, "Usage: builder.get()\n"
          "Returns result by mapping name\n" },
  { "end", map_end_method, METH_VARARGS, "Usage: builder.end()\n"
          "Returns Dict: results for all mappings\n" },
  { NULL, NULL, 0, NULL } /* Sentinel */
};

////----------------------------------------------------------------------------
static PyTypeObject MapXml2PythonBuilderType =
{
  PyVarObject_HEAD_INIT(NULL, 0)
  "nkit4py.Xml2VarBuilder", /*tp_name*/
  sizeof(MapXml2PythonBuilderData), /*tp_basicsize*/
  0, /*tp_itemsize*/
  DeleteMapXml2VarBuilder, /*tp_dealloc*/
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
  map_xml2var_methods,//tp_methods,
  0,//tp_members,
  0,//tp_getset,
  0,//tp_base,
  0,//tp_dict,
  0,//tp_descr_get,
  0,//tp_descr_set,
  0,//tp_dictoffset,
  0,//tp_init,
  0,//tp_alloc,
  CreateMapXml2VarBuilder,//tp_new,
};

////----------------------------------------------------------------------------
static PyObject* CreateAnyXml2VarBuilder(
    PyTypeObject * type, PyObject * args, PyObject *)
{
  PyObject * options_dict = NULL;
  int result = PyArg_ParseTuple(args, "|O", &options_dict);
  if(!result)
  {
    PyErr_SetString(Nkit4PyError,
        "Expected one or two arguments:"
        " 1) mappings or 2) options and mappings");
    return NULL;
  }

  std::string options, error;
  if (!options_dict)
    options = "{}";
  else if (!parse_dict(options_dict, &options, &error))
  {
    PyErr_SetString( Nkit4PyError,
        ("Options parameter must be JSON-string or dictionary: " +
        error).c_str());
    return NULL;
  }

  if (options.empty())
  {
    PyErr_SetString(
        Nkit4PyError,
        "Options parameter must be dict or JSON object" );
    return NULL;
  }

  AnyXml2PythonBuilderData * self =
      (AnyXml2PythonBuilderData *)type->tp_alloc( type, 0 );
  if (!self)
  {
    PyErr_SetString(Nkit4PyError, "Low memory");
    return NULL;
  }

  nkit::AnyXml2PythonBuilder::Ptr builder =
      nkit::AnyXml2PythonBuilder::Create(options, &error);
  if(!builder)
  {
    PyErr_SetString( Nkit4PyError, error.c_str() );
    return NULL;
  }

  self->holder_ =
      new SharedPtrHolder< nkit::AnyXml2PythonBuilder >(builder);

  return (PyObject *)self;
}

////----------------------------------------------------------------------------
static void DeleteAnyXml2VarBuilder(PyObject * self)
{
  SharedPtrHolder< nkit::AnyXml2PythonBuilder > * ptr =
        ((AnyXml2PythonBuilderData *)self)->holder_;
  if (ptr)
    delete ptr;
  self->ob_type->tp_free(self);
}

////----------------------------------------------------------------------------
static PyObject * any_feed_method( PyObject * self, PyObject * args )
{
  const char* request = NULL;
  Py_ssize_t size = 0;
  int result = PyArg_ParseTuple( args, "s#", &request, &size );
  if(!result)
  {
    PyErr_SetString( Nkit4PyError, "Expected string arguments" );
    return NULL;
  }
  if( !request || !*request || !size )
  {
    PyErr_SetString(
        Nkit4PyError, "Parameter must not be empty string" );
    return NULL;
  }

  nkit::AnyXml2PythonBuilder::Ptr builder =
      ((AnyXml2PythonBuilderData *)self)->holder_->ptr_;

  std::string error("");
  if(!builder->Feed( request, size, false, &error ))
  {
    PyErr_SetString( Nkit4PyError, error.c_str() );
    return NULL;
  }

  Py_RETURN_NONE;
}

////----------------------------------------------------------------------------
static PyObject * any_get_method( PyObject * self, PyObject * args )
{
  nkit::AnyXml2PythonBuilder::Ptr builder =
      ((AnyXml2PythonBuilderData *)self)->holder_->ptr_;

  PyObject * item = builder->var();
  Py_INCREF(item);
  return item;
}

////------------------------------------------------------------------------------
static PyObject * any_end_method( PyObject * self, PyObject * /*args*/ )
{
  nkit::AnyXml2PythonBuilder::Ptr builder =
          ((AnyXml2PythonBuilderData *)self)->holder_->ptr_;

  std::string empty("");
  std::string error("");
  if(!builder->Feed( empty.c_str(), empty.size(), true, &error ))
  {
    PyErr_SetString( Nkit4PyError, error.c_str() );
    return NULL;
  }

  PyObject * item = builder->var();
  Py_INCREF(item);
  return item;
}

////----------------------------------------------------------------------------
static PyObject * any_root_name_method( PyObject * self, PyObject * args )
{
  nkit::AnyXml2PythonBuilder::Ptr builder =
      ((AnyXml2PythonBuilderData *)self)->holder_->ptr_;

  const std::string & root_name = builder->root_name();
  return PyStr_FromString(root_name.c_str());
}

////----------------------------------------------------------------------------
static PyMethodDef any_xml2var_methods[] =
{
  { "feed", any_feed_method, METH_VARARGS, "Usage: builder.feed()\n"
          "Invoke \"Feed\" function\n"
          "Returns None\n" },
  { "get", any_get_method, METH_VARARGS, "Usage: builder.get()\n"
          "Returns result\n" },
  { "end", any_end_method, METH_VARARGS, "Usage: builder.end()\n"
          "Returns result\n" },
  { "root_name", any_root_name_method, METH_VARARGS,
      "Usage: builder.root_name()\n"
      "Returns root element name\n" },
  { NULL, NULL, 0, NULL } /* Sentinel */
};

////----------------------------------------------------------------------------
static PyTypeObject AnyXml2PythonBuilderType =
{
  PyVarObject_HEAD_INIT(NULL, 0)
  "nkit4py.AnyXml2VarBuilder", /*tp_name*/
  sizeof(AnyXml2PythonBuilderData), /*tp_basicsize*/
  0, /*tp_itemsize*/
  DeleteAnyXml2VarBuilder, /*tp_dealloc*/
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
  any_xml2var_methods,//tp_methods,
  0,//tp_members,
  0,//tp_getset,
  0,//tp_base,
  0,//tp_dict,
  0,//tp_descr_get,
  0,//tp_descr_set,
  0,//tp_dictoffset,
  0,//tp_init,
  0,//tp_alloc,
  CreateAnyXml2VarBuilder,//tp_new,
};

////----------------------------------------------------------------------------
static PyObject * var2xml_method( PyObject * self, PyObject * args )
{
  PyObject * data = NULL;
  PyObject * options_dict = NULL;
  int result = PyArg_ParseTuple( args, "O|O", &data, &options_dict);
  if(!result)
  {
    PyErr_SetString( Nkit4PyError,
            "Expected any object and optional 'options' Dict" );
    return NULL;
  }

  std::string options, error;
  nkit::Dynamic op = nkit::Dynamic::Dict();
  if (options_dict && !parse_dict(options_dict, &options, &error))
  {
    std::string tmp("Options parameter must be JSON-string or dictionary: " +
            error);
    PyErr_SetString( Nkit4PyError, tmp.c_str());
    return NULL;
  }
  else if (!options.empty())
  {
    op = nkit::DynamicFromJson(options, &error);
    if (!op && !error.empty())
    {
      std::string tmp("Options parameter must be JSON-string or dictionary: " +
              error);
      PyErr_SetString( Nkit4PyError, tmp.c_str());
      return NULL;
    }
  }

  std::string out;
  if(!nkit::Python2XmlConverter::Process(op, data, &out, &error))
  {
    PyErr_SetString( Nkit4PyError, error.c_str() );
    return NULL;
  }

  const nkit::Dynamic * unicode = NULL;
  if (op.Get("unicode", &unicode) && unicode->GetBoolean())
    return PyUnicode_FromStringAndSize(out.data(), out.size());
  else
    return PyBytes_FromStringAndSize(out.data(), out.size());
}

////----------------------------------------------------------------------------
static PyMethodDef ModuleMethods[] =
{
  { "var2xml", var2xml_method, METH_VARARGS,
          "Usage: nkit4py.var2xml(data, options)\n"
          "Converts python structure to xml string\n"
          "Returns None\n" },
  { NULL, NULL, 0, NULL } /* Sentinel */
};

static struct PyModuleDef moduledef = {
    PyModuleDef_HEAD_INIT,  /* m_base */
    "nkit4py",                 /* m_name */
    NULL,                   /* m_doc */
    -1,                     /* m_size */
    ModuleMethods            /* m_methods */
};


////----------------------------------------------------------------------------
namespace nkit
{
  std::string PythonBuilderPolicy::ToString() const
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


#ifndef PyMODINIT_FUNC  /* declarations for DLL import/export */
#define PyMODINIT_FUNC void
#endif

////----------------------------------------------------------------------------
MODULE_INIT_FUNC(nkit4py)
{
  if( -1 == PyType_Ready(&MapXml2PythonBuilderType) )
    return NULL;

  if( -1 == PyType_Ready(&AnyXml2PythonBuilderType) )
    return NULL;

  PyObject * module = PyModule_Create(&moduledef);
  if( NULL == module )
    return NULL;

  Nkit4PyError = PyErr_NewException( (char *)"nkit4py.Error", NULL, NULL );
  Py_INCREF(Nkit4PyError);
  PyModule_AddObject( module, "Error", Nkit4PyError );

  Py_INCREF(&MapXml2PythonBuilderType);
  PyModule_AddObject( module,
          "Xml2VarBuilder", (PyObject *)&MapXml2PythonBuilderType );

  Py_INCREF(&AnyXml2PythonBuilderType);
  PyModule_AddObject( module,
          "AnyXml2VarBuilder", (PyObject *)&AnyXml2PythonBuilderType );

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

  nkit::json_module_ = PyImport_ImportModule("json");
  assert(nkit::json_module_);
  Py_INCREF(nkit::json_module_);

  nkit::json_dumps_ = PyObject_GetAttrString(nkit::json_module_, "dumps");
  assert(nkit::json_dumps_);
  Py_INCREF(nkit::json_dumps_);

  nkit::dt_module_ = PyImport_ImportModule("datetime");
  assert(nkit::dt_module_);
  Py_INCREF(nkit::dt_module_);

  nkit::collections_module_ = PyImport_ImportModule("collections");
  assert(nkit::collections_module_);
  Py_INCREF(nkit::collections_module_);

  nkit::ordered_dict_ =
      PyObject_GetAttrString(nkit::collections_module_, "OrderedDict");
  if(nkit::ordered_dict_)
  {
    Py_INCREF(nkit::ordered_dict_);
    nkit::ordered_dict_set_item_ =
          PyObject_GetAttrString(nkit::ordered_dict_, "__setitem__");
    assert(nkit::ordered_dict_set_item_);
    Py_INCREF(nkit::ordered_dict_set_item_);
  }
  else
  {
    Py_DECREF(nkit::collections_module_);
    nkit::collections_module_ = PyImport_ImportModule("ordereddict");
    if (nkit::collections_module_)
    {
      Py_INCREF(nkit::collections_module_);
      nkit::ordered_dict_ =
          PyObject_GetAttrString(nkit::collections_module_, "OrderedDict");
      if(nkit::ordered_dict_)
      {
        Py_INCREF(nkit::ordered_dict_);
        nkit::ordered_dict_set_item_ =
              PyObject_GetAttrString(nkit::ordered_dict_, "__setitem__");
        assert(nkit::ordered_dict_set_item_);
        Py_INCREF(nkit::ordered_dict_set_item_);
      }
    }
  }

  PyDateTime_IMPORT;

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
    return NULL;
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

  return module;
}
