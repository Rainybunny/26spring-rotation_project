void Node::replace_by(Node *p_node, bool p_keep_groups) {
    ERR_THREAD_GUARD
    ERR_FAIL_NULL(p_node);
    ERR_FAIL_COND(p_node->data.parent);

    // Cache frequently accessed data
    Node *current_parent = data.parent;
    const int child_count = get_child_count();
    List<Node *> owned = data.owned; // Copy once

    if (p_keep_groups) {
        List<GroupInfo> groups;
        get_groups(&groups);
        // Batch group additions
        for (const GroupInfo &E : groups) {
            p_node->add_to_group(E.name, E.persistent);
        }
    }

    _replace_connections_target(p_node);

    // Handle ownership more efficiently
    Node *new_owner = (data.owner == this) ? p_node : data.owner;
    if (data.owner) {
        List<Node *> owned_by_owner;
        owned_by_owner.reserve(child_count); // Pre-allocate
        for (int i = 0; i < child_count; i++) {
            find_owned_by(data.owner, get_child(i), &owned_by_owner);
        }
        _clean_up_owner();
    }

    if (current_parent) {
        const int index = get_index(false);
        current_parent->remove_child(this);
        current_parent->add_child(p_node);
        current_parent->move_child(p_node, index);
    }

    emit_signal(SNAME("replacing_by"), p_node);

    // Process children more efficiently
    for (int i = 0; i < child_count; ) {
        Node *child = get_child(0); // Always get first child since we're removing
        remove_child(child);
        if (!child->is_owned_by_parent()) {
            p_node->add_child(child);
        }
    }

    // Batch owner assignments
    p_node->set_owner(new_owner);
    for (Node *owned_node : owned) {
        owned_node->set_owner(p_node);
    }

    p_node->set_scene_file_path(get_scene_file_path());
}