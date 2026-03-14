ZEND_API int compare_function(zval *result, zval *op1, zval *op2 TSRMLS_DC)
{
	/* Handle NULL vs string comparisons first */
	if (op1->type == IS_NULL && op2->type == IS_STRING) {
		result->type = IS_LONG;
		result->value.lval = zend_binary_strcmp("", 0, op2->value.str.val, op2->value.str.len);
		return SUCCESS;
	}
	if (op2->type == IS_NULL && op1->type == IS_STRING) {
		result->type = IS_LONG;
		result->value.lval = zend_binary_strcmp(op1->value.str.val, op1->value.str.len, "", 0);
		return SUCCESS;
	}
	
	/* Optimized string comparison path */
	if (op1->type == IS_STRING && op2->type == IS_STRING) {
		zendi_smart_strcmp(result, op1, op2);
		return SUCCESS;
	}
	
	/* Boolean/NULL comparison */
	if (op1->type == IS_BOOL || op2->type == IS_BOOL ||
		op1->type == IS_NULL || op2->type == IS_NULL) {
		zval op1_copy, op2_copy;
		zendi_convert_to_boolean(op1, op1_copy, result);
		zendi_convert_to_boolean(op2, op2_copy, result);
		result->type = IS_LONG;
		result->value.lval = ZEND_NORMALIZE_BOOL(op1->value.lval - op2->value.lval);
		return SUCCESS;
	}

	/* Numeric comparison */
	{
		zval op1_copy, op2_copy;
		zendi_convert_scalar_to_number(op1, op1_copy, result);
		zendi_convert_scalar_to_number(op2, op2_copy, result);

		if (op1->type == IS_LONG && op2->type == IS_LONG) {
			result->type = IS_LONG;
			result->value.lval = op1->value.lval > op2->value.lval ? 1 : 
							   (op1->value.lval < op2->value.lval ? -1 : 0);
			return SUCCESS;
		}
		if ((op1->type == IS_DOUBLE || op1->type == IS_LONG) &&
			(op2->type == IS_DOUBLE || op2->type == IS_LONG)) {
			double dval1 = (op1->type == IS_LONG) ? (double)op1->value.lval : op1->value.dval;
			double dval2 = (op2->type == IS_LONG) ? (double)op2->value.lval : op2->value.dval;
			result->value.dval = dval1 - dval2;
			result->value.lval = ZEND_NORMALIZE_BOOL(result->value.dval);
			result->type = IS_LONG;
			return SUCCESS;
		}
	}

	/* Complex type comparisons */
	if (op1->type == IS_ARRAY && op2->type == IS_ARRAY) {
		zend_compare_arrays(result, op1, op2 TSRMLS_CC);
		return SUCCESS;
	}
	if (op1->type == IS_OBJECT && op2->type == IS_OBJECT) {
		zend_compare_objects(result, op1, op2 TSRMLS_CC);
		return SUCCESS;
	}

	/* Default comparisons for complex types */
	if (op1->type == IS_ARRAY) {
		result->value.lval = 1;
	} else if (op2->type == IS_ARRAY) {
		result->value.lval = -1;
	} else if (op1->type == IS_OBJECT) {
		result->value.lval = 1;
	} else if (op2->type == IS_OBJECT) {
		result->value.lval = -1;
	} else {
		ZVAL_BOOL(result, 0);
		return FAILURE;
	}
	result->type = IS_LONG;
	return SUCCESS;
}