static int Handshake(intf_thread_t *p_this)
{
    char *psz_username = var_InheritString(p_this, "lastfm-username");
    char *psz_password = var_InheritString(p_this, "lastfm-password");

    if (EMPTY_STR(psz_username) || EMPTY_STR(psz_password)) {
        free(psz_username);
        free(psz_password);
        return VLC_EINVAL;
    }

    /* Generate auth token */
    time_t timestamp;
    time(&timestamp);
    char psz_timestamp[21];
    snprintf(psz_timestamp, sizeof(psz_timestamp), "%"PRIu64, (uint64_t)timestamp);

    vlc_hash_md5_t struct_md5;
    vlc_hash_md5_Init(&struct_md5);
    vlc_hash_md5_Update(&struct_md5, psz_password, strlen(psz_password));
    free(psz_password);

    char psz_password_md5[VLC_HASH_MD5_DIGEST_HEX_SIZE];
    vlc_hash_FinishHex(&struct_md5, psz_password_md5);

    vlc_hash_md5_Init(&struct_md5);
    vlc_hash_md5_Update(&struct_md5, psz_password_md5, sizeof(psz_password_md5) - 1);
    vlc_hash_md5_Update(&struct_md5, psz_timestamp, strlen(psz_timestamp));
    char psz_auth_token[VLC_HASH_MD5_DIGEST_HEX_SIZE];
    vlc_hash_FinishHex(&struct_md5, psz_auth_token);

    /* Build handshake URL */
    char *psz_scrobbler_url = var_InheritString(p_this, "scrobbler-url");
    if (!psz_scrobbler_url) {
        free(psz_username);
        return VLC_ENOMEM;
    }

    int i_ret = asprintf(&psz_handshake_url,
        "http://%s/?hs=true&p=1.2&c="CLIENT_NAME"&v="CLIENT_VERSION"&u=%s&t=%s&a=%s",
        psz_scrobbler_url, psz_username, psz_timestamp, psz_auth_token);
    free(psz_scrobbler_url);
    free(psz_username);
    if (i_ret == -1)
        return VLC_ENOMEM;

    /* Send request and read response */
    stream_t *p_stream = vlc_stream_NewURL(p_intf, psz_handshake_url);
    free(psz_handshake_url);
    if (!p_stream)
        return VLC_EGENERIC;

    uint8_t p_buffer[1024];
    i_ret = vlc_stream_Read(p_stream, p_buffer, sizeof(p_buffer) - 1);
    vlc_stream_Delete(p_stream);
    if (i_ret <= 0)
        return VLC_EGENERIC;
    p_buffer[i_ret] = '\0';

    /* Single-pass response parsing */
    char *response = (char *)p_buffer;
    char *line = response;
    int state = 0; // 0=first line, 1=session ID, 2=nowplaying URL, 3=submit URL
    
    while (*line) {
        char *next = strchr(line, '\n');
        if (next) *next++ = '\0';
        
        switch (state) {
            case 0: // First line
                if (strstr(line, "FAILED ")) {
                    msg_Err(p_this, "last.fm handshake failed: %s", line + 7);
                    return VLC_EGENERIC;
                }
                if (strstr(line, "BADAUTH")) {
                    vlc_dialog_display_error(p_this,
                        _("last.fm: Authentication failed"),
                        "%s", _("last.fm username or password is incorrect. "
                        "Please verify your settings and relaunch VLC."));
                    return VLC_AUDIOSCROBBLER_EFATAL;
                }
                if (strstr(line, "BANNED")) {
                    msg_Err(p_intf, "This version of VLC has been banned by last.fm. "
                             "You should upgrade VLC, or disable the last.fm plugin.");
                    return VLC_AUDIOSCROBBLER_EFATAL;
                }
                if (strstr(line, "BADTIME")) {
                    msg_Err(p_intf, "last.fm handshake failed because your clock is too "
                             "much shifted. Please correct it, and relaunch VLC.");
                    return VLC_AUDIOSCROBBLER_EFATAL;
                }
                if (!strstr(line, "OK"))
                    goto proto;
                state++;
                break;
                
            case 1: // Session ID
                if (strlen(line) >= 32) {
                    memcpy(p_sys->psz_auth_token, line, 32);
                    p_sys->psz_auth_token[32] = '\0';
                    state++;
                }
                break;
                
            case 2: // Nowplaying URL
                if (strstr(line, "http://")) {
                    char *url = strndup(line, strcspn(line, "\n"));
                    if (!url) goto oom;
                    vlc_UrlParse(&p_sys->p_nowp_url, url);
                    free(url);
                    if (!p_sys->p_nowp_url.psz_host || !p_sys->p_nowp_url.i_port) {
                        vlc_UrlClean(&p_sys->p_nowp_url);
                        goto proto;
                    }
                    state++;
                }
                break;
                
            case 3: // Submit URL
                if (strstr(line, "http://")) {
                    char *url = strndup(line, strcspn(line, "\n"));
                    if (!url) goto oom;
                    vlc_UrlParse(&p_sys->p_submit_url, url);
                    free(url);
                    if (!p_sys->p_submit_url.psz_host || !p_sys->p_submit_url.i_port) {
                        vlc_UrlClean(&p_sys->p_nowp_url);
                        vlc_UrlClean(&p_sys->p_submit_url);
                        goto proto;
                    }
                    return VLC_SUCCESS;
                }
                break;
        }
        
        if (!next) break;
        line = next;
    }

proto:
    msg_Err(p_intf, "Handshake: can't recognize server protocol");
    return VLC_EGENERIC;
oom:
    return VLC_ENOMEM;
}