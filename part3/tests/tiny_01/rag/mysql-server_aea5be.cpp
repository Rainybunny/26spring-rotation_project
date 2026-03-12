int
CPCD::findUniqueId() {
  static unsigned char id_bitmap[1024] = {0}; // 8192 bits
  int id;
  
  m_processes.lock();
  
  // First pass: populate bitmap from existing processes
  memset(id_bitmap, 0, sizeof(id_bitmap));
  for(unsigned i = 0; i < m_processes.size(); i++) {
    int used_id = m_processes[i]->m_id;
    if(used_id > 0 && used_id < 8192) {
      id_bitmap[used_id/8] |= (1 << (used_id%8));
    }
  }

  // Find first available ID
  for(id = 1; id < 8192; id++) {
    if(!(id_bitmap[id/8] & (1 << (id%8)))) {
      break;
    }
  }
  
  m_processes.unlock();
  return (id < 8192) ? id : -1; // Return -1 if no IDs available (unlikely)
}