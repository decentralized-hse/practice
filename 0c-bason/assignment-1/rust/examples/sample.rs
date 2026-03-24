use bason_codec::{bason_encode, bason_to_json, json_to_bason, BasonRecord};

fn main() {
    let record = BasonRecord::object(
        "",
        vec![
            BasonRecord::string("name", "Alice"),
            BasonRecord::number("age", "21"),
        ],
    );

    let bytes = bason_encode(&record).unwrap();
    println!("encoded {} bytes", bytes.len());
    println!("{}", bason_to_json(&bytes).unwrap());

    let json = r#"{"name":"Bob","active":true}"#;
    let bytes = json_to_bason(json).unwrap();
    println!("json_to_bason produced {} bytes", bytes.len());
}
