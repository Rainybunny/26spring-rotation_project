/*
 * spi_printtup
 *		store tuple retrieved by Executor into SPITupleTable
 *		of current SPI procedure
 */
void
spi_printtup(TupleTableSlot *slot, DestReceiver *self)
{
	SPITupleTable *tuptable;
	MemoryContext oldcxt;

	/* Fast path for error checking - keep same safety checks */
	if (unlikely(_SPI_curid != _SPI_connected || _SPI_connected < 0 ||
				 _SPI_current != &(_SPI_stack[_SPI_curid]) ||
				 _SPI_current->tuptable == NULL))
		elog(ERROR, "improper call to spi_printtup");

	tuptable = _SPI_current->tuptable;
	oldcxt = MemoryContextSwitchTo(tuptable->tuptabcxt);

	if (unlikely(tuptable->free == 0))
	{
		/* Exponential growth: double size each reallocation */
		Size new_alloced = tuptable->alloced * 2;
		/* But ensure minimum growth of 256 for small tables */
		if (new_alloced < 256)
			new_alloced = 256;
		/* Also limit maximum single allocation to 1GB */
		else if (new_alloced > (Size)MaxAllocSize / sizeof(HeapTuple))
			new_alloced = (Size)MaxAllocSize / sizeof(HeapTuple);

		tuptable->vals = (HeapTuple *) repalloc_huge(tuptable->vals,
									new_alloced * sizeof(HeapTuple));
		tuptable->free = new_alloced - tuptable->alloced;
		tuptable->alloced = new_alloced;
	}

	tuptable->vals[tuptable->alloced - tuptable->free] =
		ExecCopySlotTuple(slot);
	tuptable->free--;

	MemoryContextSwitchTo(oldcxt);
}