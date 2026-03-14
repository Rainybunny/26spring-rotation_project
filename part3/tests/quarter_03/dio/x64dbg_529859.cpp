void ThreadExit(DWORD ThreadId)
{
    // First find the element without holding the lock
    EXCLUSIVE_ACQUIRE(LockThreads);
    auto itr = threadList.find(ThreadId);
    if(itr != threadList.end())
    {
        threadList.erase(itr);
    }
    EXCLUSIVE_RELEASE();
    GuiUpdateThreadView();
}