const std = @import("std");
const sha2 = std.crypto.hash.sha2;
const Allocator = std.mem.Allocator;

const GB = 1 << 30;
const HASH_SIZE = 32;
const HEX_SIZE = 64;

const ErrorIncorrectTree = error{
    IncorrectHash,
};

pub fn concatenateDirCommit(allocator: Allocator, root: []const u8, commit: []const u8) ![]const u8 {
    const new_len = root.len + commit.len + 1;

    var result = try allocator.alloc(u8, new_len);

    std.mem.copy(u8, result[0..root.len], root);
    std.mem.copy(u8, result[root.len .. root.len + 1], "/");
    std.mem.copy(u8, result[root.len + 1 .. new_len], commit);

    return result;
}

fn checkBufHash(buf: []const u8, expected: []const u8) !void {
    var hash: [HASH_SIZE]u8 = undefined;
    sha2.Sha256.hash(buf, &hash, .{});

    var slice_hash = std.mem.bytesAsSlice(u8, &hash);

    std.debug.print("hex of file hash:{}\nfile name: {s}\n", .{ std.fmt.fmtSliceHexLower(slice_hash), expected });

    var hex_bytes: [HEX_SIZE]u8 = undefined;
    _ = try std.fmt.bufPrint(&hex_bytes, "{}", .{std.fmt.fmtSliceHexLower(slice_hash)});
    var hex_slice = std.mem.bytesAsSlice(u8, &hex_bytes);

    for (hex_slice, expected) |value1, value2| {
        if (value1 != value2) {
            return ErrorIncorrectTree.IncorrectHash;
        }
    }
}

fn readAndCheckCommit(allocator: Allocator, dir: []const u8, name: []const u8) ![]const u8 {
    // read current commit
    var commit_path = try concatenateDirCommit(allocator, dir, name);
    std.debug.print("commit path: {s}\n", .{commit_path});
    var file = try std.fs.cwd().openFile(commit_path, .{});
    const file_buffer = try file.readToEndAlloc(allocator, GB);
    file.close();

    // check if hash of commit != name
    try checkBufHash(file_buffer, name);

    return file_buffer;
}

fn checkFile(allocator: Allocator, root: []const u8, name: []const u8) !void {
    var file_buffer = try readAndCheckCommit(allocator, root, name);
    defer allocator.free(file_buffer);
}

fn checkDir(allocator: Allocator, root: []const u8, name: []const u8) !void {
    var file_buffer = try readAndCheckCommit(allocator, root, name);
    defer allocator.free(file_buffer);

    // split commit by '\n'
    var lines = std.mem.split(u8, file_buffer, "\n");
    while (lines.next()) |line| {
        // split line by '/\t'
        var it = std.mem.split(u8, line, "/\t");
        _ = it.next(); // skip name

        // file (len(it) == 1)
        if (it.peek() == null) {
            var file_it = std.mem.split(u8, line, ":\t");
            _ = file_it.next(); // skip name of file
            try checkFile(allocator, root, file_it.next().?);

            continue;
        }

        // dir
        try checkDir(allocator, root, it.next().?);
    }
}

pub fn main() !void {
    const allocator = std.heap.page_allocator;

    var args = try std.process.argsAlloc(allocator);
    defer std.process.argsFree(allocator, args);
    if (args.len != 3) {
        std.debug.print("Usage: ./check <root_path> <root_hash>\n", .{});
        return;
    }

    const root = args[1];
    const root_hash = args[2];

    try checkDir(allocator, root, root_hash);

    std.debug.print("All is correct!\n", .{});
}
