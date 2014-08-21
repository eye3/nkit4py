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

#include "python_var_builder.h"
#include "nkit/vx.h"
#include "nkit/types.h"
#include "Python.h"
#include <string>


typedef nkit::PythonVarBuilder VarBuilder;

/// тип исключений класса PythonXml2VarBuilder
static PyObject * PythonXml2VarBuilderError;

template<typename T>
struct SharedPtrHolder
{
  SharedPtrHolder(NKIT_SHARED_PTR(T) & ptr) : ptr_(ptr) {}
  NKIT_SHARED_PTR(T) ptr_;
};

struct PythonXml2VarBuilder
{
  PyObject_HEAD;
  SharedPtrHolder< nkit::Xml2VarBuilder< VarBuilder > > * gen_;
};

static PyObject*
CreatePythonXml2VarBuilder( PyTypeObject * type, PyObject * args, PyObject *)
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
    nkit::Xml2VarBuilder< VarBuilder >::Ptr builder_ptr =
        nkit::Xml2VarBuilder< VarBuilder >::Create( target_spec, &error );
    self->gen_ =
        new SharedPtrHolder< nkit::Xml2VarBuilder< VarBuilder > >(builder_ptr);
    if(!self->gen_)
    {
      PyErr_SetString( PythonXml2VarBuilderError, error.c_str() );
      return NULL;
    }
  }

  return (PyObject *)self;
}

static void DeletePythonXml2VarBuilder(PyObject * self)
{
  delete ((PythonXml2VarBuilder *)self)->gen_;
  self->ob_type->tp_free(self);
}

static PyObject *
feed_method( PyObject * self, PyObject * args )
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

  nkit::Xml2VarBuilder< VarBuilder >::Ptr gen =
      ((PythonXml2VarBuilder *)self)->gen_->ptr_;

  std::string error("");
  if(!gen->Feed( request, size, false, &error ))
  {
    PyErr_SetString( PythonXml2VarBuilderError, error.c_str() );
    return NULL;
  }

  Py_RETURN_NONE;
}

static PyObject *
end_method( PyObject * self, PyObject * /*args*/ )
{
  nkit::Xml2VarBuilder< VarBuilder >::Ptr gen =
        ((PythonXml2VarBuilder *)self)->gen_->ptr_;

  std::string empty("");
  std::string error("");
  if(!gen->Feed( empty.c_str(), empty.size(), true, &error ))
  {
    PyErr_SetString( PythonXml2VarBuilderError, error.c_str() );
    return NULL;
  }

  return gen->var();
}

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

static PyMethodDef ModuleMethods[] =
{
  { NULL, NULL, 0, NULL } /* Sentinel */
};

#ifndef PyMODINIT_FUNC  /* declarations for DLL import/export */
#define PyMODINIT_FUNC void
#endif

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
}
