use std::fmt;

const MAX_SIZE: usize = 1024;
static COUNTER: std::sync::atomic::AtomicUsize =
    std::sync::atomic::AtomicUsize::new(0);

struct Point {
    x: f64,
    y: f64,
}

enum Shape {
    Circle(f64),
    Rect(f64, f64),
}

trait Drawable {
    fn draw(&self);
    fn area(&self) -> f64;
}

impl Point {
    fn new(x: f64, y: f64) -> Self {
        Point { x, y }
    }

    fn distance(&self) -> f64 {
        (self.x * self.x + self.y * self.y).sqrt()
    }
}

impl fmt::Display for Point {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "({}, {})", self.x, self.y)
    }
}

impl Drawable for Shape {
    fn draw(&self) {
        match self {
            Shape::Circle(r) => println!("Circle r={}", r),
            Shape::Rect(w, h) => println!("Rect {}x{}", w, h),
        }
    }

    fn area(&self) -> f64 {
        match self {
            Shape::Circle(r) => std::f64::consts::PI * r * r,
            Shape::Rect(w, h) => w * h,
        }
    }
}

mod utils {
    pub fn clamp(val: f64, lo: f64, hi: f64) -> f64 {
        if val < lo { lo } else if val > hi { hi } else { val }
    }
}

fn main() {
    let p = Point::new(3.0, 4.0);
    println!("{} dist={}", p, p.distance());
    let s = Shape::Circle(5.0);
    s.draw();
    println!("area={}", s.area());
}
