void _update( Request& r , DbMessage& d, ChunkManagerPtr manager ){
            int flags = d.pullInt();
            
            BSONObj query = d.nextJsObj();
            uassert( 10201 ,  "invalid update" , d.moreJSObjs() );
            BSONObj toupdate = d.nextJsObj();

            BSONObj chunkFinder = query;
            
            bool upsert = flags & UpdateOption_Upsert;
            bool multi = flags & UpdateOption_Multi;

            if ( multi )
                uassert( 10202 ,  "can't mix multi and upsert and sharding" , ! upsert );

            const bool hasShardKeyInUpdate = manager->hasShardKey(toupdate);
            if ( upsert && !(hasShardKeyInUpdate ||
                             (toupdate.firstElement().fieldName()[0] == '$' && manager->hasShardKey(query))))
            {
                throw UserException( 8012 , "can't upsert something without shard key" );
            }

            bool save = false;
            if ( ! manager->hasShardKey( query ) ){
                if ( multi ){
                }
                else if ( query.nFields() != 1 || strcmp( query.firstElement().fieldName() , "_id" ) ){
                    throw UserException( 8013 , "can't do update with query that doesn't have the shard key" );
                }
                else {
                    save = true;
                    chunkFinder = toupdate;
                }
            }

            if ( ! save ){
                const ShardKeyPattern& shardKey = manager->getShardKey();
                if ( toupdate.firstElement().fieldName()[0] == '$' ){
                    BSONObjIterator ops(toupdate);
                    while(ops.more()){
                        BSONElement op(ops.next());
                        if (op.type() != Object)
                            continue;
                        BSONObjIterator fields(op.embeddedObject());
                        while(fields.more()){
                            if (shardKey.partOfShardKey(fields.next().fieldName())) {
                                uasserted(13123, "Can't modify shard key's value");
                            }
                        }
                    }
                } else if ( hasShardKeyInUpdate ){
                    uassert( 8014, "change would move shards!", shardKey.compare( query , toupdate ) == 0 );
                } else {
                    uasserted(12376, "shard key must be in update object");
                }
            }
            
            if ( multi ){
                set<Shard> shards;
                manager->getShardsForQuery( shards , chunkFinder );
                for ( set<Shard>::iterator i=shards.begin(); i!=shards.end(); i++){
                    doWrite( dbUpdate , r , *i );
                }
            }
            else {
                ChunkPtr c = manager->findChunk( chunkFinder );
                doWrite( dbUpdate , r , c->getShard() );
                c->splitIfShould( d.msg().header()->dataLen() );
            }
        }