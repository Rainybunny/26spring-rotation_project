void ThreadExit(DWORD ThreadId)
{
    EXCLUSIVE_ACQUIRE(LockThreads);

    auto itr = threadList.find(ThreadId);
    if(itr != threadList.end())
        threadList.erase(itr);

    EXCLUSIVE_RELEASE();
    GuiUpdateThreadView();
}