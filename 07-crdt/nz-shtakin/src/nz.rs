use std::collections::BTreeMap;
use std::io::{self, Error};
use std::cmp::max;

use crate::zipint;

#[derive(Clone, Debug, Default)]
pub struct N {
    pub records: BTreeMap<u64, u64>
}

impl N {
    pub fn new() -> Self {
        Default::default()
    }

    pub fn tlv(&self) -> Vec<u8> {
        let mut v = Vec::<u8>::new();
        for (src, val) in &self.records {
            let p = zipint::zip_uint64_pair(*val,* src);
            v.push(b'u');
            v.push(p.len() as u8);
            v.extend(p);
        }
        let mut result = Vec::<u8>::new();
        result.push(b'n');
        result.push(v.len() as u8);
        result.extend(v);
        return result;
    }

    pub fn merge(&mut self, other: &N) {
        for (src, val) in &other.records {
            self.records.insert(*src, if !self.records.contains_key(src) { *val } else { max(*val, self.records[src]) });
        }
    }

    pub fn to_string(&mut self) -> String {
        let mut res = String::new();
        for (src, val) in &self.records {
            if !res.is_empty() {
                res += &"\n";
            }
            res += &format!("{} {}", src, val);
        }
        return res;
    }

    pub fn from_string(s: &String) -> Result<N, Error> {
        let mut n = N::new();
        let lines: Vec<&str> = s.split('\n').collect();
        for line in lines {
            let pair: Vec<&str> = line.split(' ').collect();
            if pair.len() != 2 {
                return Err(io::Error::new(io::ErrorKind::InvalidData,format!("invalid src val pair")));
            }
            let src = pair[0].parse::<u64>().unwrap();
            let val = pair[1].parse::<u64>().unwrap();
            if n.records.contains_key(&src) { return Err(io::Error::new(io::ErrorKind::InvalidData,format!("src is presented twice"))); }
            n.records.insert(src, val);
        }
        return Ok(n);
    }
}

pub fn Nparse_impl(tlv: &Vec<u8>) -> Result<(N, usize), Error> {
    if tlv.len() < 2 || tlv[0] != b'n' { return Err(io::Error::new(io::ErrorKind::InvalidData,format!("invalid tlv type"))); }
    let size = tlv[1] as usize;
    if tlv.len() < 2 + size { return Err(io::Error::new(io::ErrorKind::InvalidData,format!("invalid tlv size"))); }
    let mut n = N::new();

    let mut i: usize = 2;
    while i < 2 + size {
        if tlv[i] != b'u' { return Err(io::Error::new(io::ErrorKind::InvalidData,format!("invalid pair type"))); }
        i += 1;

        let uint64_pair_size = tlv[i] as usize;
        if i + uint64_pair_size >= tlv.len() { return Err(io::Error::new(io::ErrorKind::InvalidData,format!("invalid uint64_pair_size"))); }
        i += 1;

        let (val, src) = zipint::unzip_uint64_pair(&tlv[i..i + uint64_pair_size])?;
        if n.records.contains_key(&src) { return Err(io::Error::new(io::ErrorKind::InvalidData,format!("src is presented twice"))); }
        n.records.insert(src, val);

        i += uint64_pair_size;
    }
    return Ok((n, i));
}

pub fn Nstring(tlv: &Vec<u8>) -> Result<String, Error> {
    let (mut n, _) = Nparse_impl(tlv)?;
    return Ok(n.to_string());
}

pub fn Nparse(tlv_string: &String) -> Result<Vec<u8>, Error> {
    let n = N::from_string(tlv_string)?;
    return Ok(n.tlv());
}

pub fn Nnative(tlv: &Vec<u8>) -> Result<u64, Error> {
    let (n, i) = Nparse_impl(tlv)?;
    if i != tlv.len() { return Err(io::Error::new(io::ErrorKind::InvalidData,format!("extra bytes in tlv record"))); };
    let mut c : u64 = 0;
    for (_, val) in &n.records {
        c += val;
    }
    return Ok(c);
}

pub fn Nmerge(tlvs: &Vec<Vec<u8>>) -> Result<Vec<u8>, Error> {
    let mut res = N::new();
    for tlv in tlvs {
        let (n, _) = Nparse_impl(tlv)?;
        res.merge(&n);
    }
    return Ok(res.tlv());
}
