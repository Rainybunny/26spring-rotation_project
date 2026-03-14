static int Handshake(intf_thread_t *p_this)
{
    char *psz_username, *psz_password;
    char *psz_scrobbler_url;
    time_t timestamp;
    char psz_timestamp[21];

    vlc_hash_md5_t struct_md5;
    stream_t *p_stream;
    char *psz_handshake_url;
    uint8_t p_buffer[1024];
    char *p_buffer_pos, *p_end;
    int i_ret;
    char *psz_url;
    intf_thread_t *p_intf = (intf_thread_t*) p_this;
    intf_sys_t *p_sys = p_this->p_sys;

    psz_username = var_InheritString(p_this, "lastfm-username");
    psz_password = var_InheritString(p_this, "lastfm-password");

    if (EMPTY_STR(psz_username) || EMPTY_STR(psz_password)) {
        free(psz_username);
        free(psz_password);
        return VLC_EINVAL;
    }

    time(&timestamp);
    snprintf(psz_timestamp, sizeof(psz_timestamp), "%"PRIu64, (uint64_t)timestamp);

    /* Single MD5 init/update/finish sequence */
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

    psz_scrobbler_url = var_InheritString(p_this, "scrobbler-url");
    if (!psz_scrobbler_url) {
        free(psz_username);
        return VLC_ENOMEM;
    }

    i_ret = asprintf(&psz_handshake_url,
        "http://%s/?hs=true&p=1.2&c="CLIENT_NAME"&v="CLIENT_VERSION"&u=%s&t=%s&a=%s",
        psz_scrobbler_url, psz_username, psz_timestamp, psz_auth_token);
    free(psz_scrobbler_url);
    free(psz_username);
    if (i_ret == -1)
        return VLC_ENOMEM;

    p_stream = vlc_stream_NewURL(p_intf, psz_handshake_url);
    free(psz_handshake_url);
    if (!p_stream)
        return VLC_EGENERIC;

    i_ret = vlc_stream_Read(p_stream, p_buffer, sizeof(p_buffer) - 1);
    if (i_ret <= 0) {
        vlc_stream_Delete(p_stream);
        return VLC_EGENERIC;
    }
    p_buffer[i_ret] = '\0';
    vlc_stream_Delete(p_stream);

    /* Single-pass response parsing */
    p_buffer_pos = (char*)p_buffer;
    p_end = p_buffer_pos + i_ret;

    if (strstr(p_buffer_pos, "FAILED ")) {
        msg_Err(p_this, "last.fm handshake failed: %s", p_buffer_pos + 7);
        return VLC_EGENERIC;
    }

    if (strstr(p_buffer_pos, "BADAUTH")) {
        vlc_dialog_display_error(p_this, _("last.fm: Authentication failed"),
            "%s", _("last.fm username or password is incorrect. Please verify your settings and relaunch VLC."));
        return VLC_AUDIOSCROBBLER_EFATAL;
    }

    if (strstr(p_buffer_pos, "BANNED")) {
        msg_Err(p_intf, "This version of VLC has been banned by last.fm. You should upgrade VLC, or disable the last.fm plugin.");
        return VLC_AUDIOSCROBBLER_EFATAL;
    }

    if (strstr(p_buffer_pos, "BADTIME")) {
        msg_Err(p_intf, "last.fm handshake failed because your clock is too much shifted. Please correct it, and relaunch VLC.");
        return VLC_AUDIOSCROBBLER_EFATAL;
    }

    p_buffer_pos = strstr(p_buffer_pos, "OK");
    if (!p_buffer_pos)
        goto proto;

    /* Extract auth token */
    p_buffer_pos = strchr(p_buffer_pos, '\n');
    if (!p_buffer_pos || (p_end - p_buffer_pos) < 33)
        goto proto;
    p_buffer_pos++;

    memcpy(p_sys->psz_auth_token, p_buffer_pos, 32);
    p_sys->psz_auth_token[32] = '\0';

    /* Extract URLs in single pass */
    while ((p_buffer_pos = strstr(p_buffer_pos, "http://"))) {
        char *url_end = strpbrk(p_buffer_pos, "\n\r");
        if (!url_end)
            break;

        psz_url = strndup(p_buffer_pos, url_end - p_buffer_pos);
        if (!psz_url)
            goto oom;

        vlc_url_t *target = (p_sys->p_nowp_url.psz_host == NULL) 
            ? &p_sys->p_nowp_url 
            : &p_sys->p_submit_url;
            
        vlc_UrlParse(target, psz_url);
        free(psz_url);
        
        if (target->psz_host == NULL || target->i_port == 0) {
            vlc_UrlClean(target);
            if (target == &p_sys->p_nowp_url)
                continue;
            goto proto;
        }
        
        p_buffer_pos = url_end;
    }

    if (p_sys->p_nowp_url.psz_host && p_sys->p_submit_url.psz_host)
        return VLC_SUCCESS;

proto:
    msg_Err(p_intf, "Handshake: can't recognize server protocol");
    return VLC_EGENERIC;

oom:
    return VLC_ENOMEM;
}