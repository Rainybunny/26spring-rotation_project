ngx_int_t
ngx_http_upstream_init_round_robin(ngx_conf_t *cf,
    ngx_http_upstream_srv_conf_t *us)
{
    ngx_url_t                      u;
    ngx_uint_t                     i, j, n, w, primary_n, backup_n;
    ngx_http_upstream_server_t    *server;
    ngx_http_upstream_rr_peers_t  *peers, *backup;

    us->peer.init = ngx_http_upstream_init_round_robin_peer;

    if (us->servers) {
        server = us->servers->elts;
        primary_n = 0;
        backup_n = 0;
        w = 0;

        /* Single pass to count primary and backup servers */
        for (i = 0; i < us->servers->nelts; i++) {
            if (server[i].backup) {
                backup_n += server[i].naddrs;
            } else {
                primary_n += server[i].naddrs;
                w += server[i].naddrs * server[i].weight;
            }
        }

        if (primary_n == 0) {
            ngx_log_error(NGX_LOG_EMERG, cf->log, 0,
                          "no servers in upstream \"%V\" in %s:%ui",
                          &us->host, us->file_name, us->line);
            return NGX_ERROR;
        }

        /* Initialize primary peers */
        peers = ngx_pcalloc(cf->pool, sizeof(ngx_http_upstream_rr_peers_t)
                              + sizeof(ngx_http_upstream_rr_peer_t) * (primary_n - 1));
        if (peers == NULL) {
            return NGX_ERROR;
        }

        peers->single = (primary_n == 1);
        peers->number = primary_n;
        peers->weighted = (w != primary_n);
        peers->total_weight = w;
        peers->name = &us->host;

        n = 0;
        for (i = 0; i < us->servers->nelts; i++) {
            if (server[i].backup) continue;
            for (j = 0; j < server[i].naddrs; j++) {
                peers->peer[n].sockaddr = server[i].addrs[j].sockaddr;
                peers->peer[n].socklen = server[i].addrs[j].socklen;
                peers->peer[n].name = server[i].addrs[j].name;
                peers->peer[n].max_fails = server[i].max_fails;
                peers->peer[n].fail_timeout = server[i].fail_timeout;
                peers->peer[n].down = server[i].down;
                peers->peer[n].weight = server[i].weight;
                peers->peer[n].effective_weight = server[i].weight;
                peers->peer[n].current_weight = 0;
                n++;
            }
        }

        us->peer.data = peers;

        /* Initialize backup peers if any */
        if (backup_n == 0) {
            return NGX_OK;
        }

        w = 0;
        for (i = 0; i < us->servers->nelts; i++) {
            if (!server[i].backup) continue;
            w += server[i].naddrs * server[i].weight;
        }

        backup = ngx_pcalloc(cf->pool, sizeof(ngx_http_upstream_rr_peers_t)
                              + sizeof(ngx_http_upstream_rr_peer_t) * (backup_n - 1));
        if (backup == NULL) {
            return NGX_ERROR;
        }

        peers->single = 0;
        backup->single = 0;
        backup->number = backup_n;
        backup->weighted = (w != backup_n);
        backup->total_weight = w;
        backup->name = &us->host;

        n = 0;
        for (i = 0; i < us->servers->nelts; i++) {
            if (!server[i].backup) continue;
            for (j = 0; j < server[i].naddrs; j++) {
                backup->peer[n] = (ngx_http_upstream_rr_peer_t) {
                    .sockaddr = server[i].addrs[j].sockaddr,
                    .socklen = server[i].addrs[j].socklen,
                    .name = server[i].addrs[j].name,
                    .weight = server[i].weight,
                    .effective_weight = server[i].weight,
                    .current_weight = 0,
                    .max_fails = server[i].max_fails,
                    .fail_timeout = server[i].fail_timeout,
                    .down = server[i].down
                };
                n++;
            }
        }

        peers->next = backup;
        return NGX_OK;
    }

    /* Handle implicitly defined upstream */
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

    for (i = 0; i < n; i++) {
        peers->peer[i] = (ngx_http_upstream_rr_peer_t) {
            .sockaddr = u.addrs[i].sockaddr,
            .socklen = u.addrs[i].socklen,
            .name = u.addrs[i].name,
            .weight = 1,
            .effective_weight = 1,
            .current_weight = 0,
            .max_fails = 1,
            .fail_timeout = 10
        };
    }

    us->peer.data = peers;
    return NGX_OK;
}