#ifndef Py_INTERNAL_CALL_H
#define Py_INTERNAL_CALL_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_pystate.h"       // _PyThreadState_GET()

// Export for shared stdlib extensions like the math extension,
// function used via inlined _PyObject_VectorcallTstate() function.
PyAPI_FUNC(PyObject*) _Py_CheckFunctionResult(
    PyThreadState *tstate,
    PyObject *callable,
    PyObject *result,
    const char *where);

/* Convert keyword arguments from the FASTCALL (stack: C array, kwnames: tuple)
   format to a Python dictionary ("kwargs" dict).

   The type of kwnames keys is not checked. The final function getting
   arguments is responsible to check if all keys are strings, for example using
   PyArg_ParseTupleAndKeywords() or PyArg_ValidateKeywordArguments().

   Duplicate keys are merged using the last value. If duplicate keys must raise
   an exception, the caller is responsible to implement an explicit keys on
   kwnames. */
extern PyObject* _PyStack_AsDict(PyObject *const *values, PyObject *kwnames);

extern PyObject* _PyObject_Call_Prepend(
    PyThreadState *tstate,
    PyObject *callable,
    PyObject *obj,
    PyObject *args,
    PyObject *kwargs);

extern PyObject* _PyObject_FastCallDictTstate(
    PyThreadState *tstate,
    PyObject *callable,
    PyObject *const *args,
    size_t nargsf,
    PyObject *kwargs);

extern PyObject* _PyObject_Call(
    PyThreadState *tstate,
    PyObject *callable,
    PyObject *args,
    PyObject *kwargs);

extern PyObject * _PyObject_CallMethodFormat(
    PyThreadState *tstate,
    PyObject *callable,
    const char *format,
    ...);

// Export for shared stdlib extensions like the array extension
PyAPI_FUNC(PyObject*) _PyObject_CallMethod(
    PyObject *obj,
    PyObject *name,
    const char *format, ...);

/* Like PyObject_CallMethod(), but expect a _Py_Identifier*
   as the method name. */
extern PyObject* _PyObject_CallMethodId(
    PyObject *obj,
    _Py_Identifier *name,
    const char *format, ...);

extern PyObject* _PyObject_CallMethodIdObjArgs(
    PyObject *obj,
    _Py_Identifier *name,
    ...);

static inline PyObject *
_PyObject_VectorcallMethodId(
    _Py_Identifier *name, PyObject *const *args,
    size_t nargsf, PyObject *kwnames)
{
    PyObject *oname = _PyUnicode_FromId(name); /* borrowed */
    if (!oname) {
        return _Py_NULL;
    }
    return PyObject_VectorcallMethod(oname, args, nargsf, kwnames);
}

static inline PyObject *
_PyObject_CallMethodIdNoArgs(PyObject *self, _Py_Identifier *name)
{
    size_t nargsf = 1 | PY_VECTORCALL_ARGUMENTS_OFFSET;
    return _PyObject_VectorcallMethodId(name, &self, nargsf, _Py_NULL);
}

static inline PyObject *
_PyObject_CallMethodIdOneArg(PyObject *self, _Py_Identifier *name, PyObject *arg)
{
    PyObject *args[2] = {self, arg};
    size_t nargsf = 2 | PY_VECTORCALL_ARGUMENTS_OFFSET;
    assert(arg != NULL);
    return _PyObject_VectorcallMethodId(name, args, nargsf, _Py_NULL);
}


/* === Vectorcall protocol (PEP 590) ============================= */

// Call callable using tp_call. Arguments are like PyObject_Vectorcall()
// or PyObject_FastCallDict() (both forms are supported),
// except that nargs is plainly the number of arguments without flags.
//
// Export for shared stdlib extensions like the math extension,
// function used via inlined _PyObject_VectorcallTstate() function.
PyAPI_FUNC(PyObject*) _PyObject_MakeTpCall(
    PyThreadState *tstate,
    PyObject *callable,
    PyObject *const *args, Py_ssize_t nargs,
    PyObject *keywords);

// Static inline variant of public PyVectorcall_Function().
static inline vectorcallfunc
_PyVectorcall_FunctionInline(PyObject *callable)
{
    assert(callable != NULL);

    PyTypeObject *tp = Py_TYPE(callable);
    if (!PyType_HasFeature(tp, Py_TPFLAGS_HAVE_VECTORCALL)) {
        return NULL;
    }
    assert(PyCallable_Check(callable));

    Py_ssize_t offset = tp->tp_vectorcall_offset;
    assert(offset > 0);

    vectorcallfunc ptr;
    memcpy(&ptr, (char *) callable + offset, sizeof(ptr));
    return ptr;
}


/* Call the callable object 'callable' with the "vectorcall" calling
   convention.

   args is a C array for positional arguments.

   nargsf is the number of positional arguments plus optionally the flag
   PY_VECTORCALL_ARGUMENTS_OFFSET which means that the caller is allowed to
   modify args[-1].

   kwnames is a tuple of keyword names. The values of the keyword arguments
   are stored in "args" after the positional arguments (note that the number
   of keyword arguments does not change nargsf). kwnames can also be NULL if
   there are no keyword arguments.

   keywords must only contain strings and all keys must be unique.

   Return the result on success. Raise an exception and return NULL on
   error. */
static inline PyObject *
_PyObject_VectorcallTstate(PyThreadState *tstate, PyObject *callable,
                           PyObject *const *args, size_t nargsf,
                           PyObject *kwnames)
{
    vectorcallfunc func;
    PyObject *res;

    assert(kwnames == NULL || PyTuple_Check(kwnames));
    assert(args != NULL || PyVectorcall_NARGS(nargsf) == 0);

    func = _PyVectorcall_FunctionInline(callable);
    if (func == NULL) {
        Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
        return _PyObject_MakeTpCall(tstate, callable, args, nargs, kwnames);
    }
    res = func(callable, args, nargsf, kwnames);
    return _Py_CheckFunctionResult(tstate, callable, res, NULL);
}


static inline PyObject *
_PyObject_CallNoArgsTstate(PyThreadState *tstate, PyObject *func) {
    return _PyObject_VectorcallTstate(tstate, func, NULL, 0, NULL);
}


// Private static inline function variant of public PyObject_CallNoArgs()
static inline PyObject *
_PyObject_CallNoArgs(PyObject *func) {
    EVAL_CALL_STAT_INC_IF_FUNCTION(EVAL_CALL_API, func);
    PyThreadState *tstate = _PyThreadState_GET();
    return _PyObject_VectorcallTstate(tstate, func, NULL, 0, NULL);
}


static inline PyObject *
_PyObject_FastCallTstate(PyThreadState *tstate, PyObject *func,
                         PyObject *const *args, Py_ssize_t nargs)
{
    EVAL_CALL_STAT_INC_IF_FUNCTION(EVAL_CALL_API, func);
    return _PyObject_VectorcallTstate(tstate, func, args, (size_t)nargs, NULL);
}

extern PyObject *const *
_PyStack_UnpackDict(PyThreadState *tstate,
    PyObject *const *args, Py_ssize_t nargs,
    PyObject *kwargs, PyObject **p_kwnames);

extern void _PyStack_UnpackDict_Free(
    PyObject *const *stack,
    Py_ssize_t nargs,
    PyObject *kwnames);

extern void _PyStack_UnpackDict_FreeNoDecRef(
    PyObject *const *stack,
    PyObject *kwnames);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_CALL_H */
