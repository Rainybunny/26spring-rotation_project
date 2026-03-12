int
CPCD::findUniqueId() {
  int id;
  bool ok = false;
  m_processes.lock();
  
  // Try a few times with better random distribution
  for (int attempts = 0; attempts < 10 && !ok; attempts++) {
    id = 1 + (rand() % 8191); // Directly generate in 1-8191 range
    ok = true;
    
    // Fast check for collision
    for(unsigned i = 0; i < m_processes.size(); i++) {
      if(m_processes[i]->m_id == id) {
        ok = false;
        break;
      }
    }
  }

  // Fallback to original method if still not found
  while(!ok) {
    ok = true;
    id = rand() % 8192;
    if(id == 0) continue;

    for(unsigned i = 0; i < m_processes.size(); i++) {
      if(m_processes[i]->m_id == id) {
        ok = false;
        break;
      }
    }
  }
  
  m_processes.unlock();
  return id;
}