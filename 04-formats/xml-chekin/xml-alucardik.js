const fs = require('fs');
const xml2js = require('xml2js')

function abort(msg) {
    console.warn(msg)
    process.exit(1)
}

function trimNulls(s) {
    let trimIdx = s.length - 1
    if (trimIdx < 0) {
        return s
    }

    for (; trimIdx > 0; --trimIdx) {
        if (s[trimIdx] !== '\x00') {
            break
        }
    }

    return s.substring(0, trimIdx + 1)
}

if (process.argv.length !== 3) {
    abort("usage: node xml-alucardik.js [filePath]")
}


const MODE_UNDEFINED = 'undefined'
const MODE_XML = 'xml'
const MODE_BIN = 'bin'

const TYPE_STRING = 'string'
const TYPE_UINT8 = 'uint8'
const TYPE_FLOAT32 = 'float32'
const TYPE_STRUCT = 'struct'
const TYPE_TRAIT_ARRAY = '[]'

function getLiteralSizeByType(type) {
    switch (type) {
        case TYPE_FLOAT32:
            return 4
        case TYPE_UINT8:
            return 1
        default:
            return -1
    }
}

const STRUCT_BINARY_SIZE = 128
const STRUCT_XML_SIZE = 494

const STRUCT_NESTED_META = {
    repo: { type: TYPE_STRING, byteSize: 59, encoding: 'utf-8' },
    mark: { type: TYPE_UINT8, byteSize: 1, encoding: 'binary' },
}

const STRUCT_META = {
    name: { type: TYPE_STRING, byteSize: 32, encoding: 'utf-8' },
    login: { type: TYPE_STRING, byteSize: 16, encoding: 'ascii' },
    group: { type: TYPE_STRING,  byteSize: 8, encoding: 'utf-8' },
    practice: { type: TYPE_UINT8+TYPE_TRAIT_ARRAY,  byteSize: 8, encoding: 'binary' },
    project: { type: TYPE_STRUCT,  byteSize: 60, encoding: STRUCT_NESTED_META },
    mark: { type: TYPE_FLOAT32,  byteSize: 4, encoding: 'binary' },
}

function parseRawLiteral(type, raw, encoding) {
    switch (type) {
        case TYPE_STRING:
            return trimNulls(raw.toString(encoding))
        case TYPE_UINT8:
            return raw.readUInt8()
        case TYPE_FLOAT32:
            return raw.readFloatLE()
        default:
            throw Error('unsupported raw field type')
    }
}

function parseRawArray(type, raw, byteSize, encoding) {
    const arrayLength = byteSize / getLiteralSizeByType(type)
    const res = []
    for (let i = 0; i < arrayLength; ++i) {
        try {
            res.push(parseRawLiteral(type, raw.subarray(i, i+1), encoding))
        } catch (e) {
            throw e
        }
    }

    return res
}

function parseRawField(type, raw, byteSize, encoding = 'binary') {
    if (type.endsWith(TYPE_TRAIT_ARRAY)) {
        return parseRawArray(type.substring(0, type.length - TYPE_TRAIT_ARRAY.length), raw, byteSize, encoding)
    } else if (type === TYPE_STRUCT) {
        return objectFromStructMeta(STRUCT_NESTED_META, raw.toString('binary'))
    }

    return parseRawLiteral(type, raw, encoding)
}

function objectFromStructMeta(meta, rawStruct) {
    if (typeof meta !== 'object') {
        throw Error('meta must be an object')
    }

    if (typeof rawStruct !== 'string') {
        throw Error('rawStruct must be passed as string')
    }

    let res = {}
    let currentOffset = 0

    Object.entries(meta).forEach(([fieldName, { type, byteSize, encoding }]) => {
        res[fieldName] = parseRawField(type, Buffer.from(rawStruct.slice(currentOffset, currentOffset+byteSize), 'binary'), byteSize, encoding)
        currentOffset += byteSize
    })

    return res
}

function structFromJSObject(meta, rawObject) {
    if (typeof meta !== 'object' && typeof rawObject !== 'object') {
        throw Error('meta and rawObject must be objects')
    }

    const bufs = []
    Object.entries(meta).forEach(([fieldName, { type, byteSize, encoding }]) => {
        const fieldValue = rawObject?.[fieldName]

        if (fieldValue === undefined) {
            throw Error(`struct meta doesn't correspond to js-object: no field ${fieldName} in js-object`)
        }

        let buf
        if (type.endsWith(TYPE_TRAIT_ARRAY)) {
            buf = Buffer.from(fieldValue, encoding)
        } else if (type === TYPE_STRUCT) {
            buf = structFromJSObject(STRUCT_NESTED_META, fieldValue[0])
        } else {
            if (type === TYPE_FLOAT32) {
                buf = Buffer.alloc(4)
                buf.writeFloatLE(parseFloat(fieldValue[0]))
            } else if (type === TYPE_UINT8) {
                buf = Buffer.alloc(1)
                buf.writeUInt8(parseInt(fieldValue[0]))
            } else {
                buf = Buffer.from(fieldValue[0], encoding)
            }
        }
        if (buf.length < byteSize) {
            const padding = Buffer.alloc(byteSize - buf.length, '\x00')
            buf = Buffer.concat([buf, padding])
        } else if (buf.length > byteSize) {
            buf = buf.subarray(0, byteSize)
        }
        bufs.push(buf)
    })

    return Buffer.concat(bufs)
}

function serialiseToXML(fileContents) {
    const xmlBuilder = new xml2js.Builder()
    let allContents = ''

    for (let currentOffset = 0; currentOffset < fileContents.length; currentOffset += STRUCT_BINARY_SIZE) {
        const jsObject = objectFromStructMeta(STRUCT_META, fileContents.slice(currentOffset, currentOffset+STRUCT_BINARY_SIZE))
        allContents += xmlBuilder.buildObject(jsObject)
    }

    return allContents
}

async function serialiseToBIN(fileContents) {
    const xmlParser = new xml2js.Parser()
    let allContents = ''

    for (let currentOffset = 0; currentOffset < fileContents.length; currentOffset += STRUCT_XML_SIZE) {
        const { root: jsObject } = await xmlParser.parseStringPromise(fileContents.slice(currentOffset, currentOffset+STRUCT_XML_SIZE))
        allContents += structFromJSObject(STRUCT_META, jsObject).toString('binary')
    }

    return allContents
}

async function run() {
    const filePath = process.argv[2]
    let serialisationMode = MODE_UNDEFINED
    if (filePath.endsWith('.xml')) {
        serialisationMode = MODE_XML
    } else if (filePath.endsWith('.bin')) {
        serialisationMode = MODE_BIN
    }

    if (serialisationMode === MODE_UNDEFINED) {
        abort('Unknown file extension')
    }

    const fileExtension = serialisationMode === MODE_BIN ? '.xml' : '.bin'
    const fileEncoding = serialisationMode === MODE_BIN ? 'utf-8' : 'binary'

    try {
        const contents = fs.readFileSync(filePath, {
            encoding: serialisationMode === MODE_BIN ? 'binary' : 'utf-8',
        })

        const serialised = serialisationMode === MODE_BIN ? serialiseToXML(contents) : await serialiseToBIN(contents)
        fs.writeFileSync(`${filePath}-chekin-serialised${fileExtension}`, serialised, {
            encoding: fileEncoding
        })
    } catch (e) {
        abort(e.toString())
    }
}

run().then()
