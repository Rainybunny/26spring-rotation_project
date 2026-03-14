Vector<Region*> AddressSpace::find_regions_intersecting(VirtualRange const& range)
{
    Vector<Region*> regions;
    const auto range_end = range.end();

    SpinlockLocker lock(m_lock);

    auto found_region = m_regions.find_largest_not_above(range.base().get());
    if (!found_region)
        return regions;

    for (auto iter = m_regions.begin_from((*found_region)->vaddr().get()); !iter.is_end(); ++iter) {
        auto* region = *iter;
        const auto& region_range = region->range();
        
        // Simplified intersection check
        if (region_range.base() < range_end && region_range.end() > range.base()) {
            regions.append(region);
        }
    }

    return regions;
}