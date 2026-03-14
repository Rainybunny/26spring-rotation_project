void PipelineExecutor::finalizeExecution()
{
    if (process_list_element && process_list_element->isKilled())
        throw Exception("Query was cancelled", ErrorCodes::QUERY_WAS_CANCELLED);

    if (cancelled)
        return;

    for (auto & node : graph)
    {
        if (node.status != ExecStatus::Finished)  /// Single thread, do not hold mutex
            throw Exception("Pipeline stuck. Current state:\n" + dumpPipeline(), ErrorCodes::LOGICAL_ERROR);
    }
}