int
CPCD::findUniqueId() {
  int id;
  bool ok = false;
  int retries = 0;
  const int max_retries = 100;
  unsigned process_count;
  
  m_processes.lock();
  process_count = m_processes.size();
  
  while(!ok && retries++ < max_retries) {
    ok = true;
    // Better random distribution than simple rand()
    id = 1 + (int)((double)8191 * rand() / (RAND_MAX + 1.0));

    for(unsigned i = 0; i < process_count; i++) {
      if(m_processes[i]->m_id == id) {
        ok = false;
        break;  // Early exit once found
      }
    }
  }
  
  // Fallback to sequential search if random failed
  if(!ok) {
    for(id = 1; id < 8192; id++) {
      ok = true;
      for(unsigned i = 0; i < process_count; i++) {
        if(m_processes[i]->m_id == id) {
          ok = false;
          break;
        }
      }
      if(ok) break;
    }
  }
  
  m_processes.unlock();
  return id;
}