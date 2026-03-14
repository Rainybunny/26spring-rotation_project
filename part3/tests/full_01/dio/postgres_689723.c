RunningTransactions
GetRunningTransactionData(void)
{
    /* result workspace */
    static RunningTransactionsData CurrentRunningXactsData;

    ProcArrayStruct *arrayP = procArray;
    TransactionId *other_xids = ProcGlobal->xids;
    RunningTransactions CurrentRunningXacts = &CurrentRunningXactsData;
    TransactionId latestCompletedXid;
    TransactionId oldestRunningXid;
    TransactionId oldestDatabaseRunningXid;
    TransactionId *xids;
    int            index;
    int            count = 0;
    int            subcount = 0;
    bool        suboverflowed = false;

    Assert(!RecoveryInProgress());

    /* Allocate xids array if first call */
    if (CurrentRunningXacts->xids == NULL)
    {
        CurrentRunningXacts->xids = (TransactionId *)
            malloc(TOTAL_MAX_CACHED_SUBXIDS * sizeof(TransactionId));
        if (CurrentRunningXacts->xids == NULL)
            ereport(ERROR,
                    (errcode(ERRCODE_OUT_OF_MEMORY),
                     errmsg("out of memory")));
    }
    xids = CurrentRunningXacts->xids;

    /* Acquire locks */
    LWLockAcquire(ProcArrayLock, LW_SHARED);
    LWLockAcquire(XidGenLock, LW_SHARED);

    latestCompletedXid =
        XidFromFullTransactionId(TransamVariables->latestCompletedXid);
    oldestDatabaseRunningXid = oldestRunningXid =
        XidFromFullTransactionId(TransamVariables->nextXid);

    /* Single scan through procArray */
    for (index = 0; index < arrayP->numProcs; index++)
    {
        int            pgprocno = arrayP->pgprocnos[index];
        PGPROC       *proc = &allProcs[pgprocno];
        TransactionId xid = UINT32_ACCESS_ONCE(other_xids[index]);
        XidCacheStatus *subxidstate = &ProcGlobal->subxidStates[index];

        if (!TransactionIdIsValid(xid))
            continue;

        /* Update oldest running xid values */
        if (TransactionIdPrecedes(xid, oldestRunningXid))
            oldestRunningXid = xid;
        if (proc->databaseId == MyDatabaseId &&
            TransactionIdPrecedes(xid, oldestDatabaseRunningXid))
            oldestDatabaseRunningXid = xid;

        /* Check for subxid overflow */
        if (subxidstate->overflowed)
            suboverflowed = true;

        /* Store xid */
        xids[count++] = xid;

        /* Collect subxids if no overflow */
        if (!suboverflowed && subxidstate->count > 0)
        {
            int nsubxids = subxidstate->count;
            pg_read_barrier();    /* pairs with GetNewTransactionId */
            memcpy(&xids[count], proc->subxids.xids,
                   nsubxids * sizeof(TransactionId));
            count += nsubxids;
            subcount += nsubxids;
        }
    }

    /* Set result fields */
    CurrentRunningXacts->xcnt = count - subcount;
    CurrentRunningXacts->subxcnt = subcount;
    CurrentRunningXacts->subxid_status = suboverflowed ? SUBXIDS_IN_SUBTRANS : SUBXIDS_IN_ARRAY;
    CurrentRunningXacts->nextXid = XidFromFullTransactionId(TransamVariables->nextXid);
    CurrentRunningXacts->oldestRunningXid = oldestRunningXid;
    CurrentRunningXacts->oldestDatabaseRunningXid = oldestDatabaseRunningXid;
    CurrentRunningXacts->latestCompletedXid = latestCompletedXid;

    Assert(TransactionIdIsValid(CurrentRunningXacts->nextXid));
    Assert(TransactionIdIsValid(CurrentRunningXacts->oldestRunningXid));
    Assert(TransactionIdIsNormal(CurrentRunningXacts->latestCompletedXid));

    /* Caller must release locks */
    return CurrentRunningXacts;
}