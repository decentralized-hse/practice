const std = @import("std");

const Sha256 = std.crypto.hash.sha2.Sha256;
const sha256SumLengthHex = 2 * Sha256.digest_length;
const cwd = std.fs.cwd();

fn copyStreamBuffered(reader: anytype, writer: anytype) !void {
    var buffer: [4096]u8 = undefined;
    while (true) {
        const n = try reader.readAll(&buffer);
        try writer.writeAll(buffer[0..n]);

        if (n < buffer.len) {
            break;
        }
    }
}

fn sha256SumAsHex(reader: anytype) ![sha256SumLengthHex]u8 {
    var sha256 = Sha256.init(.{});
    try copyStreamBuffered(reader, sha256.writer());

    return std.fmt.bytesToHex(sha256.finalResult(), std.fmt.Case.lower);
}

fn checkNameValidity(name: []const u8) bool {
    for (name) |c| {
        if (c == ':' or std.ascii.isWhitespace(c)) {
            return false;
        }
    }

    return true;
}

fn findHashByName(
    allocator: std.mem.Allocator,
    inputHash: []const u8,
    outputHash: *[sha256SumLengthHex]u8,
    name: []const u8,
    delimeter: []const u8,
) !void {
    if (!checkNameValidity(name)) {
        return error.IncorrectPath;
    }

    const file = try cwd.openFile(inputHash, .{});
    defer file.close();

    if (!std.mem.eql(u8, inputHash, &(try sha256SumAsHex(file.reader())))) {
        return error.HashSumMismatch;
    }
    try file.seekTo(0);

    var line = std.ArrayList(u8).init(allocator);
    defer line.deinit();
    var buffered = std.io.bufferedReader(file.reader());
    var reader = buffered.reader();
    while (reader.streamUntilDelimiter(line.writer(), '\n', null)) {
        defer line.clearRetainingCapacity();

        var lineIter = std.mem.splitSequence(u8, line.items, delimeter);
        if (std.mem.eql(u8, try (lineIter.next() orelse error.IncorrectDirEntry), name)) {
            const hash = try (lineIter.next() orelse error.IncorrectDirEntry);

            if (lineIter.next() != null or hash.len != sha256SumLengthHex) {
                return error.IncorrectDirEntry;
            }

            std.mem.copy(u8, outputHash, hash);

            return;
        }
    } else |err| switch (err) {
        error.EndOfStream => {},
        else => return err,
    }

    return error.DirEntryNotFound;
}

fn executeGetCommand() !void {
    var arena = std.heap.ArenaAllocator.init(std.heap.page_allocator);
    defer arena.deinit();
    const allocator = arena.allocator();

    var args = try std.process.argsWithAllocator(allocator);
    defer args.deinit();
    _ = args.next();
    const path = try (args.next() orelse error.IncorrectCmdArgs);
    var hashNullTerminated = try (args.next() orelse error.IncorrectCmdArgs);
    if (hashNullTerminated.len != sha256SumLengthHex or args.next() != null) {
        return error.IncorrectCmdArgs;
    }

    var hash: [sha256SumLengthHex]u8 = undefined;
    std.mem.copy(u8, &hash, hashNullTerminated);

    var numPathTokens: u64 = 1;
    for (path) |c| {
        numPathTokens += @intFromBool(c == '/');
    }
    var pathIter = std.mem.splitScalar(u8, path, '/');
    while (numPathTokens > 1) : (numPathTokens -= 1) {
        const name = pathIter.next().?;
        try findHashByName(allocator, &hash, &hash, name, "/\t");
    }
    const name = pathIter.next().?;
    try findHashByName(allocator, &hash, &hash, name, ":\t");

    const blob = try cwd.openFile(&hash, .{});
    defer blob.close();
    try copyStreamBuffered(blob.reader(), std.io.getStdOut().writer());
}

pub fn main() void {
    executeGetCommand() catch |err| std.log.err("{s}", .{@errorName(err)});
}
