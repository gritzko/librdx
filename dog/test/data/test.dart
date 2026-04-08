abstract class Printable {
  void display();
}

enum Color { red, green, blue }

class Point {
  double x;
  double y;

  Point(this.x, this.y);

  double distance() {
    return (x * x + y * y);
  }
}

class Animal implements Printable {
  String name;
  int age;

  Animal(this.name, this.age);

  @override
  void display() {
    print('$name age=$age');
  }

  String greet() {
    return 'Hello, I am $name';
  }
}

class Dog extends Animal {
  String breed;

  Dog(String name, int age, this.breed) : super(name, age);

  @override
  void display() {
    print('$name [$breed]');
  }
}

List<String> process(List<String> items) {
  return items
      .where((s) => !s.startsWith('#'))
      .map((s) => s.trim())
      .toList();
}

String greet(String name) {
  return 'Hello, $name!';
}

void main() {
  var dog = Dog('Rex', 5, 'Lab');
  dog.display();
  var p = Point(3.0, 4.0);
  print(p.distance());
  print(process(['a', '# b', ' c ']));
}
