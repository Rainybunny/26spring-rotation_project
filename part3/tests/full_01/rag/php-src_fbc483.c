FILE *php_fopen_url_wrap_http(char *path, char *mode, int options, int *issock, int *socketd, char **opened_path)
{
	FILE *fp=NULL;
	php_url *resource=NULL;
	char tmp_line[128];
	char location[512];
	char hdr_line[8192];
	int body = 0;
	char *scratch;
	unsigned char *tmp;
	int len;
	int reqok = 0;
	zval *response_header;
	char *http_header_line;
	int http_header_line_length, http_header_line_size;

	resource = php_url_parse((char *) path);
	if (resource == NULL) {
		php_error(E_WARNING, "Invalid URL specified, %s", path);
		*issock = BAD_URL;
		return NULL;
	}
	if (resource->port == 0) {
		resource->port = 80;
	}
	
	*socketd = php_hostconnect(resource->host, resource->port, SOCK_STREAM, 0);
	if (*socketd == -1) {
		SOCK_FCLOSE(*socketd);
		*socketd = 0;
		php_url_free(resource);
		return NULL;
	}
	
	len = snprintf(hdr_line, sizeof(hdr_line), "GET %s%s%s HTTP/1.0\r\n",
		resource->path ? resource->path : "/",
		resource->query ? "?" : "",
		resource->query ? resource->query : "");
	SOCK_WRITE(hdr_line, *socketd);
	
	if (resource->user != NULL && resource->pass != NULL) {
		int auth_len = strlen(resource->user) + strlen(resource->pass) + 1;
		scratch = (char *) emalloc(auth_len + 1);
		if (!scratch) {
			php_url_free(resource);
			return NULL;
		}
		snprintf(scratch, auth_len + 1, "%s:%s", resource->user, resource->pass);
		tmp = php_base64_encode((unsigned char *)scratch, strlen(scratch), NULL);
		
		if (snprintf(hdr_line, sizeof(hdr_line), "Authorization: Basic %s\r\n", tmp) > 0) {
			SOCK_WRITE(hdr_line, *socketd);
		}
		
		efree(scratch);
		efree(tmp);
	}

	if (cfg_get_string("from", &scratch) == SUCCESS) {
		if (snprintf(hdr_line, sizeof(hdr_line), "From: %s\r\n", scratch) > 0) {
			SOCK_WRITE(hdr_line, *socketd);
		}
	}

	len = snprintf(hdr_line, sizeof(hdr_line), "Host: %s%s\r\n",
		resource->host,
		resource->port != 80 ? ":" : "",
		resource->port != 80 ? resource->port : "");
	if (len > 0) {
		SOCK_WRITE(hdr_line, *socketd);
	}
	
	SOCK_WRITE("User-Agent: PHP/" PHP_VERSION "\r\n\r\n", *socketd);
	
	body = 0;
	location[0] = '\0';

	MAKE_STD_ZVAL(response_header);
	array_init(response_header);

	if (!SOCK_FEOF(*socketd) && SOCK_FGETS(tmp_line, sizeof(tmp_line)-1, *socketd) != NULL) {
		zval *http_response;
		char *p = tmp_line;

		MAKE_STD_ZVAL(http_response);
		if (strncmp(tmp_line + 8, " 200 ", 5) == 0) {
			reqok = 1;
		}
		while (*p && *p != '\n' && *p != '\r') p++;
		*p = '\0';
		ZVAL_STRING(http_response, tmp_line, 1);
		zend_hash_next_index_insert(Z_ARRVAL_P(response_header), &http_response, sizeof(zval *), NULL);
	}

	/* Optimized header reading loop */
	while (!body && !SOCK_FEOF(*socketd)) {
		http_header_line_size = HTTP_HEADER_BLOCK_SIZE * 2;
		http_header_line = emalloc(http_header_line_size);
		http_header_line_length = 0;

		while (!body && SOCK_FGETS(http_header_line + http_header_line_length, 
				http_header_line_size - http_header_line_length - 1, *socketd) != NULL) {
			char *p = http_header_line + http_header_line_length;
			char *end = p + strlen(p);
			
			http_header_line_length += (end - p);
			
			if ((p = strpbrk(http_header_line, "\r\n")) != NULL) {
				*p = '\0';
				http_header_line_length = p - http_header_line;
				body = (*p == '\n' && p > http_header_line && *(p-1) == '\r') || 
					   (*p == '\n' && p == http_header_line);
				break;
			}
			
			if (http_header_line_size - http_header_line_length < HTTP_HEADER_BLOCK_SIZE) {
				http_header_line_size *= 2;
				http_header_line = erealloc(http_header_line, http_header_line_size);
			}
		}

		if (!strncasecmp(http_header_line, "Location: ", 10)) {
			strlcpy(location, http_header_line + 10, sizeof(location));
		}

		if (http_header_line_length > 0) {
			zval *http_header;
			MAKE_STD_ZVAL(http_header);
			ZVAL_STRINGL(http_header, http_header_line, http_header_line_length, 0);
			zend_hash_next_index_insert(Z_ARRVAL_P(response_header), &http_header, sizeof(zval *), NULL);
		} else {
			efree(http_header_line);
		}
	}

	if (!reqok) {
		SOCK_FCLOSE(*socketd);
		*socketd = 0;
		php_url_free(resource);
		if (location[0] != '\0') {
			zval **response_header_new, *entry, **entryp;
			ELS_FETCH();

			fp = php_fopen_url_wrap_http(location, mode, options, issock, socketd, opened_path);
			if (zend_hash_find(EG(active_symbol_table), "http_response_header", sizeof("http_response_header"), (void **) &response_header_new) == SUCCESS) {
				entryp = &entry;
				MAKE_STD_ZVAL(entry);
				ZVAL_EMPTY_STRING(entry);
				zend_hash_next_index_insert(Z_ARRVAL_P(response_header), entryp, sizeof(zval *), NULL);
				zend_hash_internal_pointer_reset(Z_ARRVAL_PP(response_header_new));
				while (zend_hash_get_current_data(Z_ARRVAL_PP(response_header_new), (void **)&entryp) == SUCCESS) {
					zval_add_ref(entryp);
					zend_hash_next_index_insert(Z_ARRVAL_P(response_header), entryp, sizeof(zval *), NULL);
					zend_hash_move_forward(Z_ARRVAL_PP(response_header_new));
				}
			}
			goto out;
		} else {
			fp = NULL;
			goto out;
		}
	}
	php_url_free(resource);
	*issock = 1;
 out:
	{
		ELS_FETCH();
		ZEND_SET_SYMBOL(EG(active_symbol_table), "http_response_header", response_header);
	}	
	return (fp);
}