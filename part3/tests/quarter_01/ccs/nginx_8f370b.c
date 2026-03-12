ngx_int_t
ngx_http_upstream_init_round_robin(ngx_conf_t *cf,
    ngx_http_upstream_srv_conf_t *us)
{
    ngx_url_t                      u;
    ngx_uint_t                     i, j, n, w, is_backup;
    ngx_http_upstream_server_t    *server;
    ngx_http_upstream_rr_peers_t  *peers, *backup;

    us->peer.init = ngx_http_upstream_init_round_robin_peer;

    if (us->servers) {
        server = us->servers->elts;

        /* Count primary and backup servers in one pass */
        ngx_uint_t primary_n = 0, primary_w = 0;
        ngx_uint_t backup_n = 0, backup_w = 0;

        for (i = 0; i < us->servers->nelts; i++) {
            is_backup = server[i].backup;
            ngx_uint_t count = server[i].naddrs;
            ngx_uint_t weight = count * server[i].weight;

            if (is_backup) {
                backup_n += count;
                backup_w += weight;
            } else {
                primary_n += count;
                primary_w += weight;
            }
        }

        if (primary_n == 0) {
            ngx_log_error(NGX_LOG_EMERG, cf->log, 0,
                          "no servers in upstream \"%V\" in %s:%ui",
                          &us->host, us->file_name, us->line);
            return NGX_ERROR;
        }

        /* Create primary peers */
        peers = ngx_pcalloc(cf->pool, sizeof(ngx_http_upstream_rr_peers_t)
                              + sizeof(ngx_http_upstream_rr_peer_t) * (primary_n - 1));
        if (peers == NULL) {
            return NGX_ERROR;
        }

        peers->single = (primary_n == 1);
        peers->number = primary_n;
        peers->weighted = (primary_w != primary_n);
        peers->total_weight = primary_w;
        peers->name = &us->host;

        /* Create backup peers if needed */
        if (backup_n > 0) {
            backup = ngx_pcalloc(cf->pool, sizeof(ngx_http_upstream_rr_peers_t)
                                  + sizeof(ngx_http_upstream_rr_peer_t) * (backup_n - 1));
            if (backup == NULL) {
                return NGX_ERROR;
            }

            backup->single = 0;
            backup->number = backup_n;
            backup->weighted = (backup_w != backup_n);
            backup->total_weight = backup_w;
            backup->name = &us->host;
            peers->next = backup;
            peers->single = 0;
        }

        /* Initialize peers in one pass */
        ngx_uint_t primary_idx = 0, backup_idx = 0;

        for (i = 0; i < us->servers->nelts; i++) {
            is_backup = server[i].backup;
            
            for (j = 0; j < server[i].naddrs; j++) {
                ngx_http_upstream_rr_peer_t *peer;
                
                if (is_backup) {
                    peer = &backup->peer[backup_idx++];
                } else {
                    peer = &peers->peer[primary_idx++];
                }

                peer->sockaddr = server[i].addrs[j].sockaddr;
                peer->socklen = server[i].addrs[j].socklen;
                peer->name = server[i].addrs[j].name;
                peer->max_fails = server[i].max_fails;
                peer->fail_timeout = server[i].fail_timeout;
                peer->down = server[i].down;
                peer->weight = server[i].weight;
                peer->effective_weight = server[i].weight;
                peer->current_weight = 0;
            }
        }

        us->peer.data = peers;
        return NGX_OK;
    }

    /* Handle implicit upstream case (unchanged) */
    if (us->port == 0) {
        ngx_log_error(NGX_LOG_EMERG, cf->log, 0,
                      "no port in upstream \"%V\" in %s:%ui",
                      &us->host, us->file_name, us->line);
        return NGX_ERROR;
    }

    ngx_memzero(&u, sizeof(ngx_url_t));
    u.host = us->host;
    u.port = us->port;

    if (ngx_inet_resolve_host(cf->pool, &u) != NGX_OK) {
        if (u.err) {
            ngx_log_error(NGX_LOG_EMERG, cf->log, 0,
                          "%s in upstream \"%V\" in %s:%ui",
                          u.err, &us->host, us->file_name, us->line);
        }
        return NGX_ERROR;
    }

    n = u.naddrs;
    peers = ngx_pcalloc(cf->pool, sizeof(ngx_http_upstream_rr_peers_t)
                              + sizeof(ngx_http_upstream_rr_peer_t) * (n - 1));
    if (peers == NULL) {
        return NGX_ERROR;
    }

    peers->single = (n == 1);
    peers->number = n;
    peers->weighted = 0;
    peers->total_weight = n;
    peers->name = &us->host;

    for (i = 0; i < u.naddrs; i++) {
        peers->peer[i].sockaddr = u.addrs[i].sockaddr;
        peers->peer[i].socklen = u.addrs[i].socklen;
        peers->peer[i].name = u.addrs[i].name;
        peers->peer[i].weight = 1;
        peers->peer[i].effective_weight = 1;
        peers->peer[i].current_weight = 0;
        peers->peer[i].max_fails = 1;
        peers->peer[i].fail_timeout = 10;
    }

    us->peer.data = peers;
    return NGX_OK;
}