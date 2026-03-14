SortedAggregateBindData(ClientContext &context, BoundAggregateExpression &expr)
	    : buffer_manager(BufferManager::GetBufferManager(context)), function(expr.function),
	      bind_info(std::move(expr.bind_info)), threshold(ClientConfig::GetConfig(context).ordered_aggregate_threshold),
	      external(ClientConfig::GetConfig(context).force_external) {
		auto &children = expr.children;
		const auto num_children = children.size();
		arg_types.reserve(num_children);
		arg_funcs.reserve(num_children);
		for (const auto &child : children) {
			arg_types.emplace_back(child->return_type);
			ListSegmentFunctions funcs;
			GetSegmentDataFunctions(funcs, arg_types.back());
			arg_funcs.emplace_back(funcs);
		}
		auto &order_bys = *expr.order_bys;
		const auto num_orders = order_bys.orders.size();
		sort_types.reserve(num_orders);
		sort_funcs.reserve(num_orders);
		orders.reserve(num_orders);
		for (auto &order : order_bys.orders) {
			orders.emplace_back(order.Copy());
			sort_types.emplace_back(order.expression->return_type);
			ListSegmentFunctions funcs;
			GetSegmentDataFunctions(funcs, sort_types.back());
			sort_funcs.emplace_back(funcs);
		}
		sorted_on_args = (num_children == num_orders);
		for (size_t i = 0; sorted_on_args && i < num_children; ++i) {
			sorted_on_args = children[i]->Equals(*order_bys.orders[i].expression);
		}
	}