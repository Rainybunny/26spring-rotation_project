void bitopCommand(client *c) {
    char *opname = c->argv[1]->ptr;
    robj *o, *targetkey = c->argv[2];
    unsigned long op, j, numkeys;
    robj **objects;
    unsigned char **src;
    unsigned long *len, maxlen = 0;
    unsigned long minlen = 0;
    unsigned char *res = NULL;

    /* Parse the operation name. */
    if ((opname[0] == 'a' || opname[0] == 'A') && !strcasecmp(opname,"and"))
        op = BITOP_AND;
    else if((opname[0] == 'o' || opname[0] == 'O') && !strcasecmp(opname,"or"))
        op = BITOP_OR;
    else if((opname[0] == 'x' || opname[0] == 'X') && !strcasecmp(opname,"xor"))
        op = BITOP_XOR;
    else if((opname[0] == 'n' || opname[0] == 'N') && !strcasecmp(opname,"not"))
        op = BITOP_NOT;
    else {
        addReply(c,shared.syntaxerr);
        return;
    }

    if (op == BITOP_NOT && c->argc != 4) {
        addReplyError(c,"BITOP NOT must be called with a single source key.");
        return;
    }

    numkeys = c->argc - 3;
    src = zmalloc(sizeof(unsigned char*) * numkeys);
    len = zmalloc(sizeof(long) * numkeys);
    objects = zmalloc(sizeof(robj*) * numkeys);
    for (j = 0; j < numkeys; j++) {
        o = lookupKeyRead(c->db,c->argv[j+3]);
        if (o == NULL) {
            objects[j] = NULL;
            src[j] = NULL;
            len[j] = 0;
            minlen = 0;
            continue;
        }
        if (checkType(c,o,OBJ_STRING)) {
            unsigned long i;
            for (i = 0; i < j; i++) {
                if (objects[i])
                    decrRefCount(objects[i]);
            }
            zfree(src);
            zfree(len);
            zfree(objects);
            return;
        }
        objects[j] = getDecodedObject(o);
        src[j] = objects[j]->ptr;
        len[j] = sdslen(objects[j]->ptr);
        if (len[j] > maxlen) maxlen = len[j];
        if (j == 0 || len[j] < minlen) minlen = len[j];
    }

    if (maxlen) {
        res = (unsigned char*) sdsnewlen(NULL,maxlen);
        unsigned long i;
        j = 0;

        #ifndef USE_ALIGNED_ACCESS
        if (minlen >= sizeof(unsigned long)*4 && numkeys <= 16) {
            unsigned long *lp[16];
            unsigned long *lres = (unsigned long*) res;
            memcpy(lp,src,sizeof(unsigned long*)*numkeys);
            memcpy(res,src[0],minlen);

            if (op == BITOP_AND) {
                while(minlen >= sizeof(unsigned long)*4) {
                    for (i = 1; i < numkeys; i++) {
                        lres[0] &= lp[i][0];
                        lres[1] &= lp[i][1];
                        lres[2] &= lp[i][2];
                        lres[3] &= lp[i][3];
                        lp[i]+=4;
                    }
                    lres+=4;
                    j += sizeof(unsigned long)*4;
                    minlen -= sizeof(unsigned long)*4;
                }
            } else if (op == BITOP_OR) {
                while(minlen >= sizeof(unsigned long)*4) {
                    for (i = 1; i < numkeys; i++) {
                        lres[0] |= lp[i][0];
                        lres[1] |= lp[i][1];
                        lres[2] |= lp[i][2];
                        lres[3] |= lp[i][3];
                        lp[i]+=4;
                    }
                    lres+=4;
                    j += sizeof(unsigned long)*4;
                    minlen -= sizeof(unsigned long)*4;
                }
            } else if (op == BITOP_XOR) {
                while(minlen >= sizeof(unsigned long)*4) {
                    for (i = 1; i < numkeys; i++) {
                        lres[0] ^= lp[i][0];
                        lres[1] ^= lp[i][1];
                        lres[2] ^= lp[i][2];
                        lres[3] ^= lp[i][3];
                        lp[i]+=4;
                    }
                    lres+=4;
                    j += sizeof(unsigned long)*4;
                    minlen -= sizeof(unsigned long)*4;
                }
            } else if (op == BITOP_NOT) {
                while(minlen >= sizeof(unsigned long)*4) {
                    lres[0] = ~lres[0];
                    lres[1] = ~lres[1];
                    lres[2] = ~lres[2];
                    lres[3] = ~lres[3];
                    lres+=4;
                    j += sizeof(unsigned long)*4;
                    minlen -= sizeof(unsigned long)*4;
                }
            }
        }
        #endif

        /* Process remaining bytes in 4-byte chunks */
        while (j + 4 <= maxlen) {
            uint32_t output;
            if (len[0] <= j) {
                output = 0;
            } else {
                memcpy(&output, src[0]+j, 4);
            }
            
            if (op == BITOP_NOT) {
                output = ~output;
            } else {
                for (i = 1; i < numkeys; i++) {
                    uint32_t val;
                    if (len[i] <= j) {
                        val = 0;
                    } else {
                        memcpy(&val, src[i]+j, 4);
                    }
                    switch(op) {
                    case BITOP_AND: output &= val; break;
                    case BITOP_OR:  output |= val; break;
                    case BITOP_XOR: output ^= val; break;
                    }
                }
            }
            memcpy(res+j, &output, 4);
            j += 4;
        }

        /* Process remaining bytes one by one */
        for (; j < maxlen; j++) {
            unsigned char output = (len[0] <= j) ? 0 : src[0][j];
            if (op == BITOP_NOT) output = ~output;
            for (i = 1; i < numkeys; i++) {
                unsigned char byte = (len[i] <= j) ? 0 : src[i][j];
                switch(op) {
                case BITOP_AND: output &= byte; break;
                case BITOP_OR:  output |= byte; break;
                case BITOP_XOR: output ^= byte; break;
                }
            }
            res[j] = output;
        }
    }

    for (j = 0; j < numkeys; j++) {
        if (objects[j])
            decrRefCount(objects[j]);
    }
    zfree(src);
    zfree(len);
    zfree(objects);

    if (maxlen) {
        o = createObject(OBJ_STRING,res);
        setKey(c,c->db,targetkey,o);
        notifyKeyspaceEvent(NOTIFY_STRING,"set",targetkey,c->db->id);
        decrRefCount(o);
        server.dirty++;
    } else if (dbDelete(c->db,targetkey)) {
        signalModifiedKey(c,c->db,targetkey);
        notifyKeyspaceEvent(NOTIFY_GENERIC,"del",targetkey,c->db->id);
        server.dirty++;
    }
    addReplyLongLong(c,maxlen);
}