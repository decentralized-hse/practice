use bason_codec::{bason_validate, strictness, BasonRecord};

#[test]
fn rejects_duplicate_object_keys() {
    let record = BasonRecord::object(
        "",
        vec![
            BasonRecord::string("name", "Alice"),
            BasonRecord::string("name", "Bob"),
        ],
    );

    assert!(!bason_validate(&record, strictness::NO_DUPLICATE_KEYS));
}

#[test]
fn rejects_non_contiguous_array_indices() {
    let record = BasonRecord::array(
        "scores",
        vec![
            BasonRecord::number("0", "95"),
            BasonRecord::number("2", "87"),
        ],
    );

    assert!(!bason_validate(
        &record,
        strictness::CONTIGUOUS_ARRAY_INDICES
    ));
}

#[test]
fn rejects_unsorted_object_keys() {
    let record = BasonRecord::object(
        "",
        vec![
            BasonRecord::string("z", "1"),
            BasonRecord::string("a", "2"),
        ],
    );

    assert!(!bason_validate(&record, strictness::SORTED_OBJECT_KEYS));
}

#[test]
fn rejects_non_minimal_ron64() {
    let record = BasonRecord::array(
        "scores",
        vec![
            BasonRecord::number("00", "95"),
            BasonRecord::number("1", "87"),
        ],
    );

    assert!(!bason_validate(&record, strictness::MINIMAL_RON64));
}

#[test]
fn rejects_non_canonical_boolean_text() {
    let record = BasonRecord::boolean("flag", "TRUE");
    assert!(!bason_validate(
        &record,
        strictness::CANONICAL_BOOLEAN_TEXT
    ));
}
