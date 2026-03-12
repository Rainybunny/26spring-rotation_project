Vector<Region*> AddressSpace::find_regions_intersecting(VirtualRange const& range)
{
    Vector<Region*> regions;
    size_t total_size_collected = 0;
    auto range_start = range.base().get();
    auto range_end = range.end().get();

    SpinlockLocker lock(m_lock);

    auto found_region = m_regions.find_largest_not_above(range_start);
    if (!found_region)
        return regions;
        
    for (auto iter = m_regions.begin_from((*found_region)->vaddr().get()); !iter.is_end(); ++iter) {
        auto* region = *iter;
        auto region_start = region->range().base().get();
        auto region_end = region->range().end().get();
        
        if (region_start < range_end && region_end > range_start) {
            regions.append(region);
            
            total_size_collected += region->size() - (max(region_start, range_start) - min(region_end, range_end));
            if (total_size_collected == range.size())
                break;
        }
    }

    return regions;
}