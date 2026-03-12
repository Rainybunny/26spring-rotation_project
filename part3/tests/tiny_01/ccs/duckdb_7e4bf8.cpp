SortedAggregateBindData(ClientContext &context, BoundAggregateExpression &expr)
	    : buffer_manager(BufferManager::GetBufferManager(context)), function(expr.function),
	      bind_info(std::move(expr.bind_info)), threshold(ClientConfig::GetConfig(context).ordered_aggregate_threshold),
	      external(ClientConfig::GetConfig(context).force_external) {
		auto &children = expr.children;
		arg_types.reserve(children.size());
		arg_funcs.reserve(children.size());
		
		// Cache for ListSegmentFunctions to avoid repeated calls for same types
		unordered_map<LogicalType, ListSegmentFunctions> func_cache;
		
		for (const auto &child : children) {
			const auto &type = child->return_type;
			arg_types.emplace_back(type);
			auto it = func_cache.find(type);
			if (it == func_cache.end()) {
				ListSegmentFunctions funcs;
				GetSegmentDataFunctions(funcs, type);
				arg_funcs.emplace_back(funcs);
				func_cache[type] = funcs;
			} else {
				arg_funcs.emplace_back(it->second);
			}
		}
		
		auto &order_bys = *expr.order_bys;
		sort_types.reserve(order_bys.orders.size());
		sort_funcs.reserve(order_bys.orders.size());
		for (auto &order : order_bys.orders) {
			orders.emplace_back(order.Copy());
			const auto &type = order.expression->return_type;
			sort_types.emplace_back(type);
			auto it = func_cache.find(type);
			if (it == func_cache.end()) {
				ListSegmentFunctions funcs;
				GetSegmentDataFunctions(funcs, type);
				sort_funcs.emplace_back(funcs);
				func_cache[type] = funcs;
			} else {
				sort_funcs.emplace_back(it->second);
			}
		}
		sorted_on_args = (children.size() == order_bys.orders.size());
		for (size_t i = 0; sorted_on_args && i < children.size(); ++i) {
			sorted_on_args = children[i]->Equals(*order_bys.orders[i].expression);
		}
	}