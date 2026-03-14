static void php_snmp_internal(INTERNAL_FUNCTION_PARAMETERS, int st,
                            struct snmp_session *session,
                            struct objid_query *objid_query)
{
    struct snmp_session *ss;
    struct snmp_pdu *pdu=NULL, *response;
    struct variable_list *vars;
    oid root[MAX_NAME_LEN];
    size_t rootlen = 0;
    int status, count, found;
    /* Make buffers static to avoid stack reallocation */
    static char buf[2048], buf2[2048];
    bool keepwalking = true;
    char *err;
    zval snmpval;
    int snmp_errno;
    size_t buf_size = sizeof(buf);
    size_t buf2_size = sizeof(buf2);

    RETVAL_FALSE;
    php_snmp_error(getThis(), PHP_SNMP_ERRNO_NOERROR, "");

    if (st & SNMP_CMD_WALK) {
        memmove(root, objid_query->vars[0].name, objid_query->vars[0].name_length * sizeof(oid));
        rootlen = objid_query->vars[0].name_length;
        objid_query->offset = objid_query->count;
    }

    if ((ss = snmp_open(session)) == NULL) {
        snmp_error(session, NULL, NULL, &err);
        php_error_docref(NULL, E_WARNING, "Could not open snmp connection: %s", err);
        free(err);
        RETURN_FALSE;
    }

    if ((st & SNMP_CMD_SET) && objid_query->count > objid_query->step) {
        php_snmp_error(getThis(), PHP_SNMP_ERRNO_MULTIPLE_SET_QUERIES, 
                      "Cannot fit all OIDs for SET query into one packet, using multiple queries");
    }

    while (keepwalking) {
        keepwalking = false;
        if (st & SNMP_CMD_WALK) {
            pdu = snmp_pdu_create(session->version == SNMP_VERSION_1 ? 
                                 SNMP_MSG_GETNEXT : SNMP_MSG_GETBULK);
            if (session->version != SNMP_VERSION_1) {
                pdu->non_repeaters = objid_query->non_repeaters;
                pdu->max_repetitions = objid_query->max_repetitions;
            }
            snmp_add_null_var(pdu, objid_query->vars[0].name, objid_query->vars[0].name_length);
        } else {
            int msg_type;
            if (st & SNMP_CMD_GET) {
                msg_type = SNMP_MSG_GET;
            } else if (st & SNMP_CMD_GETNEXT) {
                msg_type = SNMP_MSG_GETNEXT;
            } else if (st & SNMP_CMD_SET) {
                msg_type = SNMP_MSG_SET;
            } else {
                snmp_close(ss);
                php_error_docref(NULL, E_ERROR, "Unknown SNMP command (internals)");
                RETURN_FALSE;
            }
            pdu = snmp_pdu_create(msg_type);
            
            for (count = 0; objid_query->offset < objid_query->count && count < objid_query->step; objid_query->offset++, count++) {
                if (st & SNMP_CMD_SET) {
                    if ((snmp_errno = snmp_add_var(pdu, objid_query->vars[objid_query->offset].name, 
                                                  objid_query->vars[objid_query->offset].name_length, 
                                                  objid_query->vars[objid_query->offset].type, 
                                                  objid_query->vars[objid_query->offset].value))) {
                        snprint_objid(buf, buf_size, objid_query->vars[objid_query->offset].name, 
                                     objid_query->vars[objid_query->offset].name_length);
                        php_snmp_error(getThis(), PHP_SNMP_ERRNO_OID_PARSING_ERROR, 
                                     "Could not add variable: OID='%s' type='%c' value='%s': %s", 
                                     buf, objid_query->vars[objid_query->offset].type, 
                                     objid_query->vars[objid_query->offset].value, 
                                     snmp_api_errstring(snmp_errno));
                        snmp_free_pdu(pdu);
                        snmp_close(ss);
                        RETURN_FALSE;
                    }
                } else {
                    snmp_add_null_var(pdu, objid_query->vars[objid_query->offset].name, 
                                    objid_query->vars[objid_query->offset].name_length);
                }
            }
            if (pdu->variables == NULL) {
                snmp_free_pdu(pdu);
                snmp_close(ss);
                RETURN_FALSE;
            }
        }

retry:
        status = snmp_synch_response(ss, pdu, &response);
        if (status == STAT_SUCCESS) {
            if (response->errstat == SNMP_ERR_NOERROR) {
                if (st & SNMP_CMD_SET) {
                    if (objid_query->offset < objid_query->count) {
                        keepwalking = true;
                        snmp_free_pdu(response);
                        continue;
                    }
                    snmp_free_pdu(response);
                    snmp_close(ss);
                    RETURN_TRUE;
                }
                
                for (vars = response->variables; vars; vars = vars->next_variable) {
                    if (vars->type == SNMP_ENDOFMIBVIEW ||
                        vars->type == SNMP_NOSUCHOBJECT ||
                        vars->type == SNMP_NOSUCHINSTANCE) {
                        if ((st & SNMP_CMD_WALK) && Z_TYPE_P(return_value) == IS_ARRAY) {
                            break;
                        }
                        snprint_objid(buf, buf_size, vars->name, vars->name_length);
                        snprint_value(buf2, buf2_size, vars->name, vars->name_length, vars);
                        php_snmp_error(getThis(), PHP_SNMP_ERRNO_ERROR_IN_REPLY, 
                                     "Error in packet at '%s': %s", buf, buf2);
                        continue;
                    }

                    if ((st & SNMP_CMD_WALK) &&
                        (vars->name_length < rootlen || memcmp(root, vars->name, rootlen * sizeof(oid)))) {
                        if (Z_TYPE_P(return_value) == IS_ARRAY) {
                            keepwalking = false;
                        } else {
                            st |= SNMP_CMD_GET;
                            st ^= SNMP_CMD_WALK;
                            objid_query->offset = 0;
                            keepwalking = true;
                        }
                        break;
                    }

                    ZVAL_NULL(&snmpval);
                    php_snmp_getvalue(vars, &snmpval, objid_query->valueretrieval);

                    if (objid_query->array_output) {
                        if (Z_TYPE_P(return_value) == IS_TRUE || Z_TYPE_P(return_value) == IS_FALSE) {
                            array_init(return_value);
                        }
                        
                        snprint_objid(buf2, buf2_size, vars->name, vars->name_length);
                        
                        if (st & SNMP_NUMERIC_KEYS) {
                            add_next_index_zval(return_value, &snmpval);
                        } else if (st & SNMP_ORIGINAL_NAMES_AS_KEYS && st & SNMP_CMD_GET) {
                            found = 0;
                            for (count = 0; count < objid_query->count; count++) {
                                if (objid_query->vars[count].name_length == vars->name_length && 
                                    snmp_oid_compare(objid_query->vars[count].name, 
                                                    objid_query->vars[count].name_length, 
                                                    vars->name, vars->name_length) == 0) {
                                    found = 1;
                                    objid_query->vars[count].name_length = 0;
                                    break;
                                }
                            }
                            if (found) {
                                add_assoc_zval(return_value, objid_query->vars[count].oid, &snmpval);
                            } else {
                                php_error_docref(NULL, E_WARNING, 
                                               "Could not find original OID name for '%s'", buf2);
                            }
                        } else if (st & SNMP_USE_SUFFIX_AS_KEYS && st & SNMP_CMD_WALK) {
                            if (rootlen <= vars->name_length && 
                                snmp_oid_compare(root, rootlen, vars->name, rootlen) == 0) {
                                char *ptr = buf2;
                                for (count = rootlen; count < vars->name_length; count++) {
                                    ptr += sprintf(ptr, "%lu.", vars->name[count]);
                                }
                                if (ptr > buf2) {
                                    ptr[-1] = '\0';
                                }
                            }
                            add_assoc_zval(return_value, buf2, &snmpval);
                        } else {
                            add_assoc_zval(return_value, buf2, &snmpval);
                        }
                    } else {
                        ZVAL_COPY_VALUE(return_value, &snmpval);
                        break;
                    }

                    if (st & SNMP_CMD_WALK) {
                        if (objid_query->oid_increasing_check && 
                            snmp_oid_compare(objid_query->vars[0].name, 
                                           objid_query->vars[0].name_length, 
                                           vars->name, vars->name_length) >= 0) {
                            php_snmp_error(getThis(), PHP_SNMP_ERRNO_OID_NOT_INCREASING, 
                                         "Error: OID not increasing: %s", buf2);
                            keepwalking = false;
                        } else {
                            memmove(objid_query->vars[0].name, vars->name, 
                                   vars->name_length * sizeof(oid));
                            objid_query->vars[0].name_length = vars->name_length;
                            keepwalking = true;
                        }
                    }
                }
                if (objid_query->offset < objid_query->count) {
                    keepwalking = true;
                }
            } else {
                /* Error handling remains the same */
                /* ... */
            }
        } else if (status == STAT_TIMEOUT) {
            /* Timeout handling remains the same */
            /* ... */
        } else {
            /* Error handling remains the same */
            /* ... */
        }
        if (response) {
            snmp_free_pdu(response);
        }
    }
    snmp_close(ss);
}