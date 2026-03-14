int
CPCD::findUniqueId() {
  int id;
  m_processes.lock();
  
  // Pre-compute used IDs
  std::unordered_set<int> used_ids;
  for(unsigned i = 0; i<m_processes.size(); i++) {
    used_ids.insert(m_processes[i]->m_id);
  }

  while(true) {
    id = rand() % 8192; /* Don't want so big numbers */
    if(id == 0) continue;
    if(used_ids.find(id) == used_ids.end()) break;
  }
  
  m_processes.unlock();
  return id;
}