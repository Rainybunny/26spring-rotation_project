bool PyUpb_PyToUpb(PyObject* obj, const upb_FieldDef* f, upb_MessageValue* val,
                   upb_Arena* arena) {
  switch (upb_FieldDef_CType(f)) {
    case kUpb_CType_Enum:
      return PyUpb_PyToUpbEnum(obj, upb_FieldDef_EnumSubDef(f), val);
    case kUpb_CType_Int32:
      return PyUpb_GetInt32(obj, &val->int32_val);
    case kUpb_CType_Int64:
      return PyUpb_GetInt64(obj, &val->int64_val);
    case kUpb_CType_UInt32:
      return PyUpb_GetUint32(obj, &val->uint32_val);
    case kUpb_CType_UInt64:
      return PyUpb_GetUint64(obj, &val->uint64_val);
    case kUpb_CType_Float:
      if (PyUpb_IsNumpyNdarray(obj, f)) return false;
      val->float_val = PyFloat_AsDouble(obj);
      return !PyErr_Occurred();
    case kUpb_CType_Double:
      if (PyUpb_IsNumpyNdarray(obj, f)) return false;
      val->double_val = PyFloat_AsDouble(obj);
      return !PyErr_Occurred();
    case kUpb_CType_Bool:
      if (PyUpb_IsNumpyNdarray(obj, f)) return false;
      val->bool_val = PyLong_AsLong(obj);
      return !PyErr_Occurred();
    case kUpb_CType_Bytes: {
      if (PyBytes_Check(obj)) {
        char* ptr;
        Py_ssize_t size;
        if (PyBytes_AsStringAndSize(obj, &ptr, &size) >= 0) {
          *val = PyUpb_MaybeCopyString(ptr, size, arena);
          return true;
        }
      }
      return false;
    }
    case kUpb_CType_String: {
      if (PyUnicode_Check(obj)) {
        Py_ssize_t size;
        const char* ptr = PyUnicode_AsUTF8AndSize(obj, &size);
        if (ptr) {
          *val = PyUpb_MaybeCopyString(ptr, size, arena);
          return true;
        }
        return false;
      } else if (PyBytes_Check(obj)) {
        char* ptr;
        Py_ssize_t size;
        if (PyBytes_AsStringAndSize(obj, &ptr, &size) < 0) return false;
        if (!utf8_range_IsValid(ptr, size)) {
          // Invalid UTF-8. Try to convert to get proper error message
          obj = PyUnicode_FromEncodedObject(obj, "utf-8", NULL);
          assert(!obj);
          return false;
        }
        *val = PyUpb_MaybeCopyString(ptr, size, arena);
        return true;
      }
      return false;
    }
    case kUpb_CType_Message:
      PyErr_Format(PyExc_ValueError, "Message objects may not be assigned");
      return false;
    default:
      PyErr_Format(PyExc_SystemError,
                   "Getting a value from a field of unknown type %d",
                   upb_FieldDef_CType(f));
      return false;
  }
}