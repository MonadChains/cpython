#ifndef Py_CPYTHON_ABSTRACTOBJECT_H
#  error "this header file must not be included directly"
#endif

/* === Object Protocol ================================================== */

/* Suggested size (number of positional arguments) for arrays of PyObject*
   allocated on a C stack to avoid allocating memory on the heap memory. Such
   array is used to pass positional arguments to call functions of the
   PyObject_Vectorcall() family.

   The size is chosen to not abuse the C stack and so limit the risk of stack
   overflow. The size is also chosen to allow using the small stack for most
   function calls of the Python standard library. On 64-bit CPU, it allocates
   40 bytes on the stack. */
#define _PY_FASTCALL_SMALL_STACK 5

/* === Vectorcall protocol (PEP 590) ============================= */

// PyVectorcall_NARGS() is exported as a function for the stable ABI.
// Here (when we are not using the stable ABI), the name is overridden to
// call a static inline function for best performance.
static inline Py_ssize_t
_PyVectorcall_NARGS(size_t n)
{
    return n & ~PY_VECTORCALL_ARGUMENTS_OFFSET;
}
#define PyVectorcall_NARGS(n) _PyVectorcall_NARGS(n)

PyAPI_FUNC(vectorcallfunc) PyVectorcall_Function(PyObject *callable);

/* Same as PyObject_Vectorcall except that keyword arguments are passed as
   dict, which may be NULL if there are no keyword arguments. */
PyAPI_FUNC(PyObject *) PyObject_VectorcallDict(
    PyObject *callable,
    PyObject *const *args,
    size_t nargsf,
    PyObject *kwargs);

// Same as PyObject_Vectorcall(), except without keyword arguments
PyAPI_FUNC(PyObject *) _PyObject_FastCall(
    PyObject *func,
    PyObject *const *args,
    Py_ssize_t nargs);

PyAPI_FUNC(PyObject *) PyObject_CallOneArg(PyObject *func, PyObject *arg);

static inline PyObject *
PyObject_CallMethodNoArgs(PyObject *self, PyObject *name)
{
    size_t nargsf = 1 | PY_VECTORCALL_ARGUMENTS_OFFSET;
    return PyObject_VectorcallMethod(name, &self, nargsf, _Py_NULL);
}

static inline PyObject *
PyObject_CallMethodOneArg(PyObject *self, PyObject *name, PyObject *arg)
{
    PyObject *args[2] = {self, arg};
    size_t nargsf = 2 | PY_VECTORCALL_ARGUMENTS_OFFSET;
    assert(arg != NULL);
    return PyObject_VectorcallMethod(name, args, nargsf, _Py_NULL);
}

/* Guess the size of object 'o' using len(o) or o.__length_hint__().
   If neither of those return a non-negative value, then return the default
   value.  If one of the calls fails, this function returns -1. */
PyAPI_FUNC(Py_ssize_t) PyObject_LengthHint(PyObject *o, Py_ssize_t);

/* === Sequence protocol ================================================ */

/* Assume tp_as_sequence and sq_item exist and that 'i' does not
   need to be corrected for a negative index. */
#define PySequence_ITEM(o, i)\
    ( Py_TYPE(o)->tp_as_sequence->sq_item((o), (i)) )

/* === Mapping protocol ================================================= */

// Convert Python int to Py_ssize_t. Do nothing if the argument is None.
// Cannot be moved to the internal C API: used by Argument Clinic.
PyAPI_FUNC(int) _Py_convert_optional_to_ssize_t(PyObject *, void *);

// Same as PyNumber_Index but can return an instance of a subclass of int.
// Cannot be moved to the internal C API: used by Argument Clinic.
PyAPI_FUNC(PyObject *) _PyNumber_Index(PyObject *o);
