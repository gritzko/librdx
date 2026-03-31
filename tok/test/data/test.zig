const std = @import("std");
const mem = std.mem;

const MAX_SIZE: usize = 1024;

const Point = struct {
    x: f64,
    y: f64,

    fn init(x: f64, y: f64) Point {
        return Point{ .x = x, .y = y };
    }

    fn distance(self: Point) f64 {
        return @sqrt(self.x * self.x + self.y * self.y);
    }
};

const Color = enum {
    red,
    green,
    blue,
};

fn clamp(val: f64, lo: f64, hi: f64) f64 {
    if (val < lo) return lo;
    if (val > hi) return hi;
    return val;
}

fn indexOf(haystack: []const u8, needle: u8) ?usize {
    for (haystack, 0..) |byte, i| {
        if (byte == needle) return i;
    }
    return null;
}

const Error = error{
    OutOfBounds,
    NotFound,
};

fn process(items: []const []const u8) !std.ArrayList([]const u8) {
    var result = std.ArrayList([]const u8).init(std.heap.page_allocator);
    for (items) |item| {
        if (item.len > 0 and item[0] == '#') continue;
        try result.append(item);
    }
    return result;
}

pub fn main() !void {
    const p = Point.init(3.0, 4.0);
    const stdout = std.io.getStdOut().writer();
    try stdout.print("dist={d}\n", .{p.distance()});
    try stdout.print("clamped={d}\n", .{clamp(10.0, 0.0, 5.0)});
}
