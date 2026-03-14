void Node::replace_by(Node *p_node, bool p_keep_groups) {
	ERR_THREAD_GUARD
	ERR_FAIL_NULL(p_node);
	ERR_FAIL_COND(p_node->data.parent);

	// Transfer groups first
	if (p_keep_groups) {
		for (const KeyValue<StringName, GroupData> &E : data.grouped) {
			p_node->add_to_group(E.key, E.value.persistent);
		}
	}

	// Redirect connections
	_replace_connections_target(p_node);

	// Handle ownership
	Node *owner = (data.owner == this) ? p_node : data.owner;
	if (data.owner) {
		_clean_up_owner();
	}

	// Reparent
	Node *parent = data.parent;
	int index_in_parent = get_index(false);
	if (parent) {
		parent->remove_child(this);
		parent->add_child(p_node);
		parent->move_child(p_node, index_in_parent);
	}

	emit_signal(SNAME("replacing_by"), p_node);

	// Transfer children in a single pass
	Vector<Node *> children_to_transfer;
	children_to_transfer.resize(get_child_count());
	for (int i = 0; i < get_child_count(); i++) {
		children_to_transfer.write[i] = get_child(i);
	}

	for (Node *child : children_to_transfer) {
		remove_child(child);
		if (!child->is_owned_by_parent()) {
			p_node->add_child(child);
		}
	}

	// Set ownership
	p_node->set_owner(owner);
	for (Node *owned_node : data.owned) {
		owned_node->set_owner(p_node);
	}

	p_node->set_scene_file_path(get_scene_file_path());
}