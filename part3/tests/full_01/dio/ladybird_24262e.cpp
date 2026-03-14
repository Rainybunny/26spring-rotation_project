Vector<Region*> AddressSpace::find_regions_intersecting(VirtualRange const& range)
{
    Vector<Region*> regions;
    regions.ensure_capacity(4); // Most cases will have <=4 intersecting regions
    size_t total_size_collected = 0;

    SpinlockLocker lock(m_lock);

    auto found_region = m_regions.find_largest_not_above(range.base().get());
    if (!found_region)
        return regions;
    for (auto iter = m_regions.begin_from((*found_region)->vaddr().get()); !iter.is_end(); ++iter) {
        auto* region = *iter;
        auto intersection = region->range().intersect(range);
        if (!intersection.is_empty()) {
            regions.unchecked_append(region);
            total_size_collected += region->size() - intersection.size();
            if (total_size_collected == range.size())
                break;
        }
    }

    return regions;
}