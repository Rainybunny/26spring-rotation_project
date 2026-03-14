size_t Exchange::getTargetConsumer(const Document& input) {
    // Cache frequently accessed member variables
    const auto& keyPattern = _keyPattern;
    const auto& keyPaths = _keyPaths;
    const auto ordering = _ordering;
    const auto& boundaries = _boundaries;
    const auto& consumerIds = _consumerIds;

    // Build the key.
    BSONObjBuilder kb;
    size_t counter = 0;
    for (auto elem : keyPattern) {
        auto value = input.getNestedField(keyPaths[counter]);

        // By definition we send documents with missing fields to the consumer 0.
        if (value.missing()) {
            return 0;
        }

        if (elem.type() == BSONType::String && elem.str() == "hashed") {
            kb << ""
               << BSONElementHasher::hash64(BSON("" << value).firstElement(),
                                            BSONElementHasher::DEFAULT_HASH_SEED);
        } else {
            kb << "" << value;
        }
        ++counter;
    }

    KeyString::Builder key{KeyString::Version::V1, kb.obj(), ordering};
    std::string keyStr{key.getBuffer(), key.getSize()};

    // Binary search for the consumer id.
    auto it = std::upper_bound(boundaries.begin(), boundaries.end(), keyStr);
    invariant(it != boundaries.end());

    size_t distance = std::distance(boundaries.begin(), it) - 1;
    invariant(distance < consumerIds.size());

    size_t cid = consumerIds[distance];
    invariant(cid < _consumers.size());

    return cid;
}