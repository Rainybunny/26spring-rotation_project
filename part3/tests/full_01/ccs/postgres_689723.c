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
	int			index;
	int			count;
	int			subcount;
	bool		suboverflowed = false;
	int			total_subxids = 0;

	Assert(!RecoveryInProgress());

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
	count = subcount = 0;

	LWLockAcquire(ProcArrayLock, LW_SHARED);
	LWLockAcquire(XidGenLock, LW_SHARED);

	latestCompletedXid =
		XidFromFullTransactionId(TransamVariables->latestCompletedXid);
	oldestDatabaseRunningXid = oldestRunningXid =
		XidFromFullTransactionId(TransamVariables->nextXid);

	/* First pass: collect main xids and check for subxid overflow */
	for (index = 0; index < arrayP->numProcs; index++)
	{
		TransactionId xid = UINT32_ACCESS_ONCE(other_xids[index]);

		if (!TransactionIdIsValid(xid))
			continue;

		if (TransactionIdPrecedes(xid, oldestRunningXid))
			oldestRunningXid = xid;

		if (arrayP->pgprocnos[index] < arrayP->maxProcs && 
			arrayP->pgprocnos[index]->databaseId == MyDatabaseId &&
			TransactionIdPrecedes(xid, oldestDatabaseRunningXid))
			oldestDatabaseRunningXid = xid;

		if (ProcGlobal->subxidStates[index].overflowed)
			suboverflowed = true;
		else
			total_subxids += ProcGlobal->subxidStates[index].count;

		xids[count++] = xid;
	}

	/* Second pass: collect subxids if no overflow */
	if (!suboverflowed && total_subxids > 0)
	{
		TransactionId *temp_subxids = (TransactionId *)
			palloc(total_subxids * sizeof(TransactionId));
		int temp_pos = 0;

		/* Collect all subxids into temp buffer */
		for (index = 0; index < arrayP->numProcs; index++)
		{
			int nsubxids = ProcGlobal->subxidStates[index].count;
			if (nsubxids > 0)
			{
				PGPROC *proc = &allProcs[arrayP->pgprocnos[index]];
				pg_read_barrier();
				memcpy(temp_subxids + temp_pos, 
					   proc->subxids.xids,
					   nsubxids * sizeof(TransactionId));
				temp_pos += nsubxids;
			}
		}

		/* Copy from temp buffer to final array */
		memcpy(xids + count, temp_subxids, total_subxids * sizeof(TransactionId));
		count += total_subxids;
		subcount = total_subxids;
		pfree(temp_subxids);
	}

	CurrentRunningXacts->xcnt = count - subcount;
	CurrentRunningXacts->subxcnt = subcount;
	CurrentRunningXacts->subxid_status = suboverflowed ? SUBXIDS_IN_SUBTRANS : SUBXIDS_IN_ARRAY;
	CurrentRunningXacts->nextXid = XidFromFullTransactionId(TransamVariables->nextXid);
	CurrentRunningXacts->oldestRunningXid = oldestRunningXid;
	CurrentRunningXacts->oldestDatabaseRunningXid = oldestDatabaseRunningXid;
	CurrentRunningXacts->latestCompletedXid = latestCompletedXid;

	return CurrentRunningXacts;
}