void ThreadExit(DWORD ThreadId)
{
    EXCLUSIVE_ACQUIRE(LockThreads);
    
    // Directly find and erase the thread if it exists
    auto itr = threadList.find(ThreadId);
    if(itr != threadList.end())
    {
        threadList.erase(itr);
    }
    
    EXCLUSIVE_RELEASE();
    GuiUpdateThreadView();
}