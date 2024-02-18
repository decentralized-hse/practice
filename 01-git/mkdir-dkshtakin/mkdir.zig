const std = @import("std");
const fs = std.fs;
const assert = std.debug.assert;
const Sha256 = std.crypto.hash.sha2.Sha256;

const BUF_SIZE = 4096;
const EMPTY_HASH = "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855";
const HASH_SIZE = 64;

fn sha256fromFile(path: []const u8, output: *[HASH_SIZE]u8) !void {
    var sha256 = Sha256.init(.{});
    const file = try fs.cwd().openFile(path, .{});
    defer file.close();
    const rdr = file.reader();

    var buf: [BUF_SIZE]u8 = undefined;
    var n = try rdr.read(&buf);
    while (n != 0) {
        sha256.update(buf[0..n]);
        n = try rdr.read(&buf);
    }

    _ = try std.fmt.bufPrint(output, "{s}", .{std.fmt.fmtSliceHexLower(&sha256.finalResult())});
}

fn findHash(path: []const u8, dir_name: []const u8, output: *[HASH_SIZE]u8) !void {
    var dir = try std.fs.cwd().openFile(path, .{});
    defer dir.close();

    var buf_reader = std.io.bufferedReader(dir.reader());
    var in_stream = buf_reader.reader();
    var buf: [1024]u8 = undefined;
    while (try in_stream.readUntilDelimiterOrEof(&buf, '\n')) |line| {
        if (std.mem.indexOfScalar(u8, line, '/')) |j| {
            if (std.mem.eql(u8, line[0..j], dir_name)) {
                const k = std.mem.indexOfScalar(u8, line, '\t').?;
                _ = try std.fmt.bufPrint(output, "{s}", .{line[k + 1 ..]});
                break;
            }
        }
    }
}

fn writeEntry(file: *std.fs.File, entry: *const []const u8) !void {
    _ = try file.write(entry.*);
    _ = try file.write("\n");
}

fn mkdirImpl(path: []const u8, parent_hash: []const u8, updated_parent_hash: *[HASH_SIZE]u8) !void {
    // setup allocator
    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    defer _ = gpa.deinit();
    const allocator = gpa.allocator();

    var dir_name: []const u8 = "";
    var dir_hash: [HASH_SIZE]u8 = undefined;
    var updated_dir_hash: [HASH_SIZE]u8 = undefined;
    var add = false;

    const index = std.mem.indexOfScalar(u8, path, '/');
    if (index) |i| {
        dir_name = path[0..i];

        // get current directory's hash
        try findHash(parent_hash, dir_name, &dir_hash);

        // get new hash recursively
        try mkdirImpl(path[i + 1 ..], &dir_hash, &updated_dir_hash);
    } else {
        add = true;
        dir_name = path;
        _ = try std.fmt.bufPrint(&updated_dir_hash, EMPTY_HASH, .{});

        // updated parent file
        var empty_dir = try std.fs.cwd().createFile(&updated_dir_hash, .{});
        defer empty_dir.close();
    }

    // create new parent file with updated/added hash
    const tmp_parent_name = parent_hash[0..4];
    {
        // previous parent file
        var parent = try std.fs.cwd().openFile(parent_hash, .{});
        defer parent.close();

        // updated parent file
        var updated_parent = try std.fs.cwd().createFile(tmp_parent_name, .{});
        defer updated_parent.close();

        var buf_reader = std.io.bufferedReader(parent.reader());
        const reader = buf_reader.reader();
        var line = std.ArrayList(u8).init(allocator);
        defer line.deinit();
        const writer = line.writer();

        const updated_line: []const u8 = try std.fmt.allocPrint(allocator, "{s}/\t{s}", .{ dir_name, updated_dir_hash });
        defer allocator.free(updated_line);

        var updated = false;
        var eof = false;
        while (!eof) {
            reader.streamUntilDelimiter(writer, '\n', null) catch |err| switch (err) {
                error.EndOfStream => eof = true,
                else => return err,
            };
            defer line.clearRetainingCapacity();
            if (line.items.len == 0) {
                break;
            }

            if (updated) {} else if ((std.mem.indexOfScalar(u8, line.items, '/'))) |j| {
                if ((add and std.mem.lessThan(u8, dir_name, line.items[0..j])) or std.mem.eql(u8, line.items[0..j], dir_name)) {
                    updated = true;
                    try writeEntry(&updated_parent, &updated_line);
                    if (!add) {
                        continue;
                    }
                }
            }
            try writeEntry(&updated_parent, &line.items);
        }
        if (!updated) {
            try writeEntry(&updated_parent, &updated_line);
        }
    }

    // calculate updated parent hash
    try sha256fromFile(tmp_parent_name, updated_parent_hash);

    try fs.cwd().rename(
        tmp_parent_name,
        updated_parent_hash,
    );
}

pub fn main() !void {
    // setup allocator
    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    defer _ = gpa.deinit();
    const allocator = gpa.allocator();

    // acquire command line args
    const args = try std.process.argsAlloc(allocator);
    defer std.process.argsFree(allocator, args);
    if (args.len != 3) {
        std.debug.print("Usage: ./mkdir <path> <root_hash>\n", .{});
        return;
    }
    const path = args[1];
    const root_hash = args[2];

    var updated_root_hash: [HASH_SIZE]u8 = undefined;
    try mkdirImpl(path, root_hash, &updated_root_hash);
    std.debug.print("{s}\n", .{updated_root_hash});
}
