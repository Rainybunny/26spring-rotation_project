ngx_int_t
ngx_http_upstream_init_round_robin(ngx_conf_t *cf,
    ngx_http_upstream_srv_conf_t *us)
{
    ngx_url_t                      u;
    ngx_uint_t                     i, j, n, w, b_n, b_w;
    ngx_http_upstream_server_t    *server;
    ngx_http_upstream_rr_peers_t  *peers, *backup;

    us->peer.init = ngx_http_upstream_init_round_robin_peer;

    if (us->servers) {
        server = us->servers->elts;

        n = 0;
        w = 0;
        b_n = 0;
        b_w = 0;

        /* Single pass to count both regular and backup servers */
        for (i = 0; i < us->servers->nelts; i++) {
            if (server[i].backup) {
                b_n += server[i].naddrs;
                b_w += server[i].naddrs * server[i].weight;
            } else {
                n += server[i].naddrs;
                w += server[i].naddrs * server[i].weight;
            }
        }

        if (n == 0) {
            ngx_log_error(NGX_LOG_EMERG, cf->log, 0,
                          "no servers in upstream \"%V\" in %s:%ui",
                          &us->host, us->file_name, us->line);
            return NGX_ERROR;
        }

        peers = ngx_pcalloc(cf->pool, sizeof(ngx_http_upstream_rr_peers_t)
                              + sizeof(ngx_http_upstream_rr_peer_t) * (n - 1));
        if (peers == NULL) {
            return NGX_ERROR;
        }

        peers->single = (n == 1);
        peers->number = n;
        peers->weighted = (w != n);
        peers->total_weight = w;
        peers->name = &us->host;

        /* Initialize regular servers */
        n = 0;
        for (i = 0; i < us->servers->nelts; i++) {
            if (server[i].backup) {
                continue;
            }
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

        /* Handle backup servers if any */
        if (b_n > 0) {
            backup = ngx_pcalloc(cf->pool, sizeof(ngx_http_upstream_rr_peers_t)
                              + sizeof(ngx_http_upstream_rr_peer_t) * (b_n - 1));
            if (backup == NULL) {
                return NGX_ERROR;
            }

            peers->single = 0;
            backup->single = 0;
            backup->number = b_n;
            backup->weighted = (b_w != b_n);
            backup->total_weight = b_w;
            backup->name = &us->host;

            /* Initialize backup servers */
            b_n = 0;
            for (i = 0; i < us->servers->nelts; i++) {
                if (!server[i].backup) {
                    continue;
                }
                for (j = 0; j < server[i].naddrs; j++) {
                    backup->peer[b_n].sockaddr = server[i].addrs[j].sockaddr;
                    backup->peer[b_n].socklen = server[i].addrs[j].socklen;
                    backup->peer[b_n].name = server[i].addrs[j].name;
                    backup->peer[b_n].weight = server[i].weight;
                    backup->peer[b_n].effective_weight = server[i].weight;
                    backup->peer[b_n].current_weight = 0;
                    backup->peer[b_n].max_fails = server[i].max_fails;
                    backup->peer[b_n].fail_timeout = server[i].fail_timeout;
                    backup->peer[b_n].down = server[i].down;
                    b_n++;
                }
            }

            peers->next = backup;
        }

        return NGX_OK;
    }

    /* an upstream implicitly defined by proxy_pass, etc. */

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