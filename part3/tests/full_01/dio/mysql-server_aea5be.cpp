int
CPCD::findUniqueId() {
  static const int MAX_ID = 8192;
  static const int MIN_ID = 1;
  static unsigned char id_bitmask[(MAX_ID + 7) / 8] = {0};
  
  m_processes.lock();
  
  // First, update the bitmask with current processes
  memset(id_bitmask, 0, sizeof(id_bitmask));
  for(unsigned i = 0; i < m_processes.size(); i++) {
    int id = m_processes[i]->m_id;
    if(id >= MIN_ID && id < MAX_ID) {
      id_bitmask[id / 8] |= (1 << (id % 8));
    }
  }
  
  // Find a random available ID
  int id;
  do {
    id = MIN_ID + (rand() % (MAX_ID - MIN_ID));
  } while(id_bitmask[id / 8] & (1 << (id % 8)));
  
  m_processes.unlock();
  return id;
}