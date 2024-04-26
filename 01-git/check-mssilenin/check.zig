const std = @import("std");
const sha2 = std.crypto.hash.sha2;
const Allocator = std.mem.Allocator;

const GB = 1 << 30;
const HASH_SIZE = 32;
const HEX_SIZE = 64;

const ErrorIncorrectTree = error{
    IncorrectHash,
};

// pub fn concatenateDirCommit(allocator: Allocator, root: []const u8, commit: []const u8) ![]const u8 {
//     const new_len = root.len + commit.len + 1;

//     var result = try allocator.alloc(u8, new_len);

//     std.mem.copy(u8, result[0..root.len], root);
//     std.mem.copy(u8, result[root.len .. root.len + 1], "/");
//     std.mem.copy(u8, result[root.len + 1 .. new_len], commit);

//     return result;
// }

fn checkBufHash(buf: []const u8, expected: []const u8) !void {
    var hash: [HASH_SIZE]u8 = undefined;
    sha2.Sha256.hash(buf, &hash, .{});

    var slice_hash = std.mem.bytesAsSlice(u8, &hash);

    //std.debug.print("hex of file hash:{}\nfile name: {s}\n", .{ std.fmt.fmtSliceHexLower(slice_hash), expected });

    var hex_bytes: [HEX_SIZE]u8 = undefined;
    _ = try std.fmt.bufPrint(&hex_bytes, "{}", .{std.fmt.fmtSliceHexLower(slice_hash)});
    var hex_slice = std.mem.bytesAsSlice(u8, &hex_bytes);

    for (hex_slice, expected) |value1, value2| {
        if (value1 != value2) {
            return ErrorIncorrectTree.IncorrectHash;
        }
    }
}

fn readCommit(allocator: Allocator, name: []const u8) ![]const u8 {
    var file = try std.fs.cwd().openFile(name, .{});
    const file_buffer = try file.readToEndAlloc(allocator, GB);
    file.close();

    return file_buffer;
}

fn readAndCheckCommit(allocator: Allocator, hash: []const u8) ![]const u8 {
    // read current commit
    std.debug.print("checking commit hash; commit_hash={s}\n", .{hash});
    var file_buffer = try readCommit(allocator, hash);

    // check if hash of commit != name
    try checkBufHash(file_buffer, hash);

    return file_buffer;
}

fn findDir(allocator: Allocator, root_dir: []const u8, commit_hash: []const u8) ![]const u8 {
    var current_commit_hash = try allocator.alloc(u8, commit_hash.len);
    std.mem.copy(u8, current_commit_hash, commit_hash);

    if (std.mem.eql(u8, root_dir, ".")) {
        return current_commit_hash;
    }

    var dirs_it = std.mem.split(u8, root_dir, "/");

    while (dirs_it.next()) |dir| {
        var file_buffer = try readCommit(allocator, current_commit_hash);
        defer allocator.free(file_buffer);

        // split commit by '\n'
        var lines = std.mem.split(u8, file_buffer, "\n");
        while (lines.next()) |line| {
            // skip empty line
            if (line.len == 0) {
                continue;
            }

            // split line by '/\t'
            var it = std.mem.split(u8, line, "/\t");
            var commit_dir = it.next();

            // file -> skip
            if (it.peek() == null) {
                continue;
            }

            // find commit_dir == dir
            if (std.mem.eql(u8, dir, commit_dir.?)) {
                allocator.free(current_commit_hash);

                var hash = it.peek().?;
                current_commit_hash = try allocator.alloc(u8, hash.len);
                std.mem.copy(u8, current_commit_hash, hash);
                break;
            }
        }
    }

    std.debug.print("found dir={s}\n", .{current_commit_hash});

    return current_commit_hash;
}

fn checkFile(allocator: Allocator, name: []const u8) !void {
    var file_buffer = try readAndCheckCommit(allocator, name);
    defer allocator.free(file_buffer);
}

fn checkDir(allocator: Allocator, name: []const u8) !void {
    var file_buffer = try readAndCheckCommit(allocator, name);
    defer allocator.free(file_buffer);

    // split commit by '\n'
    var lines = std.mem.split(u8, file_buffer, "\n");
    while (lines.next()) |line| {
        // skip empty line
        if (line.len == 0) {
            continue;
        }

        // split line by '/\t'
        var it = std.mem.split(u8, line, "/\t");
        _ = it.next(); // skip name of dir

        // file (len(it) == 1)
        if (it.peek() == null) {
            var file_it = std.mem.split(u8, line, ":\t");
            _ = file_it.next(); // skip name of file

            try checkFile(allocator, file_it.next().?);

            continue;
        }

        // dir
        try checkDir(allocator, it.next().?);
    }
}

pub fn main() !void {
    const allocator = std.heap.page_allocator;

    var args = try std.process.argsAlloc(allocator);
    defer std.process.argsFree(allocator, args);
    if (args.len != 3) {
        std.debug.print("Usage: ./check <dir> <start_hash>\n", .{});
        return;
    }

    const dir = args[1];
    const start_hash = args[2];

    var hash = try findDir(allocator, dir, start_hash);
    defer allocator.free(hash);

    try checkDir(allocator, hash);

    std.debug.print("All is correct!\n", .{});
}
