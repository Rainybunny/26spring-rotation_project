int
CPCD::findUniqueId() {
    static int counter = 0;
    int id;
    bool ok = false;
    m_processes.lock();
    
    while(!ok) {
        ok = true;
        counter = (counter + 1) % 8192;
        id = counter ? counter : 1; // Skip 0

        for(unsigned i = 0; i < m_processes.size(); i++) {
            if(m_processes[i]->m_id == id) {
                ok = false;
                break; // Early exit on collision
            }
        }
    }
    m_processes.unlock();
    return id;
}