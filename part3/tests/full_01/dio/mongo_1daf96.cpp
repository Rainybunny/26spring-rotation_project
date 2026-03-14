void ReplSource::sync_pullOpLog_applyOperation(BSONObj& op, OpTime *localLogTail) {
    log(6) << "processing op: " << op << endl;
    
    // Early ns validation
    const char *ns = op.getStringField("ns");
    if (!ns || *ns == '\0') {
        problem() << "halting replication, bad op in oplog:\n  " << op.toString() << endl;
        replAllDead = "bad object in oplog";
        throw SyncException();
    }
    if (*ns == '.') {
        problem() << "skipping bad op in oplog: " << op.toString() << endl;
        return;
    }

    // Extract database name more efficiently
    const char *dot = strchr(ns, '.');
    if (!dot || dot == ns) {
        problem() << "invalid ns format in oplog: " << ns << endl;
        return;
    }
    size_t dbNameLen = dot - ns;
    if (dbNameLen >= MaxDatabaseLen) {
        problem() << "database name too long in oplog: " << ns << endl;
        return;
    }
    
    // Skip if filtered by 'only' setting
    if (!only.empty() && (only.size() != dbNameLen || strncmp(only.c_str(), ns, dbNameLen) != 0)) {
        return;
    }

    if (cmdLine.pretouch) {
        if (cmdLine.pretouch > 1) {
            static int countdown;
            if (countdown > 0) {
                countdown--;
                assert(countdown >= 0);
            } else {
                const int m = 4;
                if (!tp.get()) {
                    int nthr = min(8, cmdLine.pretouch);
                    nthr = max(nthr, 1);
                    tp.reset(new ThreadPool(nthr));
                }
                vector<BSONObj> v;
                oplogReader.peek(v, cmdLine.pretouch);
                unsigned a = 0;
                while (a < v.size()) {
                    unsigned b = min(a + m - 1, v.size() - 1);
                    tp->schedule(pretouchN, v, a, b);
                    DEV cout << "pretouch task: " << a << ".." << b << endl;
                    a += m;
                }
                pretouchOperation(op);
                tp->join();
                countdown = v.size();
            }
        } else {
            pretouchOperation(op);
        }
    }

    dblock lk;

    if (localLogTail && replPair && replPair->state == ReplPair::State_Master) {
        updateSetsWithLocalOps(*localLogTail, true);
        updateSetsWithLocalOps(*localLogTail, false);
    }

    if (replAllDead) {
        log() << "replAllDead, throwing SyncException: " << replAllDead << endl;
        throw SyncException();
    }
    
    Client::Context ctx(ns);
    ctx.getClient()->curop()->reset();

    bool empty = ctx.db()->isEmpty();
    bool incompleteClone = incompleteCloneDbs.count(string(ns, dbNameLen)) != 0;

    if (logLevel >= 6) {
        log(6) << "ns: " << ns << ", justCreated: " << ctx.justCreated() 
               << ", empty: " << empty << ", incompleteClone: " << incompleteClone << endl;
    }
    
    // Handle admin commands
    if (dbNameLen == 5 && strncmp(ns, "admin.", 5) == 0 && *op.getStringField("op") == 'c') {
        applyOperation(op);
        return;
    }
    
    string dbName(ns, dbNameLen);
    if (ctx.justCreated() || empty || incompleteClone) {
        incompleteCloneDbs.insert(dbName);
        if (nClonedThisPass) {
            addDbNextPass.insert(dbName);
        } else {
            if (incompleteClone) {
                log() << "An earlier initial clone of '" << dbName << "' did not complete, now resyncing." << endl;
            }
            save();
            Client::Context ctx(ns);
            nClonedThisPass++;
            resync(ctx.db()->name);
            addDbNextPass.erase(dbName);
            incompleteCloneDbs.erase(dbName);
        }
        save();
    } else {
        bool mod;
        if (replPair && replPair->state == ReplPair::State_Master) {
            BSONObj id = idForOp(op, mod);
            if (!idTracker.haveId(ns, id)) {
                applyOperation(op);    
            } else if (idTracker.haveModId(ns, id)) {
                log(6) << "skipping operation matching mod id object " << op << endl;
                BSONObj existing;
                if (Helpers::findOne(ns, id, existing)) {
                    logOp("i", ns, existing);
                }
            } else {
                log(6) << "skipping operation matching changed id object " << op << endl;
            }
        } else {
            applyOperation(op);
        }
        addDbNextPass.erase(dbName);
    }
}