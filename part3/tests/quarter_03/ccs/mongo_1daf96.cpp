void ReplSource::sync_pullOpLog_applyOperation(BSONObj& op, OpTime *localLogTail) {
    log(6) << "processing op: " << op << endl;

    // Early validation checks
    const char *ns = op.getStringField("ns");
    if (*ns == '.' || *ns == 0) {
        if (*ns == '.') {
            problem() << "skipping bad op in oplog: " << op.toString() << endl;
        } else {
            problem() << "halting replication, bad op in oplog:\n  " << op.toString() << endl;
            replAllDead = "bad object in oplog";
            throw SyncException();
        }
        return;
    }

    char clientName[MaxDatabaseLen];
    nsToDatabase(ns, clientName);

    // Skip if filtered by 'only' setting
    if (!only.empty() && only != clientName) {
        return;
    }

    // Skip if replication is dead
    if (replAllDead) {
        log() << "replAllDead, throwing SyncException: " << replAllDead << endl;
        throw SyncException();
    }

    // Always apply admin commands
    if (strcmp(clientName, "admin") == 0 && *op.getStringField("op") == 'c') {
        applyOperation(op);
        return;
    }

    // Handle pretouch if enabled
    if (cmdLine.pretouch) {
        if (cmdLine.pretouch > 1) {
            static int countdown;
            if (countdown > 0) {
                countdown--;
                assert(countdown >= 0);
            } else {
                const int m = 4;
                if (tp.get() == 0) {
                    int nthr = min(8, cmdLine.pretouch);
                    nthr = max(nthr, 1);
                    tp.reset(new ThreadPool(nthr));
                }
                vector<BSONObj> v;
                oplogReader.peek(v, cmdLine.pretouch);
                unsigned a = 0;
                while (1) {
                    if (a >= v.size()) break;
                    unsigned b = a + m - 1;
                    if (b >= v.size()) b = v.size() - 1;
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

    Client::Context ctx(ns);
    ctx.getClient()->curop()->reset();

    bool empty = ctx.db()->isEmpty();
    bool incompleteClone = incompleteCloneDbs.count(clientName) != 0;

    if (logLevel >= 6) {
        log(6) << "ns: " << ns << ", justCreated: " << ctx.justCreated() 
               << ", empty: " << empty << ", incompleteClone: " << incompleteClone << endl;
    }

    if (ctx.justCreated() || empty || incompleteClone) {
        incompleteCloneDbs.insert(clientName);
        if (nClonedThisPass) {
            addDbNextPass.insert(clientName);
        } else {
            if (incompleteClone) {
                log() << "An earlier initial clone of '" << clientName << "' did not complete, now resyncing." << endl;
            }
            save();
            Client::Context ctx(ns);
            nClonedThisPass++;
            resync(ctx.db()->name);
            addDbNextPass.erase(clientName);
            incompleteCloneDbs.erase(clientName);
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
        addDbNextPass.erase(clientName);
    }
}