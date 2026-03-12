int
CPCD::findUniqueId() {
  int id = 0;
  m_processes.lock();
  
  // First try sequential search (fast path for sparse ID space)
  for(int attempt = 1; attempt <= 8191; attempt++) {
    bool found = false;
    for(unsigned i = 0; i < m_processes.size(); i++) {
      if(m_processes[i]->m_id == attempt) {
        found = true;
        break;
      }
    }
    if(!found) {
      id = attempt;
      break;
    }
  }
  
  // Fall back to random search if sequential failed
  if(id == 0) {
    bool ok = false;
    while(!ok) {
      ok = true;
      id = rand() % 8191 + 1; // 1-8191

      for(unsigned i = 0; i < m_processes.size(); i++) {
        if(m_processes[i]->m_id == id) {
          ok = false;
          break;
        }
      }
    }
  }
  
  m_processes.unlock();
  return id;
}