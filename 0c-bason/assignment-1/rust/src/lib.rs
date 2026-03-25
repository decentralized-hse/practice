use std::collections::{BTreeMap, HashSet};

use serde_json::{Map, Value};
use thiserror::Error;

pub const RON64_ALPHABET: &str = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ_abcdefghijklmnopqrstuvwxyz~";

pub mod strictness {
    pub const SHORTEST_ENCODING: u16 = 0x001;
    pub const CANONICAL_NUMBER_FORMAT: u16 = 0x002;
    pub const VALID_UTF8: u16 = 0x004;
    pub const NO_DUPLICATE_KEYS: u16 = 0x008;
    pub const CONTIGUOUS_ARRAY_INDICES: u16 = 0x010;
    pub const ORDERED_ARRAY_INDICES: u16 = 0x020;
    pub const SORTED_OBJECT_KEYS: u16 = 0x040;
    pub const CANONICAL_BOOLEAN_TEXT: u16 = 0x080;
    pub const MINIMAL_RON64: u16 = 0x100;
    pub const CANONICAL_PATH_FORMAT: u16 = 0x200;
    pub const NO_FLAT_NESTED_MIXING: u16 = 0x400;

    pub const PERMISSIVE: u16 = 0x000;
    pub const STANDARD: u16 = 0x1FF;
    pub const STRICT: u16 = 0x7FF;
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, serde::Serialize, serde::Deserialize)]
pub enum BasonType {
    Boolean,
    Array,
    String,
    Object,
    Number,
}

#[derive(Debug, Clone, PartialEq, Eq, serde::Serialize, serde::Deserialize)]
pub struct BasonRecord {
    pub r#type: BasonType,
    pub key: String,
    pub value: String,
    pub children: Vec<BasonRecord>,
}

impl BasonRecord {
    pub fn boolean<K: Into<String>, V: Into<String>>(key: K, value: V) -> Self {
        Self {
            r#type: BasonType::Boolean,
            key: key.into(),
            value: value.into(),
            children: Vec::new(),
        }
    }

    pub fn string<K: Into<String>, V: Into<String>>(key: K, value: V) -> Self {
        Self {
            r#type: BasonType::String,
            key: key.into(),
            value: value.into(),
            children: Vec::new(),
        }
    }

    pub fn number<K: Into<String>, V: Into<String>>(key: K, value: V) -> Self {
        Self {
            r#type: BasonType::Number,
            key: key.into(),
            value: value.into(),
            children: Vec::new(),
        }
    }

    pub fn object<K: Into<String>>(key: K, children: Vec<BasonRecord>) -> Self {
        Self {
            r#type: BasonType::Object,
            key: key.into(),
            value: String::new(),
            children,
        }
    }

    pub fn array<K: Into<String>>(key: K, children: Vec<BasonRecord>) -> Self {
        Self {
            r#type: BasonType::Array,
            key: key.into(),
            value: String::new(),
            children,
        }
    }

    pub fn is_container(&self) -> bool {
        matches!(self.r#type, BasonType::Array | BasonType::Object)
    }
}

#[derive(Debug, Error)]
pub enum BasonError {
    #[error("unexpected end of input")]
    UnexpectedEof,
    #[error("invalid tag byte: 0x{0:02x}")]
    InvalidTag(u8),
    #[error("record key length exceeds long-form limit of 255 bytes: {0}")]
    KeyTooLong(usize),
    #[error("record payload length exceeds long-form limit of 2^32-1 bytes: {0}")]
    ValueTooLong(usize),
    #[error("invalid UTF-8 in {0}")]
    InvalidUtf8(&'static str),
    #[error("invalid RON64 character: {0}")]
    InvalidRon64Char(char),
    #[error("malformed number text: {0}")]
    InvalidNumber(String),
    #[error("malformed boolean text: {0}")]
    InvalidBoolean(String),
    #[error("malformed BASON: {0}")]
    Malformed(String),
    #[error("JSON conversion error: {0}")]
    Json(String),
}

pub type Result<T> = std::result::Result<T, BasonError>;

fn short_tag(t: BasonType) -> u8 {
    match t {
        BasonType::Boolean => b'b',
        BasonType::Array => b'a',
        BasonType::String => b's',
        BasonType::Object => b'o',
        BasonType::Number => b'n',
    }
}

fn long_tag(t: BasonType) -> u8 {
    match t {
        BasonType::Boolean => b'B',
        BasonType::Array => b'A',
        BasonType::String => b'S',
        BasonType::Object => b'O',
        BasonType::Number => b'N',
    }
}

fn decode_tag(tag: u8) -> Result<(BasonType, bool)> {
    match tag {
        b'b' => Ok((BasonType::Boolean, true)),
        b'a' => Ok((BasonType::Array, true)),
        b's' => Ok((BasonType::String, true)),
        b'o' => Ok((BasonType::Object, true)),
        b'n' => Ok((BasonType::Number, true)),
        b'B' => Ok((BasonType::Boolean, false)),
        b'A' => Ok((BasonType::Array, false)),
        b'S' => Ok((BasonType::String, false)),
        b'O' => Ok((BasonType::Object, false)),
        b'N' => Ok((BasonType::Number, false)),
        other => Err(BasonError::InvalidTag(other)),
    }
}

pub fn bason_encode(record: &BasonRecord) -> Result<Vec<u8>> {
    let payload = encode_payload(record)?;
    let key_bytes = record.key.as_bytes();

    if key_bytes.len() > u8::MAX as usize {
        return Err(BasonError::KeyTooLong(key_bytes.len()));
    }
    if payload.len() > u32::MAX as usize {
        return Err(BasonError::ValueTooLong(payload.len()));
    }

    let use_short = key_bytes.len() <= 15 && payload.len() <= 15;

    let mut out = Vec::with_capacity(2 + key_bytes.len() + payload.len());
    out.push(if use_short {
        short_tag(record.r#type)
    } else {
        long_tag(record.r#type)
    });

    if use_short {
        let lens = ((key_bytes.len() as u8) << 4) | (payload.len() as u8);
        out.push(lens);
    } else {
        out.extend_from_slice(&(payload.len() as u32).to_le_bytes());
        out.push(key_bytes.len() as u8);
    }

    out.extend_from_slice(key_bytes);
    out.extend_from_slice(&payload);
    Ok(out)
}

fn encode_payload(record: &BasonRecord) -> Result<Vec<u8>> {
    match record.r#type {
        BasonType::Array | BasonType::Object => {
            let mut payload = Vec::new();
            for child in &record.children {
                payload.extend_from_slice(&bason_encode(child)?);
            }
            Ok(payload)
        }
        _ => Ok(record.value.as_bytes().to_vec()),
    }
}

pub fn bason_decode(data: &[u8]) -> Result<(BasonRecord, usize)> {
    if data.is_empty() {
        return Err(BasonError::UnexpectedEof);
    }

    let (r#type, is_short) = decode_tag(data[0])?;
    let mut pos = 1usize;

    let (key_len, val_len) = if is_short {
        if data.len() < pos + 1 {
            return Err(BasonError::UnexpectedEof);
        }
        let lens = data[pos];
        pos += 1;
        (((lens >> 4) & 0x0F) as usize, (lens & 0x0F) as usize)
    } else {
        if data.len() < pos + 5 {
            return Err(BasonError::UnexpectedEof);
        }
        let val_len = u32::from_le_bytes(data[pos..pos + 4].try_into().unwrap()) as usize;
        pos += 4;
        let key_len = data[pos] as usize;
        pos += 1;
        (key_len, val_len)
    };

    if data.len() < pos + key_len + val_len {
        return Err(BasonError::UnexpectedEof);
    }

    let key = std::str::from_utf8(&data[pos..pos + key_len])
        .map_err(|_| BasonError::InvalidUtf8("key"))?
        .to_owned();
    pos += key_len;

    let value_slice = &data[pos..pos + val_len];
    pos += val_len;

    let record = match r#type {
        BasonType::Array | BasonType::Object => BasonRecord {
            r#type,
            key,
            value: String::new(),
            children: bason_decode_all(value_slice)?,
        },
        _ => BasonRecord {
            r#type,
            key,
            value: std::str::from_utf8(value_slice)
                .map_err(|_| BasonError::InvalidUtf8("value"))?
                .to_owned(),
            children: Vec::new(),
        },
    };

    Ok((record, pos))
}

pub fn bason_decode_all(data: &[u8]) -> Result<Vec<BasonRecord>> {
    let mut records = Vec::new();
    let mut pos = 0usize;
    while pos < data.len() {
        let (record, used) = bason_decode(&data[pos..])?;
        if used == 0 {
            return Err(BasonError::Malformed("decoder consumed zero bytes".into()));
        }
        records.push(record);
        pos += used;
    }
    Ok(records)
}

pub fn ron64_encode(mut value: u64) -> String {
    if value == 0 {
        return "0".to_string();
    }

    let alphabet = RON64_ALPHABET.as_bytes();
    let mut digits = Vec::new();
    while value > 0 {
        digits.push(alphabet[(value % 64) as usize] as char);
        value /= 64;
    }
    digits.reverse();
    digits.into_iter().collect()
}

pub fn ron64_decode(s: &str) -> Result<u64> {
    let mut value = 0u64;
    for ch in s.chars() {
        let digit = match ch {
            '0'..='9' => (ch as u32 - '0' as u32) as u64,
            'A'..='Z' => 10 + (ch as u32 - 'A' as u32) as u64,
            '_' => 36,
            'a'..='z' => 37 + (ch as u32 - 'a' as u32) as u64,
            '~' => 63,
            _ => return Err(BasonError::InvalidRon64Char(ch)),
        };
        value = value
            .checked_mul(64)
            .and_then(|v| v.checked_add(digit))
            .ok_or_else(|| BasonError::Malformed("RON64 overflow".into()))?;
    }
    Ok(value)
}

pub fn path_join<S: AsRef<str>>(segments: &[S]) -> String {
    segments
        .iter()
        .map(|s| s.as_ref())
        .filter(|s| !s.is_empty())
        .collect::<Vec<_>>()
        .join("/")
}

pub fn path_split(path: &str) -> Vec<String> {
    if path.is_empty() {
        Vec::new()
    } else {
        path.split('/').map(ToOwned::to_owned).collect()
    }
}

pub fn path_parent(path: &str) -> String {
    let mut parts = path_split(path);
    if parts.is_empty() {
        return String::new();
    }
    parts.pop();
    parts.join("/")
}

pub fn path_basename(path: &str) -> String {
    path_split(path).pop().unwrap_or_default()
}

pub fn bason_validate(record: &BasonRecord, strictness: u16) -> bool {
    validate_record(record, strictness).is_ok()
}

fn validate_record(record: &BasonRecord, strictness: u16) -> Result<()> {
    use strictness::*;

    if strictness & SHORTEST_ENCODING != 0 {
        let payload = encode_payload(record)?;
        if record.key.as_bytes().len() > u8::MAX as usize {
            return Err(BasonError::KeyTooLong(record.key.len()));
        }
        if payload.len() > u32::MAX as usize {
            return Err(BasonError::ValueTooLong(payload.len()));
        }
    }

    if strictness & VALID_UTF8 != 0 {
        // Keys and values are Rust Strings, so they are already valid UTF-8.
    }

    if strictness & CANONICAL_BOOLEAN_TEXT != 0 && record.r#type == BasonType::Boolean {
        match record.value.as_str() {
            "" | "true" | "false" => {}
            other => return Err(BasonError::InvalidBoolean(other.to_string())),
        }
    }

    if strictness & CANONICAL_NUMBER_FORMAT != 0 && record.r#type == BasonType::Number {
        validate_canonical_number(&record.value)?;
    }

    if strictness & CANONICAL_PATH_FORMAT != 0 && key_looks_like_path(&record.key) {
        validate_canonical_path(&record.key)?;
    }

    match record.r#type {
        BasonType::Array => validate_array(record, strictness)?,
        BasonType::Object => validate_object(record, strictness)?,
        _ => {}
    }

    if strictness & NO_FLAT_NESTED_MIXING != 0 {
        validate_no_flat_nested_mixing(record)?;
    }

    Ok(())
}

fn validate_array(record: &BasonRecord, strictness: u16) -> Result<()> {
    use strictness::*;

    let mut indices = Vec::with_capacity(record.children.len());
    for child in &record.children {
        let idx = ron64_decode(&child.key)?;
        if strictness & MINIMAL_RON64 != 0 && child.key != ron64_encode(idx) {
            return Err(BasonError::Malformed(format!(
                "array index is not minimal RON64: {}",
                child.key
            )));
        }
        indices.push(idx);
        validate_record(child, strictness)?;
    }

    if strictness & ORDERED_ARRAY_INDICES != 0 {
        for pair in indices.windows(2) {
            if pair[0] >= pair[1] {
                return Err(BasonError::Malformed(
                    "array indices are not strictly ascending".into(),
                ));
            }
        }
    }

    if strictness & CONTIGUOUS_ARRAY_INDICES != 0 {
        for (expected, actual) in indices.iter().enumerate() {
            if *actual != expected as u64 {
                return Err(BasonError::Malformed(
                    "array indices are not contiguous from 0".into(),
                ));
            }
        }
    }

    Ok(())
}

fn validate_object(record: &BasonRecord, strictness: u16) -> Result<()> {
    use strictness::*;

    let mut seen = HashSet::with_capacity(record.children.len());
    let mut prev: Option<&str> = None;

    for child in &record.children {
        if strictness & NO_DUPLICATE_KEYS != 0 && !seen.insert(child.key.clone()) {
            return Err(BasonError::Malformed(format!(
                "duplicate object key: {}",
                child.key
            )));
        }

        if strictness & SORTED_OBJECT_KEYS != 0 {
            if let Some(prev_key) = prev {
                if prev_key.as_bytes() > child.key.as_bytes() {
                    return Err(BasonError::Malformed(
                        "object keys are not sorted lexicographically".into(),
                    ));
                }
            }
            prev = Some(&child.key);
        }

        validate_record(child, strictness)?;
    }

    Ok(())
}

fn validate_no_flat_nested_mixing(record: &BasonRecord) -> Result<()> {
    if contains_container(record) && any_descendant_has_path_key(record, true) {
        return Err(BasonError::Malformed(
            "record tree mixes nested containers with flat path keys".into(),
        ));
    }
    Ok(())
}

fn contains_container(record: &BasonRecord) -> bool {
    if record.is_container() {
        return true;
    }
    record.children.iter().any(contains_container)
}

fn any_descendant_has_path_key(record: &BasonRecord, is_root: bool) -> bool {
    if !is_root && record.key.contains('/') {
        return true;
    }
    record
        .children
        .iter()
        .any(|child| any_descendant_has_path_key(child, false))
}

fn key_looks_like_path(key: &str) -> bool {
    key.contains('/') || key.is_empty()
}

fn validate_canonical_path(path: &str) -> Result<()> {
    if path.starts_with('/') || path.ends_with('/') || path.contains("//") {
        return Err(BasonError::Malformed(format!(
            "non-canonical path: {path}"
        )));
    }
    Ok(())
}

fn validate_canonical_number(text: &str) -> Result<()> {
    if text.is_empty() {
        return Err(BasonError::InvalidNumber(text.to_string()));
    }
    if text.starts_with('+') || text.contains('e') || text.contains('E') || text.ends_with('.') {
        return Err(BasonError::InvalidNumber(text.to_string()));
    }

    let bytes = text.as_bytes();
    let mut start = 0usize;
    if bytes[0] == b'-' {
        start = 1;
        if text.len() == 1 {
            return Err(BasonError::InvalidNumber(text.to_string()));
        }
    }

    let body = &text[start..];
    if let Some((int_part, frac_part)) = body.split_once('.') {
        if int_part.is_empty() || frac_part.is_empty() {
            return Err(BasonError::InvalidNumber(text.to_string()));
        }
        if int_part.len() > 1 && int_part.starts_with('0') {
            return Err(BasonError::InvalidNumber(text.to_string()));
        }
        if !int_part.chars().all(|c| c.is_ascii_digit()) || !frac_part.chars().all(|c| c.is_ascii_digit()) {
            return Err(BasonError::InvalidNumber(text.to_string()));
        }
    } else {
        if body.len() > 1 && body.starts_with('0') {
            return Err(BasonError::InvalidNumber(text.to_string()));
        }
        if !body.chars().all(|c| c.is_ascii_digit()) {
            return Err(BasonError::InvalidNumber(text.to_string()));
        }
    }

    Ok(())
}

pub fn json_to_bason(json: &str) -> Result<Vec<u8>> {
    let value: Value = serde_json::from_str(json).map_err(|e| BasonError::Json(e.to_string()))?;
    let root = json_value_to_record("", &value)?;
    bason_encode(&root)
}

pub fn bason_to_json(data: &[u8]) -> Result<String> {
    let records = bason_decode_all(data)?;
    let root = if records.len() == 1
        && (records[0].is_container() || records[0].key.is_empty())
    {
        records.into_iter().next().unwrap()
    } else {
        bason_unflatten(&records)?
    };
    let value = record_to_json_value(&root)?;
    serde_json::to_string_pretty(&value).map_err(|e| BasonError::Json(e.to_string()))
}

pub fn json_value_to_record(key: &str, value: &Value) -> Result<BasonRecord> {
    let record = match value {
        Value::Null => BasonRecord::boolean(key, ""),
        Value::Bool(v) => BasonRecord::boolean(key, if *v { "true" } else { "false" }),
        Value::Number(n) => BasonRecord::number(key, n.to_string()),
        Value::String(s) => BasonRecord::string(key, s.clone()),
        Value::Array(items) => {
            let mut children = Vec::with_capacity(items.len());
            for (idx, item) in items.iter().enumerate() {
                children.push(json_value_to_record(&ron64_encode(idx as u64), item)?);
            }
            BasonRecord::array(key, children)
        }
        Value::Object(map) => {
            let mut children = Vec::with_capacity(map.len());
            for (k, v) in map {
                children.push(json_value_to_record(k, v)?);
            }
            BasonRecord::object(key, children)
        }
    };
    Ok(record)
}

pub fn record_to_json_value(record: &BasonRecord) -> Result<Value> {
    match record.r#type {
        BasonType::Boolean => match record.value.as_str() {
            "" => Ok(Value::Null),
            "true" => Ok(Value::Bool(true)),
            "false" => Ok(Value::Bool(false)),
            other => Err(BasonError::InvalidBoolean(other.to_string())),
        },
        BasonType::String => Ok(Value::String(record.value.clone())),
        BasonType::Number => serde_json::from_str::<Value>(&record.value)
            .map_err(|_| BasonError::InvalidNumber(record.value.clone()))
            .and_then(|v| match v {
                Value::Number(_) => Ok(v),
                _ => Err(BasonError::InvalidNumber(record.value.clone())),
            }),
        BasonType::Array => {
            let mut indexed = Vec::with_capacity(record.children.len());
            for child in &record.children {
                indexed.push((ron64_decode(&child.key)?, record_to_json_value(child)?));
            }
            indexed.sort_by_key(|(idx, _)| *idx);
            let values = indexed.into_iter().map(|(_, v)| v).collect();
            Ok(Value::Array(values))
        }
        BasonType::Object => {
            let mut map = Map::new();
            for child in &record.children {
                map.insert(child.key.clone(), record_to_json_value(child)?);
            }
            Ok(Value::Object(map))
        }
    }
}

pub fn bason_flatten(root: &BasonRecord) -> Vec<BasonRecord> {
    let mut out = Vec::new();
    flatten_into(root, "", &mut out);
    out
}

fn flatten_into(record: &BasonRecord, prefix: &str, out: &mut Vec<BasonRecord>) {
    let current = if prefix.is_empty() {
        record.key.clone()
    } else if record.key.is_empty() {
        prefix.to_string()
    } else {
        path_join(&[prefix, &record.key])
    };

    match record.r#type {
        BasonType::Array | BasonType::Object => {
            for child in &record.children {
                flatten_into(child, &current, out);
            }
        }
        _ => out.push(BasonRecord {
            r#type: record.r#type,
            key: current,
            value: record.value.clone(),
            children: Vec::new(),
        }),
    }
}

pub fn bason_unflatten(records: &[BasonRecord]) -> Result<BasonRecord> {
    #[derive(Debug, Clone)]
    enum Node {
        Branch(BTreeMap<String, Node>),
        Leaf(BasonRecord),
    }

    fn insert(node: &mut Node, parts: &[String], leaf: &BasonRecord) -> Result<()> {
        if parts.is_empty() {
            *node = Node::Leaf(BasonRecord {
                r#type: leaf.r#type,
                key: String::new(),
                value: leaf.value.clone(),
                children: Vec::new(),
            });
            return Ok(());
        }

        let map = match node {
            Node::Branch(map) => map,
            Node::Leaf(_) => {
                return Err(BasonError::Malformed(
                    "conflicting flat paths while rebuilding nested tree".into(),
                ))
            }
        };

        let child = map
            .entry(parts[0].clone())
            .or_insert_with(|| Node::Branch(BTreeMap::new()));
        insert(child, &parts[1..], leaf)
    }

    fn materialize(key: String, node: Node) -> Result<BasonRecord> {
        match node {
            Node::Leaf(mut leaf) => {
                leaf.key = key;
                Ok(leaf)
            }
            Node::Branch(map) => {
                let mut pairs: Vec<(String, Node)> = map.into_iter().collect();
                let is_array = looks_like_array(&pairs)?;

                if is_array {
                    pairs.sort_by_key(|(segment, _)| ron64_decode(segment).unwrap_or(u64::MAX));
                    let mut children = Vec::with_capacity(pairs.len());
                    for (segment, child) in pairs {
                        let child_record = materialize(segment, child)?;
                        children.push(child_record);
                    }
                    Ok(BasonRecord::array(key, children))
                } else {
                    let mut children = Vec::with_capacity(pairs.len());
                    for (segment, child) in pairs {
                        children.push(materialize(segment, child)?);
                    }
                    Ok(BasonRecord::object(key, children))
                }
            }
        }
    }

    fn looks_like_array(pairs: &[(String, Node)]) -> Result<bool> {
        if pairs.is_empty() {
            return Ok(false);
        }
        let mut indices = Vec::with_capacity(pairs.len());
        for (segment, _) in pairs {
            let idx = match ron64_decode(segment) {
                Ok(v) => v,
                Err(_) => return Ok(false),
            };
            if ron64_encode(idx) != *segment {
                return Ok(false);
            }
            indices.push(idx);
        }
        indices.sort_unstable();
        for (expected, actual) in indices.iter().enumerate() {
            if *actual != expected as u64 {
                return Ok(false);
            }
        }
        Ok(true)
    }

    if records.is_empty() {
        return Ok(BasonRecord::object("", Vec::new()));
    }

    if records.len() == 1 && records[0].key.is_empty() {
        return Ok(records[0].clone());
    }

    let mut root = Node::Branch(BTreeMap::new());
    for record in records {
        insert(&mut root, &path_split(&record.key), record)?;
    }
    materialize(String::new(), root)
}

pub fn pretty_print_record(record: &BasonRecord) -> String {
    let mut out = String::new();
    pretty_print_record_into(record, 0, &mut out);
    out
}

fn pretty_print_record_into(record: &BasonRecord, depth: usize, out: &mut String) {
    let indent = "  ".repeat(depth);
    match record.r#type {
        BasonType::Array | BasonType::Object => {
            out.push_str(&format!(
                "{indent}{:?} key={:?} children={}\n",
                record.r#type,
                record.key,
                record.children.len()
            ));
            for child in &record.children {
                pretty_print_record_into(child, depth + 1, out);
            }
        }
        _ => out.push_str(&format!(
            "{indent}{:?} key={:?} value={:?}\n",
            record.r#type, record.key, record.value
        )),
    }
}

pub fn hexdump_annotated(bytes: &[u8]) -> Result<String> {
    let mut out = String::new();
    let mut pos = 0usize;

    while pos < bytes.len() {
        let start = pos;
        let tag = bytes[pos];
        let (_, is_short) = decode_tag(tag)?;
        pos += 1;

        let (key_len, val_len, header_len_desc) = if is_short {
            if pos >= bytes.len() {
                return Err(BasonError::UnexpectedEof);
            }
            let lens = bytes[pos];
            pos += 1;
            (
                ((lens >> 4) & 0x0F) as usize,
                (lens & 0x0F) as usize,
                format!("short lengths=0x{lens:02x}"),
            )
        } else {
            if pos + 5 > bytes.len() {
                return Err(BasonError::UnexpectedEof);
            }
            let val_len = u32::from_le_bytes(bytes[pos..pos + 4].try_into().unwrap()) as usize;
            let key_len = bytes[pos + 4] as usize;
            pos += 5;
            (
                key_len,
                val_len,
                format!("long val_len={val_len} key_len={key_len}"),
            )
        };

        if pos + key_len + val_len > bytes.len() {
            return Err(BasonError::UnexpectedEof);
        }
        let key = String::from_utf8_lossy(&bytes[pos..pos + key_len]).to_string();
        pos += key_len;
        let value = &bytes[pos..pos + val_len];
        pos += val_len;

        out.push_str(&format!("@{start:08x} tag={} {header_len_desc} key={key:?}\n", tag as char));
        out.push_str(&format!("           value(hex)={}\n", to_hex(value)));
    }

    Ok(out)
}

fn to_hex(bytes: &[u8]) -> String {
    let mut out = String::with_capacity(bytes.len() * 2);
    for b in bytes {
        out.push_str(&format!("{b:02x}"));
    }
    out
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn ron64_examples_match_spec() {
        let cases = [
            (0, "0"),
            (9, "9"),
            (10, "A"),
            (36, "_"),
            (63, "~"),
            (64, "10"),
            (100, "1W"),
            (4095, "~~"),
            (4096, "100"),
        ];
        for (n, s) in cases {
            assert_eq!(ron64_encode(n), s);
            assert_eq!(ron64_decode(s).unwrap(), n);
        }
    }

    #[test]
    fn leaf_round_trip() {
        let record = BasonRecord::string("name", "Alice");
        let bytes = bason_encode(&record).unwrap();
        let (decoded, used) = bason_decode(&bytes).unwrap();
        assert_eq!(used, bytes.len());
        assert_eq!(decoded, record);
    }

    #[test]
    fn nested_round_trip() {
        let root = BasonRecord::object(
            "",
            vec![
                BasonRecord::string("name", "Alice"),
                BasonRecord::array(
                    "scores",
                    vec![BasonRecord::number("0", "95"), BasonRecord::number("1", "87")],
                ),
            ],
        );
        let bytes = bason_encode(&root).unwrap();
        let (decoded, used) = bason_decode(&bytes).unwrap();
        assert_eq!(used, bytes.len());
        assert_eq!(decoded, root);
    }

    #[test]
    fn json_round_trip_preserves_semantics() {
        let json = r#"{
          "name": "Alice",
          "active": true,
          "score": 95,
          "ratio": 12.5,
          "nested": {"x": null},
          "items": [1, 2, 3]
        }"#;
        let bason = json_to_bason(json).unwrap();
        let json_back = bason_to_json(&bason).unwrap();
        let a: Value = serde_json::from_str(json).unwrap();
        let b: Value = serde_json::from_str(&json_back).unwrap();
        assert_eq!(a, b);
    }

    #[test]
    fn flatten_then_unflatten() {
        let root = BasonRecord::object(
            "",
            vec![
                BasonRecord::string("name", "Alice"),
                BasonRecord::array(
                    "scores",
                    vec![BasonRecord::number("0", "95"), BasonRecord::number("1", "87")],
                ),
            ],
        );
        let flat = bason_flatten(&root);
        let nested = bason_unflatten(&flat).unwrap();
        assert_eq!(nested, root);
    }

    #[test]
    fn strict_validator_catches_bad_number() {
        let record = BasonRecord::number("x", "01");
        assert!(!bason_validate(
            &record,
            strictness::CANONICAL_NUMBER_FORMAT
        ));
    }
}
