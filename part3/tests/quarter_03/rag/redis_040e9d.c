void bitopCommand(client *c) {
    char *opname = c->argv[1]->ptr;
    robj *o, *targetkey = c->argv[2];
    unsigned long op, j, numkeys;
    robj **objects;
    unsigned char **src;
    unsigned long *len, maxlen = 0;
    unsigned long minlen = 0;
    unsigned char *res = NULL;

    /* Parse operation name */
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
        size_t word_size = sizeof(unsigned long);

        /* Process words when possible */
        j = 0;
        #ifndef USE_ALIGNED_ACCESS
        if (minlen >= word_size) {
            unsigned long remaining = minlen;
            while (remaining >= word_size) {
                unsigned long word = *(unsigned long*)(src[0] + j);
                
                if (op == BITOP_NOT) {
                    word = ~word;
                } else {
                    for (i = 1; i < numkeys; i++) {
                        unsigned long w = *(unsigned long*)(src[i] + j);
                        switch(op) {
                        case BITOP_AND: word &= w; break;
                        case BITOP_OR:  word |= w; break;
                        case BITOP_XOR: word ^= w; break;
                        }
                    }
                }
                *(unsigned long*)(res + j) = word;
                j += word_size;
                remaining -= word_size;
            }
        }
        #endif

        /* Process remaining bytes */
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