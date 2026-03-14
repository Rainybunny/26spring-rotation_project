void Node::replace_by(Node *p_node, bool p_keep_groups) {
	ERR_THREAD_GUARD
	ERR_FAIL_NULL(p_node);
	ERR_FAIL_COND(p_node->data.parent);

	List<Node *> owned = data.owned;
	List<Node *> owned_by_owner;
	Node *owner = (data.owner == this) ? p_node : data.owner;

	// Pre-allocate owned_by_owner if we have an owner
	if (data.owner) {
		owned_by_owner.reserve(get_child_count());
		for (int i = 0; i < get_child_count(); i++) {
			find_owned_by(data.owner, get_child(i), &owned_by_owner);
		}
		_clean_up_owner();
	}

	// Handle groups more efficiently
	if (p_keep_groups) {
		for (const KeyValue<StringName, GroupData> &E : data.grouped) {
			p_node->add_to_group(E.key, E.value.persistent);
		}
	}

	_replace_connections_target(p_node);

	Node *parent = data.parent;
	if (parent) {
		int index_in_parent = get_index(false);
		parent->remove_child(this);
		parent->add_child(p_node);
		parent->move_child(p_node, index_in_parent);
	}

	emit_signal(SNAME("replacing_by"), p_node);

	// Transfer children more efficiently
	int child_count = get_child_count();
	for (int i = 0; i < child_count; i++) {
		Node *child = get_child(0); // Always get first child since we're removing
		remove_child(child);
		if (!child->is_owned_by_parent()) {
			p_node->add_child(child);
		}
	}

	p_node->set_owner(owner);
	
	// Use range-based for where possible
	for (Node *node : owned) {
		node->set_owner(p_node);
	}
	for (Node *node : owned_by_owner) {
		node->set_owner(owner);
	}

	p_node->set_scene_file_path(get_scene_file_path());
}