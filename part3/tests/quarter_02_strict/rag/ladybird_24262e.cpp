Vector<Region*> AddressSpace::find_regions_intersecting(VirtualRange const& range)
{
    Vector<Region*> regions;

    SpinlockLocker lock(m_lock);

    auto found_region = m_regions.find_largest_not_above(range.base().get());
    if (!found_region)
        return regions;
    for (auto iter = m_regions.begin_from((*found_region)->vaddr().get()); !iter.is_end(); ++iter) {
        if ((*iter)->range().base() < range.end() && (*iter)->range().end() > range.base())
            regions.append(*iter);
    }

    return regions;
}