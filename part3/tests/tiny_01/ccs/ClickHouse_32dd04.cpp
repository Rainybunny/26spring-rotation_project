void DatabaseOrdinary::loadStoredObjects(
    Context & context,
    bool has_force_restore_data_flag)
{
    using FileNames = std::map<std::string, ASTPtr>;
    FileNames file_names;

    size_t total_dictionaries = 0;
    iterateMetadataFiles(context, [context, &file_names, &total_dictionaries, this](const String & file_name)
    {
        String full_path = getMetadataPath() + file_name;
        try
        {
            auto ast = parseQueryFromMetadata(context, full_path, /*throw_on_error*/ true, /*remove_empty*/false);
            if (ast)
            {
                auto * create_query = ast->as<ASTCreateQuery>();
                file_names[file_name] = ast;
                total_dictionaries += create_query->is_dictionary;
            }
        }
        catch (Exception & e)
        {
            e.addMessage("Cannot parse definition from metadata file " + full_path);
            throw;
        }
    });

    size_t total_tables = file_names.size() - total_dictionaries;
    LOG_INFO(log, "Total " << total_tables << " tables and " << total_dictionaries << " dictionaries.");

    AtomicStopwatch watch;
    std::atomic<size_t> tables_processed{0};
    std::atomic<size_t> dictionaries_processed{0};

    // Use separate pools for tables and dictionaries
    ThreadPool table_pool(SettingMaxThreads().getAutoValue());
    ThreadPool dictionary_pool(std::max(1UL, SettingMaxThreads().getAutoValue() / 2));

    // Process tables and dictionaries in parallel
    for (const auto & name_with_query : file_names)
    {
        const auto & create_query = name_with_query.second->as<const ASTCreateQuery &>();
        if (create_query.is_dictionary)
        {
            dictionary_pool.scheduleOrThrowOnError([&]()
            {
                tryAttachDictionary(context, create_query, *this);
                logAboutProgress(log, ++dictionaries_processed, total_dictionaries, watch);
            });
        }
        else
        {
            table_pool.scheduleOrThrowOnError([&]()
            {
                tryAttachTable(context, create_query, *this, getDatabaseName(), has_force_restore_data_flag);
                logAboutProgress(log, ++tables_processed, total_tables, watch);
            });
        }
    }

    // Wait for both pools to complete
    table_pool.wait();
    dictionary_pool.wait();

    // Startup tables after all are attached
    startupTables(table_pool);
    attachToExternalDictionariesLoader(context);
}