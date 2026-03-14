SortedAggregateBindData(ClientContext &context, BoundAggregateExpression &expr)
	    : buffer_manager(BufferManager::GetBufferManager(context)), function(expr.function),
	      bind_info(std::move(expr.bind_info)), threshold(ClientConfig::GetConfig(context).ordered_aggregate_threshold),
	      external(ClientConfig::GetConfig(context).force_external) {
		auto &children = expr.children;
		auto &order_bys = *expr.order_bys;
		
		// Early exit for size mismatch
		sorted_on_args = (children.size() == order_bys.orders.size());
		
		// Reserve space for both types and functions
		arg_types.reserve(children.size());
		arg_funcs.reserve(children.size());
		sort_types.reserve(order_bys.orders.size());
		sort_funcs.reserve(order_bys.orders.size());
		
		// Process children and orders in parallel where possible
		size_t i = 0;
		for (; i < children.size(); ++i) {
			// Process child
			arg_types.emplace_back(children[i]->return_type);
			ListSegmentFunctions funcs;
			GetSegmentDataFunctions(funcs, arg_types.back());
			arg_funcs.emplace_back(funcs);
			
			// Process order if within bounds and checking sorted_on_args
			if (sorted_on_args && i < order_bys.orders.size()) {
				orders.emplace_back(order_bys.orders[i].Copy());
				sort_types.emplace_back(order_bys.orders[i].expression->return_type);
				ListSegmentFunctions sort_funcs;
				GetSegmentDataFunctions(sort_funcs, sort_types.back());
				sort_funcs.emplace_back(sort_funcs);
				sorted_on_args = children[i]->Equals(*order_bys.orders[i].expression);
			}
		}
		
		// Process remaining orders if any
		for (; i < order_bys.orders.size(); ++i) {
			orders.emplace_back(order_bys.orders[i].Copy());
			sort_types.emplace_back(order_bys.orders[i].expression->return_type);
			ListSegmentFunctions funcs;
			GetSegmentDataFunctions(funcs, sort_types.back());
			sort_funcs.emplace_back(funcs);
		}
	}