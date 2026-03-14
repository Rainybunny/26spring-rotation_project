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
	/* use port 80 if one wasn't specified */
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
	
	strcpy(hdr_line, "GET ");
	
	/* tell remote http which file to get */
	if (resource->path != NULL && strlen(resource->path)) {
		strlcat(hdr_line, resource->path, sizeof(hdr_line));
	} else {
		strlcat(hdr_line, "/", sizeof(hdr_line));
	}
	/* append the query string, if any */
	if (resource->query != NULL) {
		strlcat(hdr_line, "?", sizeof(hdr_line));
		strlcat(hdr_line, resource->query, sizeof(hdr_line));
	}
	strlcat(hdr_line, " HTTP/1.0\r\n", sizeof(hdr_line));
	SOCK_WRITE(hdr_line, *socketd);
	
	/* send authorization header if we have user/pass */
	if (resource->user != NULL && resource->pass != NULL) {
		int user_len = strlen(resource->user);
		int pass_len = strlen(resource->pass);
		scratch = (char *) emalloc(user_len + pass_len + 2);
		if (!scratch) {
			php_url_free(resource);
			return NULL;
		}
		memcpy(scratch, resource->user, user_len);
		scratch[user_len] = ':';
		memcpy(scratch + user_len + 1, resource->pass, pass_len + 1);
		tmp = php_base64_encode((unsigned char *)scratch, user_len + pass_len + 1, NULL);
		
		if (snprintf(hdr_line, sizeof(hdr_line),
					 "Authorization: Basic %s\r\n", tmp) > 0) {
			SOCK_WRITE(hdr_line, *socketd);
		}
		
		efree(scratch);
		efree(tmp);
	}
	/* if the user has configured who they are, send a From: line */
	if (cfg_get_string("from", &scratch) == SUCCESS) {
		if (snprintf(hdr_line, sizeof(hdr_line),
					 "From: %s\r\n", scratch) > 0) {
			SOCK_WRITE(hdr_line, *socketd);
		}
	}
	/* send a Host: header so name-based virtual hosts work */
	if (resource->port != 80) {
		len = snprintf(hdr_line, sizeof(hdr_line),
					   "Host: %s:%i\r\n", resource->host, resource->port);
	} else {
		len = snprintf(hdr_line, sizeof(hdr_line),
					   "Host: %s\r\n", resource->host);
	}
	if(len > sizeof(hdr_line) - 1) {
		len = sizeof(hdr_line) - 1;
	}
	if (len > 0) {
		SOCK_WRITE(hdr_line, *socketd);
	}
	
	/* identify ourselves and end the headers */
	SOCK_WRITE("User-Agent: PHP/" PHP_VERSION "\r\n\r\n", *socketd);
	
	body = 0;
	location[0] = '\0';

	MAKE_STD_ZVAL(response_header);
	array_init(response_header);

	if (!SOCK_FEOF(*socketd)) {
		/* get response header */
		if (SOCK_FGETS(tmp_line, sizeof(tmp_line)-1, *socketd) != NULL) {
			zval *http_response;

			MAKE_STD_ZVAL(http_response);
			if (strncmp(tmp_line + 8, " 200 ", 5) == 0) {
				reqok = 1;
			}
			Z_STRLEN_P(http_response) = strlen(tmp_line);
			Z_STRVAL_P(http_response) = estrndup(tmp_line, Z_STRLEN_P(http_response));
			if (Z_STRVAL_P(http_response)[Z_STRLEN_P(http_response)-1]=='\n') {
				Z_STRVAL_P(http_response)[Z_STRLEN_P(http_response)-1]=0;
				Z_STRLEN_P(http_response)--;
				if (Z_STRVAL_P(http_response)[Z_STRLEN_P(http_response)-1]=='\r') {
					Z_STRVAL_P(http_response)[Z_STRLEN_P(http_response)-1]=0;
					Z_STRLEN_P(http_response)--;
				}
			}
			Z_TYPE_P(http_response) = IS_STRING;
			zend_hash_next_index_insert(Z_ARRVAL_P(response_header), &http_response, sizeof(zval *), NULL);
		}
	}

	/* Optimized header reading loop */
	while (!body && !SOCK_FEOF(*socketd)) {
		http_header_line_size = HTTP_HEADER_BLOCK_SIZE * 4; // Start with larger chunk
		http_header_line = emalloc(http_header_line_size);
		http_header_line_length = 0;
		
		while (!body && !SOCK_FEOF(*socketd)) {
			char *p = http_header_line + http_header_line_length;
			int read_size = http_header_line_size - http_header_line_length - 1;
			
			if (read_size <= 0) {
				http_header_line_size *= 2;
				http_header_line = erealloc(http_header_line, http_header_line_size);
				p = http_header_line + http_header_line_length;
				read_size = http_header_line_size - http_header_line_length - 1;
			}
			
			if (SOCK_FGETS(p, read_size, *socketd) == NULL) {
				break;
			}
			
			int line_len = strlen(p);
			http_header_line_length += line_len;
			
			// Check for end of headers
			if (line_len <= 2 && (p[0] == '\n' || (p[0] == '\r' && p[1] == '\n'))) {
				body = 1;
				break;
			}
			
			// Check for Location header
			if (!strncasecmp(p, "Location: ", 10)) {
				strlcpy(location, p + 10, sizeof(location));
			}
		}
		
		if (http_header_line_length > 0) {
			// Trim trailing CRLF
			while (http_header_line_length > 0 && 
				  (http_header_line[http_header_line_length-1] == '\n' || 
				   http_header_line[http_header_line_length-1] == '\r')) {
				http_header_line[--http_header_line_length] = '\0';
			}
			
			if (http_header_line_length > 0) {
				zval *http_header;
				MAKE_STD_ZVAL(http_header);
				Z_STRVAL_P(http_header) = http_header_line;
				Z_STRLEN_P(http_header) = http_header_line_length;
				Z_TYPE_P(http_header) = IS_STRING;
				zend_hash_next_index_insert(Z_ARRVAL_P(response_header), &http_header, sizeof(zval *), NULL);
			} else {
				efree(http_header_line);
			}
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