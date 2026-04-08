import Foundation

protocol Printable {
    func display()
}

enum Color {
    case red, green, blue
}

struct Point {
    var x: Double
    var y: Double

    func distance() -> Double {
        return (x * x + y * y).squareRoot()
    }
}

class Animal: Printable {
    let name: String
    var age: Int

    init(name: String, age: Int) {
        self.name = name
        self.age = age
    }

    func display() {
        print(name)
    }

    func greet() -> String {
        return "Hello"
    }
}

class Dog: Animal {
    let breed: String

    init(name: String, age: Int, breed: String) {
        self.breed = breed
        super.init(name: name, age: age)
    }

    override func display() {
        print(name)
        print(breed)
    }
}

func process(items: [String]) -> [String] {
    return items
        .filter { !$0.hasPrefix("#") }
        .map { $0.trimmingCharacters(in: .whitespaces) }
}

let defaultName = "World"
var counter = 0

func greet(name: String) -> String {
    counter += 1
    return "Hello"
}
