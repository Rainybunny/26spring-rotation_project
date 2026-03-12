int
CPCD::findUniqueId() {
  int id;
  bool ok = false;
  m_processes.lock();
  
  while(!ok) {
    ok = true;
    id = rand() % 8192; /* Don't want so big numbers */

    if(id == 0)
      ok = false;

    const unsigned num_processes = m_processes.size();
    for(unsigned i = 0; i < num_processes; i++) {
      if(m_processes[i]->m_id == id)
        ok = false;
    }
  }
  m_processes.unlock();
  return id;
}