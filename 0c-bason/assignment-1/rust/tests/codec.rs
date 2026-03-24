use bason_codec::{
    bason_decode, bason_decode_all, bason_encode, bason_flatten, bason_to_json, bason_unflatten,
    json_to_bason, ron64_decode, ron64_encode, BasonRecord, BasonType,
};
use proptest::prelude::*;
use serde_json::Value;

#[test]
fn encodes_long_form_using_spec_layout() {
    let record = BasonRecord::string("0123456789abcdef", "hello");
    let bytes = bason_encode(&record).unwrap();

    assert_eq!(bytes[0], b'S');
    assert_eq!(u32::from_le_bytes(bytes[1..5].try_into().unwrap()), 5);
    assert_eq!(bytes[5], 16);
}

#[test]
fn decode_all_handles_concatenated_records() {
    let a = bason_encode(&BasonRecord::string("a", "x")).unwrap();
    let b = bason_encode(&BasonRecord::number("b", "42")).unwrap();
    let mut buf = Vec::new();
    buf.extend_from_slice(&a);
    buf.extend_from_slice(&b);

    let records = bason_decode_all(&buf).unwrap();
    assert_eq!(records.len(), 2);
    assert_eq!(records[0].r#type, BasonType::String);
    assert_eq!(records[1].r#type, BasonType::Number);
}

#[test]
fn flattened_stream_round_trips_back_to_nested() {
    let nested = BasonRecord::object(
        "",
        vec![
            BasonRecord::string("name", "Alice"),
            BasonRecord::array(
                "scores",
                vec![BasonRecord::number("0", "95"), BasonRecord::number("1", "87")],
            ),
        ],
    );
    let flat = bason_flatten(&nested);
    let rebuilt = bason_unflatten(&flat).unwrap();
    assert_eq!(rebuilt, nested);
}

proptest! {
    #[test]
    fn ron64_round_trip(n in any::<u64>()) {
        let s = ron64_encode(n);
        prop_assert_eq!(ron64_decode(&s).unwrap(), n);
    }

    #[test]
    fn short_leaf_round_trip(key in "[A-Za-z0-9_]{0,8}", value in "[A-Za-z0-9_ ]{0,8}") {
        let record = BasonRecord::string(key, value);
        let bytes = bason_encode(&record).unwrap();
        let (decoded, used) = bason_decode(&bytes).unwrap();
        prop_assert_eq!(used, bytes.len());
        prop_assert_eq!(decoded, record);
    }
}

#[test]
fn json_round_trip_for_mixed_scalars() {
    let json = r#"{
      "name": "Alice",
      "active": true,
      "age": 21,
      "ratio": 1.5,
      "tags": ["db", "rust"],
      "none": null
    }"#;

    let bason = json_to_bason(json).unwrap();
    let back = bason_to_json(&bason).unwrap();

    let a: Value = serde_json::from_str(json).unwrap();
    let b: Value = serde_json::from_str(&back).unwrap();
    assert_eq!(a, b);
}
