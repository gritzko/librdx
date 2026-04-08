import java.util.List;
import java.util.ArrayList;
import java.util.stream.Collectors;

interface Printable {
    void print();
}

record Pair(int first, int second) {}

enum Direction {
    NORTH, SOUTH, EAST, WEST;

    public boolean isVertical() {
        return this == NORTH || this == SOUTH;
    }
}

class Animal implements Printable {
    private String name;
    protected int age;

    public Animal(String name, int age) {
        this.name = name;
        this.age = age;
    }

    public String getName() {
        return name;
    }

    public void print() {
        System.out.println(name + " age=" + age);
    }

    public static Animal create(String name) {
        return new Animal(name, 0);
    }
}

class Dog extends Animal {
    private String breed;

    public Dog(String name, int age, String breed) {
        super(name, age);
        this.breed = breed;
    }

    @Override
    public void print() {
        System.out.println(getName() + " [" + breed + "]");
    }
}

public class Test {
    public static List<String> process(List<String> items) {
        return items.stream()
            .filter(s -> !s.startsWith("#"))
            .map(String::trim)
            .collect(Collectors.toList());
    }

    public static void main(String[] args) {
        Dog d = new Dog("Rex", 5, "Lab");
        d.print();
        Pair p = new Pair(1, 2);
        Direction dir = Direction.NORTH;
        System.out.println(dir.isVertical());
    }
}
