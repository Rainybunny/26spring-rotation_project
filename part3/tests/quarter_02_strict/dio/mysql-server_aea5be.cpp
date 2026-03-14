int
CPCD::findUniqueId() {
  int id;
  int attempts = 0;
  const int max_attempts = 1000; // Prevent infinite loops
  m_processes.lock();
  
  do {
    // Better random distribution than modulo
    id = 1 + (int)((double)rand() / ((double)RAND_MAX + 1) * 8191);
    bool found = false;
    
    for(unsigned i = 0; i < m_processes.size(); i++) {
      if(m_processes[i]->m_id == id) {
        found = true;
        break;
      }
    }
    
    if(!found) {
      m_processes.unlock();
      return id;
    }
    
    attempts++;
  } while(attempts < max_attempts);
  
  // Fallback: linear search for first available ID
  for(id = 1; id <= 8191; id++) {
    bool found = false;
    for(unsigned i = 0; i < m_processes.size(); i++) {
      if(m_processes[i]->m_id == id) {
        found = true;
        break;
      }
    }
    if(!found) {
      m_processes.unlock();
      return id;
    }
  }
  
  m_processes.unlock();
  return -1; // All IDs exhausted (should never happen)
}