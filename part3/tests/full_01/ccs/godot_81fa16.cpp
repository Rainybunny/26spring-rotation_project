void Node::replace_by(Node *p_node, bool p_keep_groups) {
    ERR_THREAD_GUARD
    ERR_FAIL_NULL(p_node);
    ERR_FAIL_COND(p_node->data.parent);

    // Pre-allocate lists with estimated sizes
    List<Node *> owned = data.owned;
    List<Node *> owned_by_owner;
    owned_by_owner.reserve(get_child_count()); // Pre-allocate based on child count

    Node *owner = (data.owner == this) ? p_node : data.owner;

    // Process groups if needed
    if (p_keep_groups) {
        List<GroupInfo> groups;
        get_groups(&groups);
        for (const GroupInfo &E : groups) {
            p_node->add_to_group(E.name, E.persistent);
        }
    }

    _replace_connections_target(p_node);

    // Process ownership in a single pass
    if (data.owner) {
        for (int i = 0; i < get_child_count(); i++) {
            Node *child = get_child(i);
            if (child->get_owner() == data.owner) {
                owned_by_owner.push_back(child);
            }
        }
        _clean_up_owner();
    }

    // Handle parent relationship
    Node *parent = data.parent;
    if (parent) {
        int index_in_parent = get_index(false);
        parent->remove_child(this);
        parent->add_child(p_node);
        parent->move_child(p_node, index_in_parent);
    }

    // Process children in a single pass
    while (get_child_count()) {
        Node *child = get_child(0);
        remove_child(child);
        if (!child->is_owned_by_parent()) {
            p_node->add_child(child);
        }
    }

    // Set ownership
    p_node->set_owner(owner);
    for (Node *node : owned) {
        node->set_owner(p_node);
    }
    for (Node *node : owned_by_owner) {
        node->set_owner(owner);
    }

    p_node->set_scene_file_path(get_scene_file_path());
    emit_signal(SNAME("replacing_by"), p_node);
}