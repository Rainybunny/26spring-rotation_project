template <typename AggregationKeyChecker>
    ColumnPtr executeImpl(const ColumnsWithTypeAndName & arguments, size_t input_rows_count, AggregationKeyChecker checker) const
    {
        auto grouping_set_column = checkAndGetColumn<ColumnUInt64>(arguments[0].column.get());
        auto result = std::make_shared<DataTypeUInt64>()->createColumn();
        
        size_t i = 0;
        const size_t unroll = 4;
        const size_t limit = input_rows_count / unroll * unroll;
        
        for (; i < limit; i += unroll)
        {
            UInt64 set_indices[unroll];
            UInt64 values[unroll] = {0};
            
            // Prefetch and process 4 rows at a time
            for (size_t j = 0; j < unroll; ++j)
                set_indices[j] = grouping_set_column->get64(i + j);
            
            for (auto index : arguments_indexes)
            {
                for (size_t j = 0; j < unroll; ++j)
                {
                    values[j] = (values[j] << 1) + (checker(set_indices[j], index) ? 1 : 0);
                }
            }
            
            for (size_t j = 0; j < unroll; ++j)
                result->insert(Field(values[j]));
        }
        
        // Process remaining rows
        for (; i < input_rows_count; ++i)
        {
            UInt64 set_index = grouping_set_column->get64(i);
            UInt64 value = 0;
            for (auto index : arguments_indexes)
                value = (value << 1) + (checker(set_index, index) ? 1 : 0);
            result->insert(Field(value));
        }
        
        return result;
    }