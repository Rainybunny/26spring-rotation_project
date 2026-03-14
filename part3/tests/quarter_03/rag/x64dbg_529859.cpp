void ThreadExit(DWORD ThreadId)
{
    bool needsUpdate = false;
    {
        EXCLUSIVE_ACQUIRE(LockThreads);
        auto itr = threadList.find(ThreadId);
        if(itr != threadList.end())
        {
            threadList.erase(itr);
            needsUpdate = true;
        }
    }
    
    if(needsUpdate)
        GuiUpdateThreadView();
}