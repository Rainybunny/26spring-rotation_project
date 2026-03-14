int
CPCD::findUniqueId() {
  int id;
  bool ok = false;
  m_processes.lock();
  
  // Create set of used IDs for O(1) lookups
  std::unordered_set<int> used_ids;
  for(unsigned i = 0; i < m_processes.size(); i++) {
    used_ids.insert(m_processes[i]->m_id);
  }
  
  // Use a better random number generator
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dist(1, 8191); // 1-8191 inclusive
  
  while(!ok) {
    id = dist(gen);
    ok = (used_ids.find(id) == used_ids.end());
  }
  
  m_processes.unlock();
  return id;
}